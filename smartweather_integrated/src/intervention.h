#pragma once
// Couche métier « Gestion des interventions » — AUCUNE dépendance à l'interface graphique
// (exigence de maintenabilité du cahier : règles métier testables indépendamment des écrans).

#include <QDateTime>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace Priorite {
    const QString Faible = "Faible";
    const QString Moyenne = "Moyenne";
    const QString Haute = "Haute";
    const QString Critique = "Critique";
}

namespace Statut {
    const QString Planifiee = "Planifiée";
    const QString EnCours = "En cours";
    const QString Terminee = "Terminée";
    const QString Annulee = "Annulée";
}

struct Intervention {
    int id = 0;
    QString type;
    QString priorite = Priorite::Moyenne;
    QString statut = Statut::Planifiee;
    QDateTime debut;
    QDateTime fin;            // invalide = non définie
    QString description;
    QString responsable;
    QString compteRendu;
    int idZone = 0;
    QString zoneNom;           // Added missing member for integrated version
    int idEmploi = 0;         // 0 = aucun emploi lié
};

struct Prerequis {
    bool pret = false;
    QStringList problemes;
};

struct Estimation {
    double heures = -1;       // -1 = pas d'historique exploitable
    int echantillon = 0;
    bool memeZone = false;
};

using Counts = QList<QPair<QString, int>>;
using IdLabel = QList<QPair<int, QString>>;

// Note: Business logic is now centralized in the InterventionService class in interventionservice.h
