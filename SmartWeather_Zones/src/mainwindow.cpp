#include "mainwindow.h"
#include <QButtonGroup>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include "uihelpers.h"
#include "zonerepository.h"
#include "zonesmodule.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Smart Weather Management"));
    resize(1440, 880);
    setMinimumSize(1180, 720);

    m_repo = new ZoneRepository(this);

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

    // Pages : l'ordre correspond aux boutons de la barre laterale
    m_stack->addWidget(placeholder(QStringLiteral("Accueil"), QStringLiteral("Tableau de bord général")));
    m_stack->addWidget(placeholder(QStringLiteral("Gestion d'emploi"), QStringLiteral("Yasmine Kouki")));
    m_stack->addWidget(new ZonesModule(m_repo));                                   // <-- NOTRE PARTIE (Adem)
    m_stack->addWidget(placeholder(QStringLiteral("Interventions"), QStringLiteral("Bader Eddine")));
    m_stack->addWidget(placeholder(QStringLiteral("Équipements"), QStringLiteral("Aweb Ghandour")));
    m_stack->setCurrentIndex(2);

    updateClock();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateClock);
    timer->start(1000);
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

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    for (int i = 0; i < labels.size(); ++i) {
        auto *b = new QPushButton(labels[i]);
        b->setObjectName("nav");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        if (i == 2) b->setChecked(true);
        group->addButton(b, i);
        v->addWidget(b);
    }
    v->addStretch(1);

    connect(group, &QButtonGroup::idClicked, this, [this](int id) { m_stack->setCurrentIndex(id); });
    return side;
}

QWidget *MainWindow::buildHeader()
{
    auto *bar = new QFrame;
    bar->setObjectName("header");
    auto *h = new QHBoxLayout(bar);
    h->setContentsMargins(20, 10, 20, 10);
    h->setSpacing(26);

    // Meteo (valeurs d'exemple : elles viendront plus tard des releves)
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
    auto *user = new QLabel(QStringLiteral("<b>Adem Nouioui</b><br>"
                                           "<span style='color:#19c6b7; font-size:11px'>Responsable Stations &amp; Zones</span>"));
    h->addWidget(user);
    return bar;
}

QWidget *MainWindow::placeholder(const QString &title, const QString &owner)
{
    QVBoxLayout *l = nullptr;
    QFrame *card = makeCard(l);
    auto *t = new QLabel(title);
    t->setObjectName("title");
    t->setAlignment(Qt::AlignCenter);
    auto *s = new QLabel(QStringLiteral("Module à intégrer — %1").arg(owner));
    s->setObjectName("muted");
    s->setAlignment(Qt::AlignCenter);
    l->addStretch(1);
    l->addWidget(t);
    l->addWidget(s);
    l->addStretch(1);
    return card;
}

void MainWindow::updateClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QLocale fr(QLocale::French);
    m_date->setText(fr.toString(now, QStringLiteral("ddd dd MMM yyyy")));
    m_time->setText(now.toString(QStringLiteral("HH:mm")));
}
