#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "accueilpage.h"
#include "emploipage.h"
#include "equipmentpage.h"
#include "equipmentrepository.h"
#include "interventionwidget.h"
#include "uihelpers.h"
#include "zonerepository.h"
#include "zonesmodule.h"

#include <QButtonGroup>
#include <QDateTime>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("SmartWeather Management — Application Intégrée"));
    resize(1440, 880);
    setMinimumSize(1180, 720);

    // Set the logo from resources
    ui->logoLabel->setPixmap(QPixmap(":/logo.png").scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->logoLabel->setAlignment(Qt::AlignCenter);

    m_zoneRepo = new ZoneRepository(this);

    m_equipmentRepo = new EquipmentRepository(this);

    // 1. Initialize Modules
    m_accueilPage = new AccueilPage(this);
    m_emploiPage = new EmploiPage(this);
    m_zonesModule = new ZonesModule(m_zoneRepo, this);
    m_interventionWidget = new InterventionWidget(this);
    m_equipmentPage = new EquipmentPage(m_equipmentRepo, this);

    // 2. Add to StackedWidget (mapped to .ui file)
    ui->m_stack->addWidget(m_accueilPage);          // Index 0
    ui->m_stack->addWidget(m_emploiPage);           // Index 1
    ui->m_stack->addWidget(m_zonesModule);          // Index 2
    ui->m_stack->addWidget(m_interventionWidget);   // Index 3
    ui->m_stack->addWidget(m_equipmentPage);        // Index 4

    ui->m_stack->setCurrentIndex(0);

    // 3. Navigation Logic (using .ui buttons)
    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);
    m_navGroup->addButton(ui->btn_accueil, 0);
    m_navGroup->addButton(ui->btn_emploi, 1);
    m_navGroup->addButton(ui->btn_zones, 2);
    m_navGroup->addButton(ui->btn_interventions, 3);
    m_navGroup->addButton(ui->btn_equipements, 4);

    ui->btn_accueil->setChecked(true);

    connect(m_navGroup, &QButtonGroup::idClicked, this, &MainWindow::navigateToTab);

    // 4. Cross-Module Actions
    connect(m_accueilPage, &AccueilPage::actionNewZoneRequested, this, [this]() { navigateToTab(2); });
    connect(m_accueilPage, &AccueilPage::actionNewInterventionRequested, this, [this]() {
        navigateToTab(3);
        if (m_interventionWidget) m_interventionWidget->refresh();
    });
    connect(m_accueilPage, &AccueilPage::actionNewEquipmentRequested, this, [this]() { navigateToTab(4); });
    connect(m_accueilPage, &AccueilPage::actionNewAgentRequested, this, [this]() { navigateToTab(1); });

    // 5. Clock
    updateClock();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateClock);
    timer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::navigateToTab(int index)
{
    if (m_navGroup) {
        if (auto *btn = m_navGroup->button(index)) btn->setChecked(true);
    }
    ui->m_stack->setCurrentIndex(index);

    // Auto-refresh interventions when navigating to that tab
    if (index == 3 && m_interventionWidget) {
        m_interventionWidget->refresh();
    }
}

void MainWindow::updateClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    ui->lbl_date->setText(now.toString("ddd dd MMM yyyy"));
    ui->lbl_time->setText(now.toString("HH:mm"));
}
