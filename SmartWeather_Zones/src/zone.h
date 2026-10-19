#pragma once
#include <QDateTime>
#include <QString>

// ------------------------------------------------------------
//  Entite ZONE  (voir MLD du cahier : chapitre 8.2)
//  ZONE(id_zone, nom, code, vulnerabilite, latitude, longitude, statut)
//  Les 2 derniers champs sont des donnees "support" qui viendront plus tard
//  des autres gestions (STATION de Tasnim, INTERVENTION de Bader).
// ------------------------------------------------------------
struct Zone {
    int     id = 0;
    QString nom;
    QString code;
    double  vulnerabilite = 0.0;                 // 0..100
    double  latitude  = 36.8065;
    double  longitude = 10.1815;
    QString statut = QStringLiteral("Active");   // Active / Inactive

    int nbStations = 0;            // support : nombre de stations rattachees
    int interventionsActives = 0;  // support : interventions en cours dans la zone
};

// Metier 5 : historique d'evolution
struct ZoneHistory {
    int       idZone = 0;
    QDateTime date;
    QString   champ;
    QString   ancien;
    QString   nouveau;
};
