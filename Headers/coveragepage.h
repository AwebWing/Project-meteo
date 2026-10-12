#pragma once
#include <QList>
#include <QWidget>
#include "zone.h"

class QChart;
class QChartView;
class QLabel;
class QTableWidget;
class ZoneRepository;

// Interface "Analyse couverture" : couverture reseau, zones sous-couvertes,
// classement de priorite territoriale et statistiques graphiques.
class CoveragePage : public QWidget
{
    Q_OBJECT
public:
    explicit CoveragePage(ZoneRepository *repo, QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    void rebuildBarChart(const QList<Zone> &zones);
    void rebuildPieChart(const QList<Zone> &zones);

    ZoneRepository *m_repo;
    QLabel *m_kpiCouverture, *m_kpiSous, *m_kpiTop;
    QTableWidget *m_coverageTable, *m_rankTable;
    QChartView *m_barView, *m_pieView;
};
