#include "equipmentservice.h"
#include <algorithm>
#include <cmath>

namespace EquipmentService {

// --- Validation ---
QStringList valider(const Equipment &eq, const QList<Equipment> &existants)
{
    QStringList errors;

    if (eq.nom.trimmed().isEmpty())
        errors << QStringLiteral("Le nom de l'équipement est obligatoire.");

    if (eq.categorie.trimmed().isEmpty())
        errors << QStringLiteral("La catégorie est obligatoire.");
    else if (!EQUIPMENT_CATEGORIES.contains(eq.categorie))
        errors << QStringLiteral("Catégorie inconnue.");

    if (!EQUIPMENT_STATUTS.contains(eq.statut))
        errors << QStringLiteral("Le statut doit être Disponible, En maintenance ou Hors service.");

    if (eq.localisation.trimmed().isEmpty())
        errors << QStringLiteral("La localisation est obligatoire.");

    // Code unique (case-insensitive) si renseigné
    if (!eq.code.trimmed().isEmpty()) {
        for (const Equipment &e : existants) {
            if (e.id != eq.id && e.code.compare(eq.code, Qt::CaseInsensitive) == 0) {
                errors << QStringLiteral("Ce code existe déjà (%1).").arg(e.nom);
                break;
            }
        }
    }

    return errors;
}

// --- Statistiques ---
Stats statistiques(const QList<Equipment> &equipments)
{
    Stats s;
    s.total = equipments.size();
    for (const Equipment &e : equipments) {
        if      (e.statut == QLatin1String("Disponible"))     ++s.disponibles;
        else if (e.statut == QLatin1String("En maintenance")) ++s.enMaintenance;
        else if (e.statut == QLatin1String("Hors service"))   ++s.horsService;
    }
    s.pctDisponibles = (s.total > 0)
        ? std::round(1000.0 * s.disponibles / s.total) / 10.0
        : 0.0;
    return s;
}

// --- Comptage par catégorie ---
QMap<QString, int> categoryCounts(const QList<Equipment> &equipments)
{
    QMap<QString, int> counts;
    for (const QString &cat : EQUIPMENT_CATEGORIES)
        counts[cat] = 0;
    for (const Equipment &e : equipments)
        if (counts.contains(e.categorie))
            counts[e.categorie]++;
    return counts;
}

// --- Alertes : dernière maintenance > 180 jours ---
QList<Equipment> alertesMaintenance(const QList<Equipment> &equipments)
{
    QList<Equipment> alerts;
    const QDate today = QDate::currentDate();
    for (const Equipment &e : equipments)
        if (e.statut != QLatin1String("Hors service") &&
            e.derniereMaintenance.isValid() &&
            e.derniereMaintenance.daysTo(today) > 180)
            alerts << e;
    return alerts;
}

} // namespace EquipmentService
