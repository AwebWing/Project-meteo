#include "mainwindow.h"
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
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMetaObject>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Smart Weather Management — Application Intégrée"));
    resize(1440, 880);
    setMinimumSize(1180, 720);

    m_zoneRepo = new ZoneRepository(this);
    m_equipmentRepo = new EquipmentRepository(this);

    auto *root = new QWidget;
    root->setObjectName("root");
    setCentralWidget(root);

    auto *h = new QHBoxLayout(root);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    h->addWidget(buildSidebar());

    auto *right = new QVBoxLayout;
    right->setContentsMargins(18, 16, 18, 16);
    right->setSpacing(14);
    right->addWidget(buildHeader());

    m_stack = new QStackedWidget;
    right->addWidget(m_stack, 1);
    h->addLayout(right, 1);

    // Instanciation des 5 modules intégrés
    m_accueilPage = new AccueilPage(this);
    m_emploiPage = new EmploiPage(this);
    m_zonesModule = new ZonesModule(m_zoneRepo, this);
    m_interventionWidget = new InterventionWidget(this);
    m_equipmentPage = new EquipmentPage(m_equipmentRepo, this);

    // Ajout au QStackedWidget dans l'ordre de la barre latérale
    m_stack->addWidget(m_accueilPage);          // Index 0: Accueil
    m_stack->addWidget(m_emploiPage);           // Index 1: Gestion d'emploi (Yasmine Kouki)
    m_stack->addWidget(m_zonesModule);          // Index 2: Stations & Zones (Adem Nouioui)
    m_stack->addWidget(m_interventionWidget);   // Index 3: Interventions (Bader Eddine)
    m_stack->addWidget(m_equipmentPage);        // Index 4: Équipements (Aweb Ghandour)

    m_stack->setCurrentIndex(0); // Start on Accueil dashboard

    // Actions Rapides de la page Accueil
    connect(m_accueilPage, &AccueilPage::actionNewZoneRequested, this, [this]() {
        navigateToTab(2); // Stations & Zones
    });

    connect(m_accueilPage, &AccueilPage::actionNewInterventionRequested, this, [this]() {
        navigateToTab(3); // Interventions
        if (m_interventionWidget) {
            QMetaObject::invokeMethod(m_interventionWidget, "onAdd", Qt::QueuedConnection);
        }
    });

    connect(m_accueilPage, &AccueilPage::actionNewEquipmentRequested, this, [this]() {
        navigateToTab(4); // Équipements
        if (m_equipmentPage) {
            QMetaObject::invokeMethod(m_equipmentPage, "onAdd", Qt::QueuedConnection);
        }
    });

    connect(m_accueilPage, &AccueilPage::actionNewAgentRequested, this, [this]() {
        navigateToTab(1); // Gestion d'emploi
        if (m_emploiPage) {
            QMetaObject::invokeMethod(m_emploiPage, "onAddAgent", Qt::QueuedConnection);
        }
    });

    updateClock();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateClock);
    timer->start(1000);
}

void MainWindow::navigateToTab(int index)
{
    if (m_navGroup) {
        if (auto *btn = m_navGroup->button(index)) {
            btn->setChecked(true);
        }
    }
    m_stack->setCurrentIndex(index);
    if (index == 3 && m_interventionWidget) {
        m_interventionWidget->refresh();
    }
}

QWidget *MainWindow::buildSidebar()
{
    auto *side = new QFrame;
    side->setObjectName("sidebar");
    side->setFixedWidth(230);

    auto *v = new QVBoxLayout(side);
    v->setContentsMargins(14, 18, 14, 18);
    v->setSpacing(6);

    auto *logo = new QLabel;
    logo->setPixmap(QPixmap(QStringLiteral(":/logo.png")).scaledToHeight(120, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);
    v->addWidget(logo);
    v->addSpacing(14);

    const QStringList labels = {
        QStringLiteral("🏠   Accueil"),
        QStringLiteral("📅   Gestion d'emploi"),
        QStringLiteral("📡   Stations & Zones"),
        QStringLiteral("⚠   Interventions"),
        QStringLiteral("🧰   Équipements")};

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);
    for (int i = 0; i < labels.size(); ++i) {
        auto *b = new QPushButton(labels[i]);
        b->setObjectName("nav");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        if (i == 0) b->setChecked(true);
        m_navGroup->addButton(b, i);
        v->addWidget(b);
    }
    v->addStretch(1);

    connect(m_navGroup, &QButtonGroup::idClicked, this, &MainWindow::navigateToTab);
    return side;
}

QWidget *MainWindow::buildHeader()
{
    auto *bar = new QFrame;
    bar->setObjectName("header");
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(20, 10, 20, 10);
    h->setSpacing(26);

    auto *sun = new QLabel(QStringLiteral("⛅"));
    sun->setStyleSheet("font-size: 34px;");
    auto *temp = new QLabel(QStringLiteral("<b style='font-size:20px'>22°C</b><br>"
                                           "<span style='color:#8fb3c2'>Partiellement nuageux — Tunis</span>"));
    h->addWidget(sun);
    h->addWidget(temp);
    h->addWidget(new QLabel(QStringLiteral("💧 Humidité<br><b>68 %</b>")));
    h->addWidget(new QLabel(QStringLiteral("🌬 Vent<br><b>18 km/h</b>")));
    h->addWidget(new QLabel(QStringLiteral("🌧 Précipitations<br><b>0 mm</b>")));
    h->addStretch(1);

    m_date = new QLabel;
    m_time = new QLabel;
    m_time->setStyleSheet("font-size: 18px; font-weight: 600;");
    auto *clock = new QVBoxLayout;
    clock->setSpacing(0);
    clock->addWidget(m_date);
    clock->addWidget(m_time);
    h->addLayout(clock);

    h->addWidget(new QLabel(QStringLiteral("🔔")));
    auto *user = new QLabel(QStringLiteral("<b>Équipe SmartWeather</b><br>"
                                           "<span style='color:#19c6b7; font-size:11px'>Adem • Aweb • Bader • Yasmine</span>"));
    h->addWidget(user);
    return bar;
}

void MainWindow::updateClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QLocale fr(QLocale::French);
    m_date->setText(fr.toString(now, QStringLiteral("ddd dd MMM yyyy")));
    m_time->setText(now.toString(QStringLiteral("HH:mm")));
}
