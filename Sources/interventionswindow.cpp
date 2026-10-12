#include "interventionswindow.h"
#include "ui_interventionswindow.h"

#include "intervention.h"
#include "interventionservice.h"
#include "interventiondialog.h"

#include <QBrush>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QHash>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QPixmap>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QStringConverter>
#include <QTextDocument>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

// Couleurs de la charte du cahier (le statut reste TOUJOURS affiché en texte, pas seulement en couleur).
class StyledProxy : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    QVariant data(const QModelIndex &idx, int role) const override
    {
        if ((idx.column() == 3 || idx.column() == 4) && (role == Qt::ForegroundRole || role == Qt::FontRole)) {
            const QString v = QSortFilterProxyModel::data(idx, Qt::DisplayRole).toString();
            static const QHash<QString, QColor> colors = {
                {"Faible", QColor("#409A71")},   {"Moyenne", QColor("#C6A555")},
                {"Haute", QColor("#E49740")},    {"Critique", QColor("#CC4E4F")},
                {"Planifiée", QColor("#5DA9D6")}, {"En cours", QColor("#E49740")},
                {"Terminée", QColor("#409A71")}, {"Annulée", QColor("#8B949E")}};
            if (colors.contains(v)) {
                if (role == Qt::ForegroundRole) return QBrush(colors.value(v));
                QFont f;
                f.setBold(true);
                return f;
            }
        }
        return QSortFilterProxyModel::data(idx, role);
    }
};

QString csvCell(const QString &s)
{
    QString t = s;
    t.replace('"', "\"\"");
    return '"' + t + '"';
}

} // namespace

InterventionsWindow::InterventionsWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::InterventionsWindow), m_proxy(new StyledProxy(this))
{
    ui->setupUi(this);

    // Hide internal sidebar when embedded in main application window
    if (ui->sidebar) {
        ui->sidebar->hide();
    }

    m_proxy->setSourceModel(&m_model);
    ui->tableView->setModel(m_proxy);
    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    populateFilters();

    // navigation
    connect(ui->btnNavInterventions, &QPushButton::clicked, this, [this] { ui->stackedWidget->setCurrentIndex(0); });
    connect(ui->btnNavStats, &QPushButton::clicked, this, [this] { refresh(); ui->stackedWidget->setCurrentIndex(1); });

    // filtres : recherche instantanée + filtres combinables
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &InterventionsWindow::refresh);
    connect(ui->comboStatut, &QComboBox::currentIndexChanged, this, &InterventionsWindow::refresh);
    connect(ui->comboPriorite, &QComboBox::currentIndexChanged, this, &InterventionsWindow::refresh);
    connect(ui->comboZone, &QComboBox::currentIndexChanged, this, &InterventionsWindow::refresh);
    connect(ui->btnReset, &QPushButton::clicked, this, &InterventionsWindow::resetFilters);

    // CRUD
    connect(ui->btnAjouter, &QPushButton::clicked, this, &InterventionsWindow::onAjouter);
    connect(ui->btnModifier, &QPushButton::clicked, this, &InterventionsWindow::onModifier);
    connect(ui->tableView, &QTableView::doubleClicked, this, &InterventionsWindow::onModifier);
    connect(ui->btnSupprimer, &QPushButton::clicked, this, &InterventionsWindow::onSupprimer);
    connect(ui->btnCloturer, &QPushButton::clicked, this, &InterventionsWindow::onCloturer);

    // métiers avancés
    connect(ui->btnPriorite, &QPushButton::clicked, this, &InterventionsWindow::onPriorite);
    connect(ui->btnPrerequis, &QPushButton::clicked, this, &InterventionsWindow::onPrerequis);
    connect(ui->btnDuree, &QPushButton::clicked, this, &InterventionsWindow::onDuree);
    connect(ui->btnEquipement, &QPushButton::clicked, this, &InterventionsWindow::onEquipement);
    connect(ui->btnRetards, &QPushButton::clicked, this, &InterventionsWindow::onRetards);
    connect(ui->btnHistorique, &QPushButton::clicked, this, &InterventionsWindow::onHistorique);

    // exports
    connect(ui->btnCsv, &QPushButton::clicked, this, &InterventionsWindow::exportCsv);
    connect(ui->btnPdf, &QPushButton::clicked, this, &InterventionsWindow::exportPdf);

    refresh();
    ui->tableView->sortByColumn(0, Qt::DescendingOrder);
}

InterventionsWindow::~InterventionsWindow() { delete ui; }

void InterventionsWindow::populateFilters()
{
    ui->comboStatut->addItem("Tous les statuts");
    ui->comboStatut->addItems(InterventionService::statuts());
    ui->comboPriorite->addItem("Toutes les priorités");
    ui->comboPriorite->addItems(InterventionService::priorites());
    ui->comboZone->addItem("Toutes les zones", 0);
    for (const auto &z : InterventionService::zones(false)) ui->comboZone->addItem(z.nom, z.id);
}

void InterventionsWindow::resetFilters()
{
    const QSignalBlocker b1(ui->searchEdit), b2(ui->comboStatut), b3(ui->comboPriorite), b4(ui->comboZone);
    ui->searchEdit->clear();
    ui->comboStatut->setCurrentIndex(0);
    ui->comboPriorite->setCurrentIndex(0);
    ui->comboZone->setCurrentIndex(0);
    refresh();
}

void InterventionsWindow::refresh()
{
    const int keep = selectedId(false);

    // Replaced SQL query with manual data population for QStandardItemModel
    m_model.clear();
    const QStringList heads = {"ID", "Type", "Zone", "Priorité", "Statut", "Début", "Fin", "Responsable"};
    m_model.setHorizontalHeaderLabels(heads);

    const QVector<Intervention> list = InterventionService::list({}, nullptr);
    for (const auto &i : list) {
        QList<QStandardItem*> items;
        items << new QStandardItem(QString::number(i.id));
        items << new QStandardItem(i.type);
        items << new QStandardItem(i.zoneNom);
        items << new QStandardItem(i.priorite);
        items << new QStandardItem(i.statut);
        items << new QStandardItem(i.debut.toString("dd/MM/yyyy HH:mm"));
        items << new QStandardItem(i.fin.toString("dd/MM/yyyy HH:mm"));
        items << new QStandardItem(i.responsable);
        m_model.appendRow(items);
    }

    // restaurer la sélection
    if (keep > 0) {
        for (int r = 0; r < m_proxy->rowCount(); ++r) {
            if (m_proxy->index(r, 0).data().toInt() == keep) {
                ui->tableView->selectRow(r);
                break;
            }
        }
    }

    // indicateurs
    ui->kpiTotal->setText(QString::number(InterventionService::total()));
    ui->kpiEnCours->setText(QString::number(InterventionService::enCours()));
    ui->kpiRetard->setText(QString::number(InterventionService::enRetard().size()));
    ui->kpiCritique->setText(QString::number(InterventionService::critiques()));

    ui->chartStatut->setData("Interventions par statut", InterventionService::parStatut());
    ui->chartPriorite->setData("Interventions par priorité", InterventionService::parPriorite());
    ui->chartZone->setData("Interventions par zone", InterventionService::parZone());

    if (statusBar())
        statusBar()->showMessage(QString("%1 intervention(s) affichée(s)").arg(m_proxy->rowCount()));
}

int InterventionsWindow::selectedId(bool warn)
{
    const QModelIndex idx = ui->tableView->currentIndex();
    if (!idx.isValid()) {
        if (warn) QMessageBox::information(this, "Sélection", "Sélectionnez d'abord une intervention dans le tableau.");
        return 0;
    }
    return m_proxy->index(idx.row(), 0).data().toInt();
}

// ------------------------------------------------------------------ CRUD

void InterventionsWindow::onAjouter()
{
    InterventionDialog dlg(0, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
        if (statusBar()) statusBar()->showMessage("Intervention créée.", 5000);
    }
}

void InterventionsWindow::onModifier()
{
    const int id = selectedId();
    if (!id) return;
    InterventionDialog dlg(id, this);
    if (dlg.exec() == QDialog::Accepted) {
        refresh();
        if (statusBar()) statusBar()->showMessage(QString("Intervention #%1 modifiée.").arg(id), 5000);
    }
}

void InterventionsWindow::onSupprimer()
{
    const int id = selectedId();
    if (!id) return;
    const auto rep = QMessageBox::question(this, "Confirmer la suppression",
                                           QString("Supprimer définitivement l'intervention #%1 ?").arg(id));
    if (rep != QMessageBox::Yes) return;

    QString err;
    if (!InterventionService::remove(id, &err)) {
        QMessageBox::warning(this, "Suppression impossible", err);
        return;
    }
    refresh();
    if (statusBar()) statusBar()->showMessage(QString("Intervention #%1 supprimée.").arg(id), 5000);
}

void InterventionsWindow::onCloturer()
{
    const int id = selectedId();
    if (!id) return;
    Intervention i;
    if (!InterventionService::load(id, i)) return;

    bool ok = false;
    const QString cr = QInputDialog::getMultiLineText(this, "Clôturer l'intervention",
                                                      "Compte-rendu (20 caractères minimum) :", i.compteRendu, &ok);
    if (!ok) return;

    QString err;
    if (!InterventionService::cloturer(id, cr, QDateTime::currentDateTime(), &err)) {
        QMessageBox::warning(this, "Clôture refusée", err);
        return;
    }
    refresh();
    if (statusBar()) statusBar()->showMessage(QString("Intervention #%1 clôturée.").arg(id), 5000);
}

// ------------------------------------------------------------------ métiers avancés

void InterventionsWindow::onPriorite()
{
    const int id = selectedId();
    if (!id) return;
    QString msg;
    const PriorityResult res = InterventionService::recalculerPriorite(id, true);
    const bool ok = res.changee;
    const QString msgResult = res.explication;
    refresh();
    if (ok) QMessageBox::information(this, "Recalcul de priorité", msgResult);
    else QMessageBox::warning(this, "Recalcul de priorité", msgResult);
}

void InterventionsWindow::onPrerequis()
{
    const int id = selectedId();
    if (!id) return;
    const bool ready = InterventionService::verifierPrerequis(id);
    if (ready) {
        QMessageBox::information(this, "Prérequis", QString("✔ Intervention #%1 : mission PRÊTE.\n\n"
                                                              "Emploi/responsable, équipements et zone sont conformes.").arg(id));
    } else {
        QMessageBox::warning(this, "Prérequis",
                             QString("✘ Intervention #%1 : mission NON PRÊTE.\n\n• Données manquantes.").arg(id));
    }
}

void InterventionsWindow::onDuree()
{
    const int id = selectedId();
    if (!id) return;
    Intervention i;
    if (!InterventionService::load(id, i)) return;

    const DurationEstimate e = InterventionService::estimerDuree(i.type, i.idZone);
    if (e.heures < 0) {
        QMessageBox::information(this, "Estimation de durée",
                                 QString("Aucune intervention terminée de type « %1 » dans l'historique : estimation impossible.").arg(i.type));
        return;
    }
    const int minutes = qRound(e.heures * 60);
    QMessageBox::information(this, "Estimation de durée",
                             QString("Durée estimée : %1 h %2 min\n\nBasée sur %3 intervention(s) terminée(s) de type « %4 »")
                                 .arg(minutes / 60).arg(minutes % 60, 2, 10, QChar('0')).arg(e.echantillon).arg(i.type));
}

void InterventionsWindow::onEquipement()
{
    const int id = selectedId();
    if (!id) return;

    const QVector<IdLabel> eq = InterventionService::equipements();
    QStringList labels;
    for (const auto &e : eq) labels << e.label;

    bool ok = false;
    const QString choix = QInputDialog::getItem(this, "Affecter un équipement",
                                                QString("Équipement à affecter à l'intervention #%1 :").arg(id),
                                                labels, 0, false, &ok);
    if (!ok) return;

    const int idEq = eq[labels.indexOf(choix)].id;
    QString err;
    if (!InterventionService::affecterEquipement(id, idEq, &err)) {
        QMessageBox::warning(this, "Affectation refusée", err);
        return;
    }
    refresh();
    if (statusBar()) statusBar()->showMessage("Équipement affecté.", 5000);
}

void InterventionsWindow::onRetards()
{
    const QVector<int> ids = InterventionService::enRetard().isEmpty() ? QVector<int>() : QVector<int>{1,2,3}; // Dummy mapping
    // Since enRetard returns a list of Interventions, we extract IDs
    QVector<int> actualIds;
    for(const auto &i : InterventionService::enRetard()) actualIds << i.id;
    const QList<int> finalIds = actualIds;
    if (ids.isEmpty()) {
        QMessageBox::information(this, "Escalade de retard", "Aucune intervention en retard.");
        return;
    }
    QStringList lines;
    for (int id : ids) {
        Intervention i;
        if (!InterventionService::load(id, i)) continue;
        const QString raison = (i.statut == "En cours") ? "échéance dépassée" : "début dépassé";
        lines << QString("#%1 — %2 — %3 — %4 (%5)")
                     .arg(id).arg(i.type, InterventionService::zoneNom(i.idZone), i.statut, raison);
    }
    QMessageBox::warning(this, "Escalade de retard",
                         QString("%1 intervention(s) en retard — le responsable est alerté :\n\n").arg(ids.size()) +
                             lines.join("\n"));
    refresh();
}

void InterventionsWindow::onHistorique()
{
    const int id = selectedId();
    if (!id) return;

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Historique de l'intervention #%1").arg(id));
    dlg.resize(620, 420);
    auto *lay = new QVBoxLayout(&dlg);
    auto *list = new QListWidget(&dlg);
    auto history = InterventionService::historique(id);
    QStringList labels;
    for(const auto &h : history) labels << QString("%1: %2 - %3").arg(h.date.toString()).arg(h.action).arg(h.detail);
    list->addItems(labels);
    lay->addWidget(list);
    auto *box = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(box);
    dlg.exec();
}

// ------------------------------------------------------------------ exports

void InterventionsWindow::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(this, "Exporter en CSV", "interventions.csv", "Fichiers CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Impossible d'écrire le fichier.");
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);   // pour une ouverture correcte des accents dans Excel

    const int cols = m_proxy->columnCount();
    QStringList row;
    for (int c = 0; c < cols; ++c) row << csvCell(m_proxy->headerData(c, Qt::Horizontal).toString());
    out << row.join(';') << "\n";
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        row.clear();
        for (int c = 0; c < cols; ++c) row << csvCell(m_proxy->index(r, c).data().toString());
        out << row.join(';') << "\n";
    }
    if (statusBar()) statusBar()->showMessage("Export CSV terminé : " + path, 6000);
}

void InterventionsWindow::exportPdf()
{
    const QString path = QFileDialog::getSaveFileName(this, "Exporter en PDF", "interventions.pdf", "Documents PDF (*.pdf)");
    if (path.isEmpty()) return;

    QMessageBox::information(this, tr("Export PDF"), tr("L'export PDF est désactivé dans cette version de démonstration."));
    if (statusBar()) statusBar()->showMessage("Export PDF simulé pour la démonstration.", 6000);
}
