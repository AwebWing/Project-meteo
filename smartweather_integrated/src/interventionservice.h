#pragma once
#include "intervention.h"

#include <QDate>
#include <QPair>
#include <QStringList>
#include <QVector>
#include <optional>

struct InterventionFilter {
    QString texte;          // recherche instantanée (id, type, zone, responsable, statut...)
    QString statut;
    QString priorite;
    QString type;
    int idZone = 0;         // 0 = toutes
    bool periode = false;
    QDate du, au;
};

struct PriorityResult {
    QString ancienne, nouvelle;
    int score = 0;
    bool changee = false;
    QString explication;
};

struct DurationEstimate {
    double heures = -1;     // -1 = pas assez d'historique
    int echantillon = 0;
    QString base;           // "même type et même zone" / "même type" / "toutes interventions"
};

struct HistoryEntry {
    QDateTime date;
    QString action;
    QString detail;
};

struct ZoneItem {
    int id = 0;
    QString nom;
    bool active = true;
};

struct EmploiItem {
    int id = 0;
    QString label;
};

using Counts = QVector<QPair<QString, int>>;

// Toute la logique métier du module "Gestion des interventions".
// Aucune dépendance à l'interface graphique => testable seule (exigence « Maintenabilité »).
class InterventionService {
public:
    static const QStringList &types();
    static const QStringList &priorites();   // du moins au plus grave
    static const QStringList &statuts();
    static QStringList transitionsFrom(const QString &statut);   // inclut le statut lui-même

    // ----- CRUD + validations -----
    static QStringList validate(const Intervention &i, const Intervention *ancien = nullptr);
    static bool add(Intervention &i, QString *err = nullptr);
    static bool update(const Intervention &i, const QString &justification, QString *err = nullptr);
    static bool remove(int id, QString *err = nullptr);
    static bool cancel(int id, QString *err = nullptr);
    static std::optional<Intervention> get(int id);
    static QVector<Intervention> list(const InterventionFilter &f = {}, QString *err = nullptr);

    // ----- 5 métiers avancés -----
    static PriorityResult recalculerPriorite(int id, bool appliquer = true);     // 1
    static QVector<Intervention> interventionsEnRetard();                        // 2
    static QStringList prerequis(const Intervention &i);                         // 3 (vide = prête)
    static DurationEstimate estimerDuree(const QString &type, int idZone);       // 4
    static QStringList controleCloture(const Intervention &i);                   // 5
    static bool cloturer(int id, const QString &compteRendu, const QDateTime &fin, QString *err = nullptr);

    // ----- Statistiques, historique, listes de référence -----
    static Counts repartition(const QString &dimension, const InterventionFilter &f = {});
    static QVector<HistoryEntry> historique(int id);
    static QVector<ZoneItem> zones();
    static QVector<EmploiItem> emplois();

private:
    static void log(int id, const QString &action, const QString &detail);
    static void libererEquipements(int idIntervention);
};
