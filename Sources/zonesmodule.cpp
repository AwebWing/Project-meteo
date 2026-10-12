#include "zonesmodule.h"
#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include "coveragepage.h"
#include "uihelpers.h"
#include "zonespage.h"

ZonesModule::ZonesModule(ZoneRepository *repo, QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // ---- Bandeau titre (comme la figure 3 du cahier) ----
    auto *banner = new QFrame;
    banner->setObjectName("card");
    auto *h = new QHBoxLayout(banner);
    h->setContentsMargins(18, 12, 18, 12);
    h->setSpacing(14);

    auto *icon = new QLabel(QStringLiteral("📡"));
    icon->setObjectName("kpiIcon");
    icon->setFixedSize(48, 48);
    icon->setAlignment(Qt::AlignCenter);

    auto *titles = new QVBoxLayout;
    titles->setSpacing(0);
    auto *title = new QLabel(QStringLiteral("Stations & Zones"));
    title->setObjectName("title");
    auto *sub = new QLabel(QStringLiteral("Gestion des zones géographiques et des stations météo"));
    sub->setObjectName("muted");
    titles->addWidget(title);
    titles->addWidget(sub);

    auto *tabZones = new QPushButton(QStringLiteral("Zones"));
    auto *tabCov = new QPushButton(QStringLiteral("Analyse couverture"));
    for (auto *b : {tabZones, tabCov}) {
        b->setObjectName("tab");
        b->setCheckable(true);
    }
    tabZones->setChecked(true);
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(tabZones, 0);
    group->addButton(tabCov, 1);

    h->addWidget(icon);
    h->addLayout(titles, 1);
    h->addWidget(tabZones);
    h->addWidget(tabCov);
    root->addWidget(banner);

    // ---- Pages ----
    auto *stack = new QStackedWidget;
    stack->addWidget(new ZonesPage(repo));
    stack->addWidget(new CoveragePage(repo));
    root->addWidget(stack, 1);

    connect(group, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);
}
