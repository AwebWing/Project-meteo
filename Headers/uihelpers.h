#pragma once
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

// Petite carte KPI : icône + titre + grande valeur.
inline QFrame *makeKpiCard(const QString &icon, const QString &title, QLabel *&valueOut)
{
    auto *frame = new QFrame;
    frame->setObjectName("kpi");

    auto *h = new QHBoxLayout(frame);
    h->setContentsMargins(16, 12, 16, 12);
    h->setSpacing(14);

    auto *ic = new QLabel(icon);
    ic->setObjectName("kpiIcon");
    ic->setFixedSize(44, 44);
    ic->setAlignment(Qt::AlignCenter);

    auto *v = new QVBoxLayout;
    v->setSpacing(0);
    auto *t = new QLabel(title);
    t->setObjectName("muted");
    valueOut = new QLabel(QStringLiteral("—"));
    valueOut->setObjectName("kpiValue");
    v->addWidget(t);
    v->addWidget(valueOut);

    h->addWidget(ic);
    h->addLayout(v, 1);
    return frame;
}

// Cadre "carte" vide (fond sombre + bordure) avec un layout vertical.
inline QFrame *makeCard(QVBoxLayout *&layoutOut)
{
    auto *frame = new QFrame;
    frame->setObjectName("card");
    layoutOut = new QVBoxLayout(frame);
    layoutOut->setContentsMargins(16, 14, 16, 14);
    layoutOut->setSpacing(10);
    return frame;
}

// Séparateur horizontal
inline QFrame *makeSeparator()
{
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #17475a;");
    return sep;
}
