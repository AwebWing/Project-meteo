#pragma once
#include <QList>
#include <QStringList>
#include "zone.h"

// ============================================================
//  ZoneService : les 5 METIERS AVANCES de la gestion des zones
//  (cahier 5.2.2). Aucune dependance a l'interface => testable seul.
// ============================================================
namespace ZoneService {

// Metier 1 : score de vulnerabilite (criteres ponderes) -> niveau
double  scoreVulnerabilite(double expositionInondation, double densitePopulation, double historiqueIncidents);
QString niveau(double vulnerabilite);                 // Faible / Moyen / Élevé

// Metier 2 : priorite territoriale (exposition + activite operationnelle)
double      scorePriorite(const Zone &z);
QList<Zone> classementPriorite(QList<Zone> zones);    // tri decroissant

// Metier 3 : couverture insuffisante (stations presentes vs attendues)
int    stationsAttendues(const Zone &z);
bool   sousCouverture(const Zone &z);
double couvertureReseau(const QList<Zone> &zones);    // en %

// Metier 4 : coherence geographique
bool        coordonneesValides(const Zone &z);
QStringList controleGeographique(const Zone &z);      // anomalies (avertissements)

// Validations du CRUD (cahier 5.2.1) -> liste d'erreurs (vide = OK)
QStringList valider(const Zone &z, const QList<Zone> &existantes);

// Metier 5 (historique) : voir ZoneRepository::history()

} // namespace ZoneService
