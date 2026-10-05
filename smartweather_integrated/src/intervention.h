#pragma once
#include <QDateTime>
#include <QString>

// Valeurs contrôlées (cahier des spécifications, annexe B)
namespace Statut {
inline const QString Planifiee = QStringLiteral("Planifiée");
inline const QString EnCours   = QStringLiteral("En cours");
inline const QString Terminee  = QStringLiteral("Terminée");
inline const QString Annulee   = QStringLiteral("Annulée");
}
namespace Priorite {
inline const QString Faible    = QStringLiteral("Faible");
inline const QString Moyenne   = QStringLiteral("Moyenne");
inline const QString Haute     = QStringLiteral("Haute");
inline const QString Critique  = QStringLiteral("Critique");
}

struct Intervention {
    int id = 0;
    QString type;
    QString priorite;
    QString statut;
    QDateTime debut;
    QDateTime fin;
    QString description;
    QString responsable;
    QString compteRendu;
    int idZone = 0;
    int idEmploi = 0;      // 0 = aucun emploi lié (NULL en base)
    QString zoneNom;       // lecture seule (jointure avec ZONE)
};
