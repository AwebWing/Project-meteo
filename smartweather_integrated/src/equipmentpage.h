#pragma once
#include <QWidget>
#include "equipment.h"

class QButtonGroup;
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QStandardItemModel;
class QTableView;
class EquipmentRepository;

class QChart;
class QChartView;
class QPieSeries;

// ---------------------------------------------------------------
//  EquipmentPage : interface principale de gestion des équipements
//    - KPI row (Total / Disponibles / En maintenance / Hors service)
//    - Panneau catégories (gauche)
//    - Tableau CRUD avec recherche + filtre statut (centre)
//    - Graphique état du parc donut (droite)
// ---------------------------------------------------------------
class EquipmentPage : public QWidget
{
    Q_OBJECT
public:
    explicit EquipmentPage(EquipmentRepository *repo, QWidget *parent = nullptr);

private slots:
    void refresh();
    void applyFilters();
    void updateDetail();
    void onAdd();
    void onEdit();
    void onDelete();

private:
    int  currentId() const;   // id de la ligne sélectionnée (-1 si aucune)
    void selectById(int id);

    EquipmentRepository *m_repo;

    // KPIs
    QLabel *m_kpiTotal, *m_kpiDispo, *m_kpiMaint, *m_kpiHs;

    // Catégories
    QButtonGroup *m_catGroup;

    // Filtres / recherche
    QLineEdit *m_search;
    QComboBox *m_statutFilter;

    // Tableau
    QTableView         *m_table;
    QStandardItemModel *m_model;

    // Actions
    QPushButton *m_btnEdit, *m_btnDelete;

    // Panneau détail
    QLabel *m_detailTitle, *m_detailBody;

    // Graphique
    QPieSeries  *m_pieSeries{nullptr};
    QChartView  *m_chartView{nullptr};
    QLabel                *m_chartCenter; // label superposé au centre du donut
    QLabel                *m_legendDispo, *m_legendMaint, *m_legendHs;
};
