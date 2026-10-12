#include "zonespage.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextDocument>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>

#include "mapwidget.h"
#include "theme.h"
#include "uihelpers.h"
#include "zonedetailsdialog.h"
#include "zonedialog.h"
#include "zonerepository.h"
#include "zoneservice.h"

namespace {
enum Col { ColCode, ColNom, ColVuln, ColNiveau, ColStations, ColCouverture, ColStatut, ColCount };
}

// ------------------------------------------------------------
//  Proxy : recherche instantanee + filtres combines (statut, niveau)
//  Le tri se fait en cliquant sur l'en-tete d'une colonne.
// ------------------------------------------------------------
class ZoneFilterProxy : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setCriteria(const QString &text, const QString &statut, const QString &niveau)
    {
        m_text = text; m_statut = statut; m_niveau = niveau;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        const QAbstractItemModel *m = sourceModel();
        auto cell = [&](int c) { return m->index(row, c, parent).data().toString(); };

        if (!m_text.isEmpty()) {
            QStringList all;
            for (int c = 0; c < m->columnCount(); ++c) all << cell(c);
            if (!all.join(' ').contains(m_text, Qt::CaseInsensitive)) return false;
        }
        if (m_statut != QLatin1String("Tous") && cell(ColStatut) != m_statut) return false;
        if (m_niveau != QLatin1String("Tous") && cell(ColNiveau) != m_niveau) return false;
        return true;
    }

private:
    QString m_text, m_statut = QStringLiteral("Tous"), m_niveau = QStringLiteral("Tous");
};

// ------------------------------------------------------------
ZonesPage::ZonesPage(ZoneRepository *repo, QWidget *parent) : QWidget(parent), m_repo(repo)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // ---- 1) Indicateurs ----
    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(14);
    kpis->addWidget(makeKpiCard(QStringLiteral("🗺"), QStringLiteral("Zones"), m_kpiZones));
    kpis->addWidget(makeKpiCard(QStringLiteral("✅"), QStringLiteral("Zones actives"), m_kpiActives));
    kpis->addWidget(makeKpiCard(QStringLiteral("📶"), QStringLiteral("Couverture réseau"), m_kpiCouverture));
    kpis->addWidget(makeKpiCard(QStringLiteral("🕒"), QStringLiteral("Dernière mise à jour"), m_kpiMaj));
    root->addLayout(kpis);

    // ---- 2) Barre d'outils ----
    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("🔍  Rechercher une zone (nom, code, niveau...)"));
    m_search->setClearButtonEnabled(true);
    m_statutBox = new QComboBox;
    m_statutBox->addItems({QStringLiteral("Tous"), QStringLiteral("Active"), QStringLiteral("Inactive")});
    m_niveauBox = new QComboBox;
    m_niveauBox->addItems({QStringLiteral("Tous"), QStringLiteral("Faible"), QStringLiteral("Moyen"), QStringLiteral("Élevé")});
    auto *btnPdf = new QPushButton(QStringLiteral("Exporter PDF"));
    auto *btnCsv = new QPushButton(QStringLiteral("Exporter CSV"));
    auto *btnNew = new QPushButton(QStringLiteral("＋  Nouvelle zone"));
    btnNew->setObjectName("primary");

    auto *bar = new QHBoxLayout;
    bar->setSpacing(10);
    bar->addWidget(m_search, 1);
    bar->addWidget(new QLabel(QStringLiteral("Statut")));
    bar->addWidget(m_statutBox);
    bar->addWidget(new QLabel(QStringLiteral("Niveau")));
    bar->addWidget(m_niveauBox);
    bar->addWidget(btnPdf);
    bar->addWidget(btnCsv);
    bar->addWidget(btnNew);
    root->addLayout(bar);

    // ---- 3) Contenu : tableau (gauche) + carte & detail (droite) ----
    // Tableau
    QVBoxLayout *tl = nullptr;
    QFrame *tableCard = makeCard(tl);
    auto *tTitle = new QLabel(QStringLiteral("Liste des zones"));
    tTitle->setObjectName("cardTitle");
    tl->addWidget(tTitle);

    m_model = new QStandardItemModel(0, ColCount, this);
    m_model->setHorizontalHeaderLabels({QStringLiteral("Code"), QStringLiteral("Zone"),
        QStringLiteral("Vulnérabilité"), QStringLiteral("Niveau"), QStringLiteral("Stations"),
        QStringLiteral("Couverture"), QStringLiteral("Statut")});
    m_proxy = new ZoneFilterProxy(this);
    m_proxy->setSourceModel(m_model);

    m_table = new QTableView;
    m_table->setModel(m_proxy);
    m_table->setSortingEnabled(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(38);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->sortByColumn(ColCode, Qt::AscendingOrder);
    tl->addWidget(m_table, 1);

    // Carte
    m_map = new MapWidget;

    // Panneau de detail
    QVBoxLayout *dl = nullptr;
    QFrame *detailCard = makeCard(dl);
    m_detailTitle = new QLabel;
    m_detailTitle->setObjectName("cardTitle");
    m_detailBody = new QLabel;
    m_detailBody->setTextFormat(Qt::RichText);
    m_detailBody->setWordWrap(true);
    m_detailBody->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_btnEdit = new QPushButton(QStringLiteral("Modifier"));
    m_btnDelete = new QPushButton(QStringLiteral("Supprimer"));
    m_btnDelete->setObjectName("danger");
    m_btnDetails = new QPushButton(QStringLiteral("Voir détails"));
    m_btnDetails->setObjectName("primary");
    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_btnEdit);
    btnRow->addWidget(m_btnDelete);
    dl->addWidget(m_detailTitle);
    dl->addWidget(m_detailBody, 1);
    dl->addLayout(btnRow);
    dl->addWidget(m_btnDetails);

    auto *right = new QVBoxLayout;
    right->setSpacing(14);
    right->addWidget(m_map, 3);
    right->addWidget(detailCard, 4);

    auto *content = new QHBoxLayout;
    content->setSpacing(14);
    content->addWidget(tableCard, 3);
    content->addLayout(right, 2);
    root->addLayout(content, 1);

    // ---- Connexions (signaux -> slots) ----
    connect(m_search, &QLineEdit::textChanged, this, &ZonesPage::applyFilters);
    connect(m_statutBox, &QComboBox::currentTextChanged, this, &ZonesPage::applyFilters);
    connect(m_niveauBox, &QComboBox::currentTextChanged, this, &ZonesPage::applyFilters);
    connect(btnNew, &QPushButton::clicked, this, &ZonesPage::onAdd);
    connect(btnPdf, &QPushButton::clicked, this, &ZonesPage::exportPdf);
    connect(btnCsv, &QPushButton::clicked, this, &ZonesPage::exportCsv);
    connect(m_btnEdit, &QPushButton::clicked, this, &ZonesPage::onEdit);
    connect(m_btnDelete, &QPushButton::clicked, this, &ZonesPage::onDelete);
    connect(m_btnDetails, &QPushButton::clicked, this, &ZonesPage::onDetails);
    connect(m_table, &QTableView::doubleClicked, this, &ZonesPage::onEdit);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &ZonesPage::updateDetail);
    connect(m_map, &MapWidget::zoneClicked, this, &ZonesPage::selectZoneById);
    connect(m_repo, &ZoneRepository::changed, this, &ZonesPage::refresh);

    refresh();
}

// ------------------------------------------------------------
void ZonesPage::refresh()
{
    const int keepId = currentZoneId();
    const QList<Zone> zones = m_repo->all();

    m_model->removeRows(0, m_model->rowCount());
    for (const Zone &z : zones) {
        const QString niv = ZoneService::niveau(z.vulnerabilite);
        const bool sous = ZoneService::sousCouverture(z);

        auto *code = new QStandardItem(z.code);
        code->setData(z.id, Qt::UserRole);            // on garde l'id cache dans la 1ere cellule
        auto *nom = new QStandardItem(z.nom);
        auto *vuln = new QStandardItem;
        vuln->setData(z.vulnerabilite, Qt::DisplayRole);  // nombre => tri numerique
        auto *nivItem = new QStandardItem(niv);
        nivItem->setForeground(Theme::levelColor(niv));
        auto *st = new QStandardItem;
        st->setData(z.nbStations, Qt::DisplayRole);
        auto *cov = new QStandardItem(sous ? QStringLiteral("Insuffisante") : QStringLiteral("OK"));
        cov->setForeground(Theme::levelColor(sous ? QStringLiteral("Insuffisante") : QStringLiteral("OK")));
        auto *statut = new QStandardItem(z.statut);
        statut->setForeground(z.statut == QLatin1String("Active") ? Theme::levelColor("OK") : Theme::mutedColor());

        QList<QStandardItem *> row{code, nom, vuln, nivItem, st, cov, statut};
        for (auto *it : row) it->setEditable(false);
        m_model->appendRow(row);
    }

    m_kpiZones->setText(QString::number(zones.size()));
    int actives = 0;
    for (const Zone &z : zones) if (z.statut == QLatin1String("Active")) ++actives;
    m_kpiActives->setText(QStringLiteral("%1 / %2").arg(actives).arg(zones.size()));
    m_kpiCouverture->setText(QStringLiteral("%1 %").arg(ZoneService::couvertureReseau(zones)));
    m_kpiMaj->setText(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));

    m_map->setZones(zones);
    if (keepId != -1) selectZoneById(keepId);
    updateDetail();
}

void ZonesPage::applyFilters()
{
    m_proxy->setCriteria(m_search->text().trimmed(), m_statutBox->currentText(), m_niveauBox->currentText());
}

int ZonesPage::currentZoneId() const
{
    const QModelIndex idx = m_table->currentIndex();
    if (!idx.isValid()) return -1;
    const QModelIndex src = m_proxy->mapToSource(idx);
    return m_model->item(src.row(), ColCode)->data(Qt::UserRole).toInt();
}

void ZonesPage::selectZoneById(int id)
{
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        const QModelIndex src = m_proxy->mapToSource(m_proxy->index(r, 0));
        if (m_model->item(src.row(), ColCode)->data(Qt::UserRole).toInt() == id) {
            m_table->setCurrentIndex(m_proxy->index(r, 0));
            m_table->selectRow(r);
            return;
        }
    }
}

void ZonesPage::updateDetail()
{
    const int id = currentZoneId();
    const auto zone = m_repo->find(id);
    m_btnEdit->setEnabled(zone.has_value());
    m_btnDelete->setEnabled(zone.has_value());
    m_btnDetails->setEnabled(zone.has_value());
    m_map->setSelected(id);

    if (!zone) {
        m_detailTitle->setText(QStringLiteral("Détail de la zone"));
        m_detailBody->setText(QStringLiteral("<span style='color:#8fb3c2'>Sélectionnez une zone dans le tableau "
                                             "ou sur la carte.</span>"));
        return;
    }
    const Zone &z = *zone;
    const QString niv = ZoneService::niveau(z.vulnerabilite);
    const bool sous = ZoneService::sousCouverture(z);
    const QList<Zone> ranking = ZoneService::classementPriorite(m_repo->all());
    int rang = 0;
    for (int i = 0; i < ranking.size(); ++i) if (ranking[i].id == z.id) rang = i + 1;

    auto row = [](const QString &k, const QString &v) {
        return QStringLiteral("<tr><td style='color:#8fb3c2; padding-right:14px; padding-bottom:5px'>%1</td>"
                              "<td style='padding-bottom:5px'>%2</td></tr>").arg(k, v);
    };
    QString html = QStringLiteral("<table>");
    html += row("Code", z.code);
    html += row("Niveau", QStringLiteral("<b style='color:%1'>%2</b> (%3/100)")
                              .arg(Theme::levelColor(niv).name(), niv).arg(z.vulnerabilite));
    html += row("Coordonnées", QStringLiteral("%1 ; %2").arg(z.latitude, 0, 'f', 4).arg(z.longitude, 0, 'f', 4));
    html += row("Stations", QStringLiteral("%1 / %2 attendues — %3").arg(z.nbStations)
                                .arg(ZoneService::stationsAttendues(z))
                                .arg(sous ? QStringLiteral("<b style='color:#e05a5b'>insuffisante</b>")
                                          : QStringLiteral("<b style='color:#3fc98f'>OK</b>")));
    html += row("Priorité", QStringLiteral("rang %1 / %2").arg(rang).arg(ranking.size()));
    html += row("Statut", z.statut);
    html += QStringLiteral("</table>");

    m_detailTitle->setText(QStringLiteral("Détail de la zone — %1").arg(z.nom));
    m_detailBody->setText(html);
}

// ------------------------------------------------------------
//  CRUD
// ------------------------------------------------------------
void ZonesPage::onAdd()
{
    ZoneDialog dlg(m_repo->all(), nullptr, this);
    if (dlg.exec() == QDialog::Accepted) {
        const int id = m_repo->add(dlg.zone());
        selectZoneById(id);
    }
}

void ZonesPage::onEdit()
{
    const auto zone = m_repo->find(currentZoneId());
    if (!zone) return;
    ZoneDialog dlg(m_repo->all(), &*zone, this);
    if (dlg.exec() == QDialog::Accepted) m_repo->update(dlg.zone());
}

void ZonesPage::onDelete()
{
    const auto zone = m_repo->find(currentZoneId());
    if (!zone) return;

    const auto rep = QMessageBox::question(this, QStringLiteral("Confirmer la suppression"),
        QStringLiteral("Supprimer définitivement la zone « %1 » ?").arg(zone->nom));
    if (rep != QMessageBox::Yes) return;

    QString erreur;
    if (!m_repo->remove(zone->id, &erreur)) {
        // Regle du cahier : pas de suppression si des donnees dependent de la zone -> on propose la desactivation
        const auto r2 = QMessageBox::question(this, QStringLiteral("Suppression impossible"),
            erreur + QStringLiteral("\n\nVoulez-vous la désactiver à la place ?"));
        if (r2 == QMessageBox::Yes) {
            Zone z = *zone;
            z.statut = QStringLiteral("Inactive");
            m_repo->update(z);
        }
    }
}

void ZonesPage::onDetails()
{
    const auto zone = m_repo->find(currentZoneId());
    if (!zone) return;
    const QList<Zone> ranking = ZoneService::classementPriorite(m_repo->all());
    int rang = 0;
    for (int i = 0; i < ranking.size(); ++i) if (ranking[i].id == zone->id) rang = i + 1;
    ZoneDetailsDialog dlg(*zone, m_repo->history(zone->id), rang, ranking.size(), this);
    dlg.exec();
}

// ------------------------------------------------------------
//  Export (on exporte ce qui est affiche : filtres et tri pris en compte)
// ------------------------------------------------------------
QList<QStringList> ZonesPage::visibleRows() const
{
    QList<QStringList> rows;
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        QStringList cells;
        for (int c = 0; c < ColCount; ++c) cells << m_proxy->index(r, c).data().toString();
        rows << cells;
    }
    return rows;
}

void ZonesPage::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Exporter en CSV"),
                                                      QStringLiteral("zones.csv"), QStringLiteral("CSV (*.csv)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Export"), QStringLiteral("Impossible d'écrire le fichier."));
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);   // pour qu'Excel lise bien les accents
    QStringList head;
    for (int c = 0; c < ColCount; ++c) head << m_model->headerData(c, Qt::Horizontal).toString();
    out << head.join(';') << "\n";
    for (const QStringList &r : visibleRows()) out << r.join(';') << "\n";
    QMessageBox::information(this, QStringLiteral("Export"), QStringLiteral("Fichier CSV généré avec succès."));
}

void ZonesPage::exportPdf()
{
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Exporter en PDF"),
                                                      QStringLiteral("zones.pdf"), QStringLiteral("PDF (*.pdf)"));
    if (path.isEmpty()) return;

    QString html = QStringLiteral("<h2>Smart Weather Management — Liste des zones</h2>"
                                  "<p>Généré le %1</p>"
                                  "<table border='1' cellspacing='0' cellpadding='6' width='100%'><tr>")
                       .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));
    for (int c = 0; c < ColCount; ++c)
        html += QStringLiteral("<th bgcolor='#dfeef5'>%1</th>").arg(m_model->headerData(c, Qt::Horizontal).toString());
    html += QStringLiteral("</tr>");
    for (const QStringList &r : visibleRows()) {
        html += QStringLiteral("<tr>");
        for (const QString &cell : r) html += QStringLiteral("<td>%1</td>").arg(cell.toHtmlEscaped());
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");

    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);
    QMessageBox::information(this, QStringLiteral("Export"), QStringLiteral("Fichier PDF généré avec succès."));
}
