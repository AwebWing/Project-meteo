#include "accueilpage.h"
#include "theme.h"
#include "uihelpers.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AccueilPage::AccueilPage(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // Title
    auto *tRow = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("🏠  Tableau de bord général"));
    title->setObjectName("title");
    auto *sub = new QLabel(QStringLiteral("Vue d'ensemble du système SmartWeather"));
    sub->setObjectName("muted");
    tRow->addWidget(title);
    tRow->addWidget(sub, 1);
    root->addLayout(tRow);

    // Global KPIs
    auto *kpis = new QHBoxLayout;
    kpis->setSpacing(12);
    QLabel *v1, *v2, *v3, *v4;
    kpis->addWidget(makeKpiCard("📡", "Zones Actives", v1));
    kpis->addWidget(makeKpiCard("👥", "Agents sur Terrain", v2));
    kpis->addWidget(makeKpiCard("⚠", "Interventions en cours", v3));
    kpis->addWidget(makeKpiCard("🧰", "Équipements Dispos", v4));
    v1->setText("4");
    v2->setText("8");
    v3->setText("2");
    v4->setText("14");
    root->addLayout(kpis);

    // 2 Grid cards
    auto *grid = new QGridLayout;
    grid->setSpacing(14);

    QVBoxLayout *l1 = nullptr;
    QFrame *c1 = makeCard(l1);
    auto *h1 = new QLabel(QStringLiteral("📊 Synthèse des Modules"));
    h1->setObjectName("cardTitle");
    l1->addWidget(h1);
    l1->addWidget(makeSeparator());
    auto *b1 = new QLabel(
        "• <b>Gestion d'emploi</b> : 8 agents répertoriés (5 en service, 2 disponibles, 1 en congé)<br><br>"
        "• <b>Stations & Zones</b> : 4 zones surveillées (Tunis Nord, Sfax Côte, Gabès, Kairouan)<br><br>"
        "• <b>Interventions</b> : Alerte chaleur active sur Tunis Nord — 2 interventions planifiées<br><br>"
        "• <b>Équipements</b> : 21 équipements enregistrés — 14 disponibles, 4 en maintenance"
    );
    b1->setTextFormat(Qt::RichText);
    b1->setWordWrap(true);
    l1->addWidget(b1, 1);

    QVBoxLayout *l2 = nullptr;
    QFrame *c2 = makeCard(l2);
    auto *h2 = new QLabel(QStringLiteral("⚡ Actions Rapides"));
    h2->setObjectName("cardTitle");
    l2->addWidget(h2);
    l2->addWidget(makeSeparator());

    auto *btnAcc1 = new QPushButton("📡 Déclarer une nouvelle Zone");
    btnAcc1->setObjectName("primary");
    btnAcc1->setCursor(Qt::PointingHandCursor);

    auto *btnAcc2 = new QPushButton("⚠ Créer une Intervention d'urgence");
    btnAcc2->setCursor(Qt::PointingHandCursor);

    auto *btnAcc3 = new QPushButton("🧰 Ajouter un Équipement");
    btnAcc3->setCursor(Qt::PointingHandCursor);

    auto *btnAcc4 = new QPushButton("👤 Affecter un Agent");
    btnAcc4->setCursor(Qt::PointingHandCursor);

    l2->addWidget(btnAcc1);
    l2->addWidget(btnAcc2);
    l2->addWidget(btnAcc3);
    l2->addWidget(btnAcc4);
    l2->addStretch(1);

    grid->addWidget(c1, 0, 0);
    grid->addWidget(c2, 0, 1);
    grid->setColumnStretch(0, 2);
    grid->setColumnStretch(1, 1);

    root->addLayout(grid, 1);

    // Wire action buttons to emitted signals
    connect(btnAcc1, &QPushButton::clicked, this, &AccueilPage::actionNewZoneRequested);
    connect(btnAcc2, &QPushButton::clicked, this, &AccueilPage::actionNewInterventionRequested);
    connect(btnAcc3, &QPushButton::clicked, this, &AccueilPage::actionNewEquipmentRequested);
    connect(btnAcc4, &QPushButton::clicked, this, &AccueilPage::actionNewAgentRequested);
}
