#pragma once
#include <QDialog>
#include <QList>
#include "zone.h"

// Interface "Détails zone" : fiche complete + indicateurs metiers.
class ZoneDetailsDialog : public QDialog
{
    Q_OBJECT
public:
    ZoneDetailsDialog(const Zone &zone, int rangPriorite, int nbZones, QWidget *parent = nullptr);
};
