#pragma once
#include <QMainWindow>
#include <QSqlQueryModel>

class QSortFilterProxyModel;

QT_BEGIN_NAMESPACE
namespace Ui { class InterventionsWindow; }
QT_END_NAMESPACE

class InterventionsWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit InterventionsWindow(QWidget *parent = nullptr);
    ~InterventionsWindow() override;

public slots:
    void refresh();
    void onAjouter();

private slots:
    void resetFilters();
    void onModifier();
    void onSupprimer();
    void onCloturer();

    void onPriorite();
    void onPrerequis();
    void onDuree();
    void onEquipement();
    void onRetards();
    void onHistorique();

    void exportCsv();
    void exportPdf();

private:
    int selectedId(bool warn = true);
    void populateFilters();

    Ui::InterventionsWindow *ui;
    QSqlQueryModel m_model;
    QSortFilterProxyModel *m_proxy;
};
