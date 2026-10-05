#pragma once
#include <QList>
#include <QStringList>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTableView;
class MapWidget;
class ZoneFilterProxy;
class ZoneRepository;

// Interface principale "Zones" : indicateurs, recherche/filtres, tableau CRUD,
// carte, panneau de detail, export PDF / CSV.
class ZonesPage : public QWidget
{
    Q_OBJECT
public:
    explicit ZonesPage(ZoneRepository *repo, QWidget *parent = nullptr);

private slots:
    void refresh();
    void applyFilters();
    void updateDetail();
    void onAdd();
    void onEdit();
    void onDelete();
    void onDetails();
    void exportPdf();
    void exportCsv();

private:
    int  currentZoneId() const;
    void selectZoneById(int id);
    QList<QStringList> visibleRows() const;   // lignes affichees (apres filtre/tri)

    ZoneRepository *m_repo;

    QLabel *m_kpiZones, *m_kpiActives, *m_kpiCouverture, *m_kpiMaj;
    QLineEdit *m_search;
    QComboBox *m_statutBox, *m_niveauBox;
    QTableView *m_table;
    QStandardItemModel *m_model;
    ZoneFilterProxy *m_proxy;
    MapWidget *m_map;
    QLabel *m_detailTitle, *m_detailBody;
    QPushButton *m_btnEdit, *m_btnDelete, *m_btnDetails;
};
