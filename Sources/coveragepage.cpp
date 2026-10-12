#include "coveragepage.h"

#include <QBarCategoryAxis>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLegend>
#include <QPieSeries>
#include <QPieSlice>
#include <QStackedBarSeries>
#include <QTableWidget>
#include <QValueAxis>
#include <QVBoxLayout>

#include "theme.h"
#include "uihelpers.h"
#include "zone.h"
#include "zonerepository.h"
#include "zoneservice.h"

namespace {

QTableWidget *makeTable(const QStringList &headers)
{
    auto *t = new QTableWidget(0, headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->verticalHeader()->hide();
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setAlternatingRowColors(true);
    t->setShowGrid(false);
    t->verticalHeader()->setDefaultSectionSize(34);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    return t;
}

// Enleve les anciennes series/axes avant de reconstruire un graphique
void clearChart(QChart *chart)
{
    chart->removeAllSeries();
    const auto axes = chart->axes();
    for (QAbstractAxis *a : axes) {
        chart->removeAxis(a);
        delete a;
    }
}

QChartView *makeChartView(const QString &title)
{
    auto *chart = new QChart;
    chart->setTitle(title);
    chart->setTitleBrush(QBrush(Theme::textColor()));
    chart->setBackgroundVisible(false);
    chart->legend()->setLabelColor(Theme::textColor());
    chart->legend()->setAlignment(Qt::AlignBottom);
    auto *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setBackgroundBrush(Qt::transparent);
    view->setStyleSheet("background: transparent; border: none;");
    return view;
}

QTableWidgetItem *coloredItem(const QString &text, const QColor &color)
{
    auto *it = new QTableWidgetItem(text);
    it->setForeground(color);
    return it;
}

} // namespace

CoveragePage::CoveragePage(ZoneRepository *repo, QWidget *parent) : QWidget(parent), m_repo(repo)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // ---- Indicateurs ----
    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(14);
    kpis->addWidget(makeKpiCard(QStringLiteral("📶"), QStringLiteral("Couverture réseau"), m_kpiCouverture));
    kpis->addWidget(makeKpiCard(QStringLiteral("⚠"), QStringLiteral("Zones sous-couvertes"), m_kpiSous));
    kpis->addWidget(makeKpiCard(QStringLiteral("🎯"), QStringLiteral("Zone prioritaire n°1"), m_kpiTop));
    root->addLayout(kpis);

    // ---- Grille 2 x 2 ----
    auto *grid = new QGridLayout;
    grid->setSpacing(14);

    QVBoxLayout *l1 = nullptr;
    QFrame *c1 = makeCard(l1);
    auto *t1 = new QLabel(QStringLiteral("Couverture par zone (stations présentes / attendues)"));
    t1->setObjectName("cardTitle");
    m_coverageTable = makeTable({QStringLiteral("Zone"), QStringLiteral("Stations"),
                                 QStringLiteral("Attendues"), QStringLiteral("État")});
    l1->addWidget(t1);
    l1->addWidget(m_coverageTable, 1);

    QVBoxLayout *l2 = nullptr;
    QFrame *c2 = makeCard(l2);
    auto *t2 = new QLabel(QStringLiteral("Priorité territoriale (classement)"));
    t2->setObjectName("cardTitle");
    m_rankTable = makeTable({QStringLiteral("Rang"), QStringLiteral("Zone"),
                             QStringLiteral("Score priorité"), QStringLiteral("Niveau")});
    l2->addWidget(t2);
    l2->addWidget(m_rankTable, 1);

    QVBoxLayout *l3 = nullptr;
    QFrame *c3 = makeCard(l3);
    m_barView = makeChartView(QStringLiteral("Score de vulnérabilité par zone"));
    l3->addWidget(m_barView);

    QVBoxLayout *l4 = nullptr;
    QFrame *c4 = makeCard(l4);
    m_pieView = makeChartView(QStringLiteral("Répartition par niveau"));
    l4->addWidget(m_pieView);

    grid->addWidget(c1, 0, 0);
    grid->addWidget(c2, 0, 1);
    grid->addWidget(c3, 1, 0);
    grid->addWidget(c4, 1, 1);
    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 1);
    root->addLayout(grid, 1);

    connect(m_repo, &ZoneRepository::changed, this, &CoveragePage::refresh);
    refresh();
}

void CoveragePage::refresh()
{
    const QList<Zone> zones = m_repo->all();
    QList<Zone> actives;
    for (const Zone &z : zones)
        if (z.statut == QLatin1String("Active")) actives << z;

    // ---- Tableau couverture (metier 3) ----
    int nbSous = 0;
    m_coverageTable->setRowCount(actives.size());
    for (int i = 0; i < actives.size(); ++i) {
        const Zone &z = actives[i];
        const bool sous = ZoneService::sousCouverture(z);
        if (sous) ++nbSous;
        m_coverageTable->setItem(i, 0, new QTableWidgetItem(z.nom));
        m_coverageTable->setItem(i, 1, new QTableWidgetItem(QString::number(z.nbStations)));
        m_coverageTable->setItem(i, 2, new QTableWidgetItem(QString::number(ZoneService::stationsAttendues(z))));
        m_coverageTable->setItem(i, 3, coloredItem(sous ? QStringLiteral("Insuffisante") : QStringLiteral("Suffisante"),
                                                   Theme::levelColor(sous ? QStringLiteral("Insuffisante") : QStringLiteral("OK"))));
    }

    // ---- Classement priorite (metier 2) ----
    const QList<Zone> ranking = ZoneService::classementPriorite(actives);
    m_rankTable->setRowCount(ranking.size());
    for (int i = 0; i < ranking.size(); ++i) {
        const Zone &z = ranking[i];
        const QString niv = ZoneService::niveau(z.vulnerabilite);
        m_rankTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_rankTable->setItem(i, 1, new QTableWidgetItem(z.nom));
        m_rankTable->setItem(i, 2, new QTableWidgetItem(QString::number(ZoneService::scorePriorite(z))));
        m_rankTable->setItem(i, 3, coloredItem(niv, Theme::levelColor(niv)));
    }

    // ---- Indicateurs ----
    m_kpiCouverture->setText(QStringLiteral("%1 %").arg(ZoneService::couvertureReseau(zones)));
    m_kpiSous->setText(QString::number(nbSous));
    m_kpiTop->setText(ranking.isEmpty() ? QStringLiteral("—") : ranking.first().nom);

    rebuildBarChart(actives);
    rebuildPieChart(actives);
}

void CoveragePage::rebuildBarChart(const QList<Zone> &zones)
{
    QChart *chart = m_barView->chart();
    clearChart(chart);

    // 3 series (Faible / Moyen / Eleve) empilees => une barre par zone, coloree selon son niveau
    const QStringList niveaux = {QStringLiteral("Faible"), QStringLiteral("Moyen"), QStringLiteral("Élevé")};
    QList<QBarSet *> sets;
    for (const QString &n : niveaux) {
        auto *s = new QBarSet(n);
        s->setColor(Theme::levelColor(n));
        s->setBorderColor(Qt::transparent);
        sets << s;
    }
    QStringList categories;
    for (const Zone &z : zones) {
        categories << z.nom;
        const QString n = ZoneService::niveau(z.vulnerabilite);
        for (int i = 0; i < niveaux.size(); ++i) *sets[i] << (niveaux[i] == n ? z.vulnerabilite : 0.0);
    }

    auto *series = new QStackedBarSeries;
    for (QBarSet *s : sets) series->append(s);
    series->setBarWidth(0.6);
    chart->addSeries(series);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(categories);
    axisX->setLabelsColor(Theme::mutedColor());
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d");
    axisY->setLabelsColor(Theme::mutedColor());
    axisY->setGridLineColor(QColor(255, 255, 255, 28));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
}

void CoveragePage::rebuildPieChart(const QList<Zone> &zones)
{
    QChart *chart = m_pieView->chart();
    clearChart(chart);

    int count[3] = {0, 0, 0};
    const QStringList niveaux = {QStringLiteral("Faible"), QStringLiteral("Moyen"), QStringLiteral("Élevé")};
    for (const Zone &z : zones) {
        const int i = niveaux.indexOf(ZoneService::niveau(z.vulnerabilite));
        if (i >= 0) ++count[i];
    }

    auto *series = new QPieSeries;
    series->setHoleSize(0.45);
    for (int i = 0; i < 3; ++i) {
        if (count[i] == 0) continue;
        QPieSlice *slice = series->append(QStringLiteral("%1 (%2)").arg(niveaux[i]).arg(count[i]), count[i]);
        slice->setColor(Theme::levelColor(niveaux[i]));
        slice->setBorderColor(QColor(12, 35, 51));
        slice->setLabelVisible(true);
        slice->setLabelColor(Theme::textColor());
    }
    chart->addSeries(series);
}
