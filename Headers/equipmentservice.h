#pragma once
#include <QList>
#include <QMap>
#include <QStringList>
#include "equipment.h"

// ============================================================
//  EquipmentService : logique métier — aucune dépendance à l'UI
// ============================================================
namespace EquipmentService {

// Nom : commence par une lettre, puis lettres / espaces / apostrophe / tiret — aucun chiffre.
// (Utilisé par la validation ET par le filtre de saisie du formulaire.)
inline const QString NOM_REGEX = QStringLiteral("^\\p{L}[\\p{L} '’\\-]*$");

// --- Validation CRUD (retourne liste d'erreurs, vide = OK) ---
QStringList valider(const Equipment &eq, const QList<Equipment> &existants);

// --- Statistiques globales ---
struct Stats {
    int    total          = 0;
    int    disponibles    = 0;
    int    enMaintenance  = 0;
    int    horsService    = 0;
    double pctDisponibles = 0.0;   // arrondi à 1 décimale
};
Stats statistiques(const QList<Equipment> &equipments);

// --- Comptage par catégorie ---
QMap<QString, int> categoryCounts(const QList<Equipment> &equipments);

// --- Alertes de maintenance (dernière maintenance > 180 jours) ---
QList<Equipment> alertesMaintenance(const QList<Equipment> &equipments);

} // namespace EquipmentService
