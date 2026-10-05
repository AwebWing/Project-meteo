#pragma once
#include <QWidget>

class QLabel;

class AccueilPage : public QWidget
{
    Q_OBJECT
public:
    explicit AccueilPage(QWidget *parent = nullptr);

signals:
    void actionNewZoneRequested();
    void actionNewInterventionRequested();
    void actionNewEquipmentRequested();
    void actionNewAgentRequested();
};
