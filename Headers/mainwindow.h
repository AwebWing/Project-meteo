#pragma once
#include <QMainWindow>

class QLabel;
class QStackedWidget;
class QButtonGroup;
class ZoneRepository;
class EquipmentRepository;
class InterventionWidget;
class EmploiPage;
class AccueilPage;
class ZonesModule;
class EquipmentPage;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void navigateToTab(int index);

private:
    void updateClock();

    Ui::MainWindow *ui;
    QButtonGroup       *m_navGroup{nullptr};
    ZoneRepository      *m_zoneRepo{nullptr};
    EquipmentRepository *m_equipmentRepo{nullptr};

    AccueilPage         *m_accueilPage{nullptr};
    EmploiPage          *m_emploiPage{nullptr};
    ZonesModule         *m_zonesModule{nullptr};
    InterventionWidget  *m_interventionWidget{nullptr};
    EquipmentPage       *m_equipmentPage{nullptr};
};
