#pragma once
#include <QString>
#include <QDateTime>
#include <QVector>

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
    QString zoneNom;
    int idEmploi = 0;

    bool operator==(const Intervention &other) const {
        return id == other.id;
    }
};

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

struct IdLabel {
    int id;
    QString label;
};
