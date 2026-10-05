#pragma once
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QStringList>

// Catégories disponibles (ordre stable = ordre d'affichage)
inline const QStringList EQUIPMENT_CATEGORIES = {
    QStringLiteral("Capteurs"),
    QStringLiteral("Véhicules"),
    QStringLiteral("Matériel de terrain"),
    QStringLiteral("Équipements de sécurité")
};

// Statuts possibles
inline const QStringList EQUIPMENT_STATUTS = {
    QStringLiteral("Disponible"),
    QStringLiteral("En maintenance"),
    QStringLiteral("Hors service")
};

// Icônes par catégorie
inline QString categoryIcon(const QString &cat)
{
    if (cat == QLatin1String("Capteurs"))                  return QStringLiteral("📡");
    if (cat == QLatin1String("Véhicules"))                 return QStringLiteral("🚗");
    if (cat == QLatin1String("Matériel de terrain"))       return QStringLiteral("🧰");
    if (cat == QLatin1String("Équipements de sécurité"))   return QStringLiteral("🦺");
    return QStringLiteral("📦");
}

// ---------------------------------------------------------------
//  Entité EQUIPEMENT
// ---------------------------------------------------------------
struct Equipment {
    int     id   = 0;
    QString nom;
    QString code;                // identifiant court ex: EQ-001
    QString categorie;
    QString statut = QStringLiteral("Disponible"); // Disponible / En maintenance / Hors service
    QString localisation;
    QDate   derniereMaintenance;
    QString description;
};

// ---------------------------------------------------------------
//  Historique des modifications (Métier 5)
// ---------------------------------------------------------------
struct EquipmentHistory {
    int       idEquipment = 0;
    QDateTime date;
    QString   champ;
    QString   ancien;
    QString   nouveau;
};
