#pragma once
#include <QDate>
#include <QDateTime>
#include <QList>
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
//  Entité EQUIPEMENT  (table EQUIPEMENT de la base Oracle)
//  EQUIPEMENT(id_equipement, nom, code, categorie, statut,
//             localisation, derniere_maint, description)
//
//  Architecture modèle-vue : TOUTES les requêtes SQL sont ici
//  (Sources/equipment.cpp). Aucune requête derrière un bouton.
//  Toutes les requêtes sont PRÉPARÉES (prepare + bindValue).
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

    // ---------------- CRUD (Oracle) ----------------
    // Chaque méthode retourne true si la requête a réussi.
    // En cas d'échec, *erreur reçoit un message lisible.

    // Create : insère l'équipement, renseigne this->id et historise la création.
    bool ajouter(QString *erreur = nullptr);
    // Update : met à jour la ligne et historise les champs modifiés (métier 5).
    bool modifier(QString *erreur = nullptr) const;
    // Delete : supprime l'équipement (son historique est supprimé en cascade).
    static bool supprimer(int id, QString *erreur = nullptr);
    // Read : tous les équipements, triés par id.
    static QList<Equipment> afficher(QString *erreur = nullptr);
};

// ---------------------------------------------------------------
//  Historique des modifications (Métier 5) — table EQUIPEMENT_HISTORIQUE
// ---------------------------------------------------------------
struct EquipmentHistory {
    int       idEquipment = 0;
    QDateTime date;
    QString   champ;
    QString   ancien;
    QString   nouveau;

    // Historique d'un équipement, du plus récent au plus ancien.
    static QList<EquipmentHistory> pourEquipement(int idEquipment,
                                                   QString *erreur = nullptr);
};
