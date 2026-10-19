#pragma once
#include <QDateTime>
#include <QList>
#include <QString>

// ------------------------------------------------------------
//  Entite ZONE  (table ZONE de la base Oracle, voir SQL/zones_oracle.sql)
//  ZONE(id_zone, nom, code, vulnerabilite, latitude, longitude, statut)
//
//  Architecture modele-vue : TOUTES les requetes SQL de l'entite sont ici
//  (Sources/zone.cpp). Aucune requete derriere un bouton.
//  Toutes les requetes sont PREPAREES (prepare + bindValue) => pas d'injection SQL.
//
//  nbStations / interventionsActives sont des donnees "support" qui viendront
//  des tables STATION (Tasnim) et INTERVENTION (Bader) : elles ne sont pas
//  stockees dans ZONE.
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

    // ---------------- CRUD (Oracle) ----------------
    // Chaque methode retourne true si la requete a reussi ; sinon, si "erreur"
    // n'est pas nul, il recoit un message lisible pour l'utilisateur.

    // Create : insere la zone, renseigne this->id et historise la creation.
    bool ajouter(QString *erreur = nullptr);
    // Update : met a jour la ligne ID_ZONE = id et historise les champs modifies (metier 5).
    bool modifier(QString *erreur = nullptr) const;
    // Delete : supprime la zone (son historique est supprime en cascade).
    static bool supprimer(int id, QString *erreur = nullptr);
    // Read : toutes les zones, triees par id.
    static QList<Zone> afficher(QString *erreur = nullptr);
};

// Metier 5 : historique d'evolution (table ZONE_HISTORIQUE)
struct ZoneHistory {
    int       idZone = 0;
    QDateTime date;
    QString   champ;
    QString   ancien;
    QString   nouveau;

    // Historique d'une zone, du plus recent au plus ancien.
    static QList<ZoneHistory> pourZone(int idZone, QString *erreur = nullptr);
};
