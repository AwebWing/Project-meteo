#pragma once
#include <QWidget>

class ZoneRepository;

// Module "Stations & Zones" : bandeau titre + onglets + pages.
// Onglets actuels : Zones (Adem)  |  Analyse couverture (Adem).
// Plus tard, Tasnim pourra ajouter ici son onglet "Stations".
class ZonesModule : public QWidget
{
    Q_OBJECT
public:
    explicit ZonesModule(ZoneRepository *repo, QWidget *parent = nullptr);
};
