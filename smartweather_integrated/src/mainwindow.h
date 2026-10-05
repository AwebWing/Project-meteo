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

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

public slots:
    void navigateToTab(int index);

private:
    QWidget *buildSidebar();
    QWidget *buildHeader();
    void updateClock();

    ZoneRepository      *m_zoneRepo{nullptr};
    EquipmentRepository *m_equipmentRepo{nullptr};

    AccueilPage         *m_accueilPage{nullptr};
    EmploiPage          *m_emploiPage{nullptr};
    ZonesModule         *m_zonesModule{nullptr};
    InterventionWidget  *m_interventionWidget{nullptr};
    EquipmentPage       *m_equipmentPage{nullptr};

    QStackedWidget      *m_stack{nullptr};
    QButtonGroup        *m_navGroup{nullptr};
    QLabel              *m_date{nullptr};
    QLabel              *m_time{nullptr};
};
