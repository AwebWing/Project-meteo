#include "interventionwidget.h"
#include "barchartwidget.h"
#include "interventiondialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMarginsF>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QStringConverter>
#include <QTableView>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

namespace {

enum Col { ColId, ColType, ColZone, ColPriorite, ColStatut, ColDebut, ColFin, ColResp, ColEmploi, ColAlerte, ColCount };

const QString kDateFmt = QStringLiteral("dd/MM/yyyy HH:mm");

QColor prioriteColor(const QString &p)
{
    if (p == Priorite::Critique) return QColor("#CC4E4F");
    if (p == Priorite::Haute)    return QColor("#E49740");
    if (p == Priorite::Moyenne)  return QColor("#C6A555");
    return QColor("#409A71");
}

QColor statutColor(const QString &s)
{
    if (s == Statut::Terminee) return QColor("#409A71");
    if (s == Statut::EnCours)  return QColor("#347EA7");
    if (s == Statut::Annulee)  return QColor("#7A7A7A");
    return QColor("#162B3E");
}

QStandardItem *makeItem(const QString &text, const QVariant &sortKey)
{
    auto *it = new QStandardItem(text);
    it->setEditable(false);
    it->setData(sortKey, Qt::UserRole);
    return it;
}

QString fmtHeures(double h)
{
    const int minutes = qRound(h * 60.0);
    return QObject::tr("%1 h %2 min").arg(minutes / 60).arg(minutes % 60, 2, 10, QLatin1Char('0'));
}

QString csvCell(QString s)
{
    s.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    s.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return QLatin1Char('"') + s + QLatin1Char('"');
}

} // namespace

InterventionWidget::InterventionWidget(QWidget *parent) : QWidget(parent)
{
    buildUi();
    refresh();
}

// ------------------------------------------------------------------ interface

void InterventionWidget::buildUi()
{
    // ----- Titre + indicateurs -----
    auto *title = new QLabel(tr("Gestion des interventions"));
    title->setObjectName("title");

    auto makeKpi = [this](const QString &caption, QLabel *&valueLabel) {
        auto *frame = new QFrame;
        frame->setObjectName("kpi");
        auto *lay = new QVBoxLayout(frame);
        valueLabel = new QLabel("0");
        valueLabel->setObjectName("kpiValue");
        valueLabel->setAlignment(Qt::AlignCenter);
        auto *cap = new QLabel(caption);
        cap->setObjectName("kpiCaption");
        cap->setAlignment(Qt::AlignCenter);
        lay->addWidget(valueLabel);
        lay->addWidget(cap);
        return frame;
    };
    auto *kpiRow = new QHBoxLayout;
    kpiRow->addWidget(makeKpi(tr("Interventions"), m_kTotal));
    kpiRow->addWidget(makeKpi(tr("En cours"), m_kEnCours));
    kpiRow->addWidget(makeKpi(tr("En retard"), m_kRetard));
    kpiRow->addWidget(makeKpi(tr("Critiques actives"), m_kCritiques));

    // ----- Filtres -----
    m_search = new QLineEdit;
    m_search->setPlaceholderText(tr("Rechercher (n°, type, zone, responsable, statut…)"));
    m_search->setClearButtonEnabled(true);

    m_fStatut = new QComboBox;
    m_fStatut->addItem(tr("Tous les statuts"));
    m_fStatut->addItems(InterventionService::statuts());
    m_fPriorite = new QComboBox;
    m_fPriorite->addItem(tr("Toutes les priorités"));
    m_fPriorite->addItems(InterventionService::priorites());
    m_fType = new QComboBox;
    m_fType->addItem(tr("Tous les types"));
    m_fType->addItems(InterventionService::types());
    m_fZone = new QComboBox;
    m_fZone->addItem(tr("Toutes les zones"), 0);
    for (const ZoneItem &z : InterventionService::zones())
        m_fZone->addItem(z.nom, z.id);

    m_fPeriode = new QCheckBox(tr("Période du"));
    m_fDu = new QDateEdit(QDate::currentDate().addMonths(-3));
    m_fAu = new QDateEdit(QDate::currentDate().addMonths(1));
    for (auto *e : {m_fDu, m_fAu}) {
        e->setCalendarPopup(true);
        e->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    }

    auto *filters1 = new QHBoxLayout;
    filters1->addWidget(m_search, 3);
    filters1->addWidget(m_fStatut, 1);
    filters1->addWidget(m_fPriorite, 1);
    auto *filters2 = new QHBoxLayout;
    filters2->addWidget(m_fType, 1);
    filters2->addWidget(m_fZone, 1);
    filters2->addWidget(m_fPeriode);
    filters2->addWidget(m_fDu);
    filters2->addWidget(new QLabel(tr("au")));
    filters2->addWidget(m_fAu);

    // ----- Tableau -----
    m_model = new QStandardItemModel(0, ColCount, this);
    m_model->setHorizontalHeaderLabels({tr("N°"), tr("Type"), tr("Zone"), tr("Priorité"), tr("Statut"),
                                        tr("Début"), tr("Fin"), tr("Responsable"), tr("Emploi"), tr("Alerte")});
    m_model->setSortRole(Qt::UserRole);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(ColDebut, Qt::DescendingOrder);
    connect(m_table, &QTableView::doubleClicked, this, &InterventionWidget::onEdit);

    // ----- Actions -----
    auto button = [this](const QString &text, const char *objName, auto slot) {
        auto *b = new QPushButton(text);
        if (objName && *objName) b->setObjectName(objName);
        connect(b, &QPushButton::clicked, this, slot);
        return b;
    };
    auto *crud = new QHBoxLayout;
    crud->addWidget(button(tr("Ajouter"), "ok", &InterventionWidget::onAdd));
    crud->addWidget(button(tr("Modifier"), "", &InterventionWidget::onEdit));
    crud->addWidget(button(tr("Clôturer"), "ok", &InterventionWidget::onClose));
    crud->addWidget(button(tr("Annuler l'intervention"), "warn", &InterventionWidget::onCancel));
    crud->addWidget(button(tr("Supprimer"), "danger", &InterventionWidget::onDelete));
    crud->addStretch();
    crud->addWidget(button(tr("Historique"), "", &InterventionWidget::onHistory));
    crud->addWidget(button(tr("Export PDF"), "", &InterventionWidget::exportPdf));
    crud->addWidget(button(tr("Export CSV"), "", &InterventionWidget::exportCsv));

    auto *metiers = new QHBoxLayout;
    metiers->addWidget(new QLabel(tr("Aide à la décision :")));
    metiers->addWidget(button(tr("Recalculer la priorité"), "", &InterventionWidget::onRecalcPriority));
    metiers->addWidget(button(tr("Vérifier les prérequis"), "", &InterventionWidget::onPrerequisites));
    metiers->addWidget(button(tr("Estimer la durée"), "", &InterventionWidget::onEstimate));
    metiers->addWidget(button(tr("Interventions en retard"), "warn", &InterventionWidget::onLate));
    metiers->addStretch();

    m_msg = new QLabel;
    m_msg->setObjectName("message");

    auto *listPage = new QWidget;
    auto *listLay = new QVBoxLayout(listPage);
    listLay->addLayout(filters1);
    listLay->addLayout(filters2);
    listLay->addWidget(m_table, 1);
    listLay->addLayout(crud);
    listLay->addLayout(metiers);

    // ----- Onglet statistiques -----
    m_sDim = new QComboBox;
    m_sDim->addItem(tr("Statut"), "statut");
    m_sDim->addItem(tr("Priorité"), "priorite");
    m_sDim->addItem(tr("Type"), "type");
    m_sDim->addItem(tr("Zone"), "zone");
    m_sDim->addItem(tr("Mois"), "mois");
    m_sPeriode = new QCheckBox(tr("Période du"));
    m_sDu = new QDateEdit(QDate::currentDate().addMonths(-3));
    m_sAu = new QDateEdit(QDate::currentDate().addMonths(1));
    for (auto *e : {m_sDu, m_sAu}) {
        e->setCalendarPopup(true);
        e->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    }
    m_sTotal = new QLabel;
    m_chart = new BarChartWidget;

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_chart);

    auto *statsPage = new QWidget;
    auto *statsLay = new QVBoxLayout(statsPage);
    auto *statsTop = new QHBoxLayout;
    statsTop->addWidget(new QLabel(tr("Répartition par :")));
    statsTop->addWidget(m_sDim);
    statsTop->addSpacing(16);
    statsTop->addWidget(m_sPeriode);
    statsTop->addWidget(m_sDu);
    statsTop->addWidget(new QLabel(tr("au")));
    statsTop->addWidget(m_sAu);
    statsTop->addStretch();
    statsTop->addWidget(m_sTotal);
    statsLay->addLayout(statsTop);
    statsLay->addWidget(scroll, 1);

    auto *tabs = new QTabWidget;
    tabs->addTab(listPage, tr("Liste des interventions"));
    tabs->addTab(statsPage, tr("Statistiques"));

    auto *root = new QVBoxLayout(this);
    root->addWidget(title);
    root->addLayout(kpiRow);
    root->addWidget(tabs, 1);
    root->addWidget(m_msg);

    // ----- Connexions (après construction pour éviter les rafraîchissements prématurés) -----
    connect(m_search, &QLineEdit::textChanged, this, &InterventionWidget::refresh);
    for (auto *c : {m_fStatut, m_fPriorite, m_fType, m_fZone})
        connect(c, &QComboBox::currentIndexChanged, this, &InterventionWidget::refresh);
    connect(m_fPeriode, &QCheckBox::toggled, this, &InterventionWidget::refresh);
    connect(m_fDu, &QDateEdit::dateChanged, this, &InterventionWidget::refresh);
    connect(m_fAu, &QDateEdit::dateChanged, this, &InterventionWidget::refresh);

    connect(m_sDim, &QComboBox::currentIndexChanged, this, &InterventionWidget::refreshStats);
    connect(m_sPeriode, &QCheckBox::toggled, this, &InterventionWidget::refreshStats);
    connect(m_sDu, &QDateEdit::dateChanged, this, &InterventionWidget::refreshStats);
    connect(m_sAu, &QDateEdit::dateChanged, this, &InterventionWidget::refreshStats);
}

InterventionFilter InterventionWidget::currentFilter() const
{
    InterventionFilter f;
    f.texte = m_search->text();
    if (m_fStatut->currentIndex() > 0)   f.statut = m_fStatut->currentText();
    if (m_fPriorite->currentIndex() > 0) f.priorite = m_fPriorite->currentText();
    if (m_fType->currentIndex() > 0)     f.type = m_fType->currentText();
    f.idZone = m_fZone->currentData().toInt();
    f.periode = m_fPeriode->isChecked();
    f.du = m_fDu->date();
    f.au = m_fAu->date();
    return f;
}

int InterventionWidget::selectedId()
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows(ColId);
    if (rows.isEmpty()) {
        QMessageBox::information(this, tr("Sélection requise"), tr("Sélectionnez d'abord une intervention dans la liste."));
        return 0;
    }
    return rows.first().data(Qt::DisplayRole).toInt();
}

void InterventionWidget::notify(const QString &text)
{
    m_msg->setText(text);
    QTimer::singleShot(6000, m_msg, [this] { m_msg->clear(); });
}

// ------------------------------------------------------------------ données

void InterventionWidget::refresh()
{
    QString err;
    m_shown = InterventionService::list(currentFilter(), &err);
    if (!err.isEmpty())
        QMessageBox::critical(this, tr("Erreur base de données"), err);

    const QVector<Intervention> late = InterventionService::interventionsEnRetard();
    QSet<int> lateIds;
    for (const Intervention &l : late)
        lateIds.insert(l.id);

    populateTable(lateIds);
    updateKpis(late);
    refreshStats();
}

void InterventionWidget::populateTable(const QSet<int> &lateIds)
{
    auto *header = m_table->horizontalHeader();
    const int sortSection = header->sortIndicatorSection();
    const Qt::SortOrder sortOrder = header->sortIndicatorOrder();

    m_table->setSortingEnabled(false);
    m_model->setRowCount(0);

    for (const Intervention &i : std::as_const(m_shown)) {
        auto *id = makeItem(QString::number(i.id), i.id);
        id->setData(i.id, Qt::DisplayRole);
        auto *prio = makeItem(i.priorite, int(InterventionService::priorites().indexOf(i.priorite)));
        prio->setForeground(prioriteColor(i.priorite));
        QFont bold = prio->font();
        bold.setBold(true);
        prio->setFont(bold);
        auto *statut = makeItem(i.statut, i.statut);
        statut->setForeground(statutColor(i.statut));
        const bool late = lateIds.contains(i.id);
        auto *alerte = makeItem(late ? tr("En retard") : QString(), late ? 1 : 0);
        alerte->setForeground(QColor("#CC4E4F"));
        alerte->setFont(bold);

        m_model->appendRow({id,
                            makeItem(i.type, i.type),
                            makeItem(i.zoneNom, i.zoneNom),
                            prio,
                            statut,
                            makeItem(i.debut.toString(kDateFmt), i.debut.toString(Qt::ISODate)),
                            makeItem(i.fin.toString(kDateFmt), i.fin.toString(Qt::ISODate)),
                            makeItem(i.responsable, i.responsable),
                            makeItem(i.idEmploi > 0 ? QStringLiteral("#%1").arg(i.idEmploi) : QStringLiteral("—"), i.idEmploi),
                            alerte});
    }

    m_table->setSortingEnabled(true);
    m_table->sortByColumn(sortSection, sortOrder);
}

void InterventionWidget::updateKpis(const QVector<Intervention> &late)
{
    const QVector<Intervention> all = InterventionService::list();
    int enCours = 0, critiques = 0;
    for (const Intervention &i : all) {
        if (i.statut == Statut::EnCours) ++enCours;
        const bool active = i.statut == Statut::Planifiee || i.statut == Statut::EnCours;
        if (active && i.priorite == Priorite::Critique) ++critiques;
    }
    m_kTotal->setText(QString::number(all.size()));
    m_kEnCours->setText(QString::number(enCours));
    m_kRetard->setText(QString::number(late.size()));
    m_kCritiques->setText(QString::number(critiques));
}

void InterventionWidget::refreshStats()
{
    if (!m_sDim) return;
    InterventionFilter f;
    f.periode = m_sPeriode->isChecked();
    f.du = m_sDu->date();
    f.au = m_sAu->date();

    const Counts counts = InterventionService::repartition(m_sDim->currentData().toString(), f);
    int total = 0;
    for (const auto &c : counts)
        total += c.second;
    m_chart->setData(counts, tr("Interventions par %1").arg(m_sDim->currentText().toLower()));
    m_sTotal->setText(tr("%1 intervention(s)").arg(total));
}

// ------------------------------------------------------------------ CRUD

void InterventionWidget::onAdd()
{
    InterventionDialog dlg(Intervention(), true, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    Intervention i = dlg.result();
    QString err;
    if (!InterventionService::add(i, &err)) {
        QMessageBox::critical(this, tr("Création impossible"), err);
        return;
    }
    notify(tr("Intervention #%1 créée.").arg(i.id));
    refresh();
}

void InterventionWidget::onEdit()
{
    const int id = selectedId();
    if (!id) return;
    const auto cur = InterventionService::get(id);
    if (!cur) return;

    InterventionDialog dlg(*cur, false, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    const Intervention r = dlg.result();

    // Réouverture d'une intervention terminée : justification obligatoire
    QString justification;
    if (cur->statut == Statut::Terminee && r.statut != Statut::Terminee) {
        bool ok = false;
        justification = QInputDialog::getText(this, tr("Réouverture"),
                                              tr("Justification de la réouverture :"), QLineEdit::Normal, QString(), &ok);
        if (!ok) return;
    }
    // Démarrage : avertir si les prérequis ne sont pas réunis (mission « non prête »)
    if (cur->statut == Statut::Planifiee && r.statut == Statut::EnCours) {
        const QStringList manques = InterventionService::prerequis(r);
        if (!manques.isEmpty()
            && QMessageBox::question(this, tr("Mission non prête"),
                   tr("Les prérequis ne sont pas réunis :\n\n• %1\n\nDémarrer quand même ?")
                       .arg(manques.join(QStringLiteral("\n• ")))) != QMessageBox::Yes)
            return;
    }

    QString err;
    if (!InterventionService::update(r, justification, &err)) {
        QMessageBox::critical(this, tr("Modification impossible"), err);
        return;
    }
    notify(tr("Intervention #%1 mise à jour.").arg(id));
    refresh();
}

void InterventionWidget::onDelete()
{
    const int id = selectedId();
    if (!id) return;
    if (QMessageBox::question(this, tr("Confirmation"),
            tr("Supprimer définitivement l'intervention #%1 ?").arg(id)) != QMessageBox::Yes)
        return;
    QString err;
    if (!InterventionService::remove(id, &err)) {
        QMessageBox::warning(this, tr("Suppression refusée"), err);
        return;
    }
    notify(tr("Intervention #%1 supprimée.").arg(id));
    refresh();
}

void InterventionWidget::onCancel()
{
    const int id = selectedId();
    if (!id) return;
    if (QMessageBox::question(this, tr("Confirmation"),
            tr("Annuler l'intervention #%1 ? Les équipements affectés seront libérés.").arg(id)) != QMessageBox::Yes)
        return;
    QString err;
    if (!InterventionService::cancel(id, &err)) {
        QMessageBox::warning(this, tr("Annulation impossible"), err);
        return;
    }
    notify(tr("Intervention #%1 annulée.").arg(id));
    refresh();
}

void InterventionWidget::onClose()
{
    const int id = selectedId();
    if (!id) return;
    const auto cur = InterventionService::get(id);
    if (!cur) return;

    QString compteRendu = cur->compteRendu;
    QDateTime fin = QDateTime::currentDateTime();
    fin.setTime(QTime(fin.time().hour(), fin.time().minute()));

    while (true) {
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Clôturer l'intervention #%1").arg(id));
        dlg.setMinimumWidth(460);
        auto *cr = new QPlainTextEdit(compteRendu);
        cr->setPlaceholderText(tr("Compte-rendu (20 caractères minimum)"));
        auto *dte = new QDateTimeEdit(fin);
        dte->setCalendarPopup(true);
        dte->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
        auto *form = new QFormLayout;
        form->addRow(tr("Fin réelle"), dte);
        form->addRow(tr("Compte-rendu *"), cr);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        auto *lay = new QVBoxLayout(&dlg);
        lay->addLayout(form);
        lay->addWidget(bb);

        if (dlg.exec() != QDialog::Accepted)
            return;
        compteRendu = cr->toPlainText().trimmed();
        fin = dte->dateTime();

        QString err;
        if (InterventionService::cloturer(id, compteRendu, fin, &err)) {
            notify(tr("Intervention #%1 clôturée.").arg(id));
            refresh();
            return;
        }
        QMessageBox::warning(this, tr("Clôture refusée"), err);   // puis on rouvre le formulaire
    }
}

// ------------------------------------------------------------------ métiers avancés

void InterventionWidget::onRecalcPriority()
{
    const int id = selectedId();
    if (!id) return;
    const PriorityResult r = InterventionService::recalculerPriorite(id, false);
    if (!r.changee) {
        QMessageBox::information(this, tr("Recalcul de priorité"),
            tr("Priorité inchangée (%1).\n\n%2").arg(r.ancienne, r.explication));
        return;
    }
    if (QMessageBox::question(this, tr("Recalcul de priorité"),
            tr("Priorité suggérée : %1 → %2\n\n%3\n\nAppliquer ?").arg(r.ancienne, r.nouvelle, r.explication))
        == QMessageBox::Yes) {
        InterventionService::recalculerPriorite(id, true);
        notify(tr("Priorité de l'intervention #%1 ajustée : %2.").arg(id).arg(r.nouvelle));
        refresh();
    }
}

void InterventionWidget::onPrerequisites()
{
    const int id = selectedId();
    if (!id) return;
    const auto cur = InterventionService::get(id);
    if (!cur) return;
    const QStringList manques = InterventionService::prerequis(*cur);
    if (manques.isEmpty())
        QMessageBox::information(this, tr("Prérequis"), tr("Mission PRÊTE : emploi valide et équipements disponibles."));
    else
        QMessageBox::warning(this, tr("Prérequis"),
            tr("Mission NON PRÊTE :\n\n• %1").arg(manques.join(QStringLiteral("\n• "))));
}

void InterventionWidget::onEstimate()
{
    const int id = selectedId();
    if (!id) return;
    const auto cur = InterventionService::get(id);
    if (!cur) return;
    const DurationEstimate d = InterventionService::estimerDuree(cur->type, cur->idZone);
    if (d.heures < 0)
        QMessageBox::information(this, tr("Estimation de durée"), tr("Pas assez d'historique pour estimer la durée."));
    else
        QMessageBox::information(this, tr("Estimation de durée"),
            tr("Durée estimée : %1\n\nBase : %2 (%3 intervention(s) terminée(s)).")
                .arg(fmtHeures(d.heures), d.base).arg(d.echantillon));
}

void InterventionWidget::onLate()
{
    const QVector<Intervention> late = InterventionService::interventionsEnRetard();
    if (late.isEmpty()) {
        QMessageBox::information(this, tr("Escalade de retard"), tr("Aucune intervention en retard."));
        return;
    }
    const QDateTime now = QDateTime::currentDateTime();
    QStringList lines;
    for (const Intervention &i : late) {
        const qint64 h = i.fin.secsTo(now) / 3600;
        lines << tr("#%1 %2 — %3 — fin prévue %4 (%5 h de retard)")
                     .arg(i.id).arg(i.type, i.zoneNom, i.fin.toString(kDateFmt)).arg(h);
    }
    QMessageBox::warning(this, tr("Escalade de retard"),
        tr("%1 intervention(s) en dépassement de délai — à signaler au responsable :\n\n%2")
            .arg(late.size()).arg(lines.join(QLatin1Char('\n'))));
}

void InterventionWidget::onHistory()
{
    const int id = selectedId();
    if (!id) return;
    const QVector<HistoryEntry> h = InterventionService::historique(id);

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Historique de l'intervention #%1").arg(id));
    dlg.resize(760, 360);
    auto *table = new QTableWidget(int(h.size()), 3, &dlg);
    table->setHorizontalHeaderLabels({tr("Date"), tr("Action"), tr("Détail")});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();
    for (int r = 0; r < h.size(); ++r) {
        table->setItem(r, 0, new QTableWidgetItem(h[r].date.toString(kDateFmt)));
        table->setItem(r, 1, new QTableWidgetItem(h[r].action));
        table->setItem(r, 2, new QTableWidgetItem(h[r].detail));
    }
    table->resizeColumnsToContents();
    auto *lay = new QVBoxLayout(&dlg);
    lay->addWidget(h.isEmpty() ? static_cast<QWidget *>(new QLabel(tr("Aucun événement enregistré."))) : table);
    dlg.exec();
}

// ------------------------------------------------------------------ exports

void InterventionWidget::exportCsv()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Exporter en CSV"),
                                                      QStringLiteral("interventions.csv"), tr("Fichier CSV (*.csv)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export"), tr("Impossible d'écrire le fichier : %1").arg(f.errorString()));
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out.setGenerateByteOrderMark(true);   // accents corrects dans Excel
    out << "N°;Type;Zone;Priorité;Statut;Début;Fin;Responsable;Emploi;Description;Compte-rendu\n";
    for (const Intervention &i : std::as_const(m_shown)) {
        out << i.id << ';' << csvCell(i.type) << ';' << csvCell(i.zoneNom) << ';' << csvCell(i.priorite) << ';'
            << csvCell(i.statut) << ';' << i.debut.toString(kDateFmt) << ';' << i.fin.toString(kDateFmt) << ';'
            << csvCell(i.responsable) << ';' << (i.idEmploi > 0 ? QString::number(i.idEmploi) : QString()) << ';'
            << csvCell(i.description) << ';' << csvCell(i.compteRendu) << '\n';
    }
    notify(tr("%1 intervention(s) exportée(s) en CSV.").arg(m_shown.size()));
}

void InterventionWidget::exportPdf()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Exporter en PDF"),
                                                      QStringLiteral("interventions.pdf"), tr("Document PDF (*.pdf)"));
    if (path.isEmpty()) return;

    QString html = QStringLiteral(
        "<h2 style='color:#162B3E'>Smart Weather Management — Interventions</h2>"
        "<p>Édité le %1 — %2 intervention(s)</p>"
        "<table border='1' cellspacing='0' cellpadding='4' width='100%'>"
        "<tr style='background-color:#162B3E;color:white'>"
        "<th>N°</th><th>Type</th><th>Zone</th><th>Priorité</th><th>Statut</th>"
        "<th>Début</th><th>Fin</th><th>Responsable</th></tr>")
        .arg(QDateTime::currentDateTime().toString(kDateFmt)).arg(m_shown.size());
    for (const Intervention &i : std::as_const(m_shown)) {
        html += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td></tr>")
                    .arg(i.id)
                    .arg(i.type.toHtmlEscaped(), i.zoneNom.toHtmlEscaped(), i.priorite.toHtmlEscaped(), i.statut.toHtmlEscaped(),
                         i.debut.toString(kDateFmt), i.fin.toString(kDateFmt), i.responsable.toHtmlEscaped());
    }
    html += QStringLiteral("</table>");

    QTextDocument doc;
    doc.setHtml(html);
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageMargins(QMarginsF(10, 10, 10, 10), QPageLayout::Millimeter);
    doc.print(&printer);
    notify(tr("%1 intervention(s) exportée(s) en PDF.").arg(m_shown.size()));
}
