#pragma once
#include <QWidget>
#include <QList>
#include <QString>

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QButtonGroup;
class QChart;
class QChartView;
class QPieSeries;

struct AgentEmploi {
    int id;
    QString nom;
    QString prenom;
    QString role;
    QString statut;        // "En service", "Disponible", "En congé", "Formation"
    QString zoneAffectee;
    QString quartTravail;  // "Matin (06h-14h)", "Après-midi (14h-22h)", "Nuit (22h-06h)"
};

class EmploiPage : public QWidget
{
    Q_OBJECT
public:
    explicit EmploiPage(QWidget *parent = nullptr);

private slots:
    void refresh();
    void applyFilters();
    void onAddAgent();
    void onEditAgent();
    void onDeleteAgent();

private:
    void initSampleData();
    int currentId() const;

    QList<AgentEmploi> m_agents;
    int m_nextId = 1;

    // KPIs
    QLabel *m_kpiTotal{nullptr};
    QLabel *m_kpiService{nullptr};
    QLabel *m_kpiDispo{nullptr};
    QLabel *m_kpiConge{nullptr};

    // Sidebar role filter
    QButtonGroup *m_roleGroup{nullptr};

    // Search & Statut Filter
    QLineEdit *m_search{nullptr};
    QComboBox *m_statutFilter{nullptr};

    // Table
    QTableView *m_table{nullptr};
    QStandardItemModel *m_model{nullptr};

    // Action buttons
    QPushButton *m_btnEdit{nullptr};
    QPushButton *m_btnDelete{nullptr};

    // Donut Chart
    QPieSeries *m_pieSeries{nullptr};
    QChartView *m_chartView{nullptr};
    QLabel *m_chartCenter{nullptr};

    // Details panel
    QLabel *m_detailTitle{nullptr};
    QLabel *m_detailBody{nullptr};
};
