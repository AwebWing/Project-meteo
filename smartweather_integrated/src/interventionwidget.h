#pragma once
#include "interventionservice.h"
#include <QSet>
#include <QWidget>

class BarChartWidget;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QStandardItemModel;
class QTableView;

// Espace « Gestion des interventions » (Bader Eddine) :
// titre, indicateurs, filtres, tableau, actions, messages, historique + onglet statistiques.
class InterventionWidget : public QWidget {
    Q_OBJECT
public:
    explicit InterventionWidget(QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onCancel();
    void onClose();
    void onRecalcPriority();
    void onPrerequisites();
    void onEstimate();
    void onLate();
    void onHistory();
    void exportCsv();
    void exportPdf();
    void refreshStats();

private:
    void buildUi();
    InterventionFilter currentFilter() const;
    int selectedId();
    void populateTable(const QSet<int> &lateIds);
    void updateKpis(const QVector<Intervention> &late);
    void notify(const QString &text);

    // liste
    QLineEdit *m_search = nullptr;
    QComboBox *m_fStatut = nullptr, *m_fPriorite = nullptr, *m_fType = nullptr, *m_fZone = nullptr;
    QCheckBox *m_fPeriode = nullptr;
    QDateEdit *m_fDu = nullptr, *m_fAu = nullptr;
    QTableView *m_table = nullptr;
    QStandardItemModel *m_model = nullptr;
    QLabel *m_msg = nullptr;
    QLabel *m_kTotal = nullptr, *m_kEnCours = nullptr, *m_kRetard = nullptr, *m_kCritiques = nullptr;

    // statistiques
    QComboBox *m_sDim = nullptr;
    QCheckBox *m_sPeriode = nullptr;
    QDateEdit *m_sDu = nullptr, *m_sAu = nullptr;
    BarChartWidget *m_chart = nullptr;
    QLabel *m_sTotal = nullptr;

    QVector<Intervention> m_shown;   // lignes affichées = lignes exportées
};
