#pragma once
#include "intervention.h"
#include <QDate>
#include <QStringList>
#include <QVector>
#include <optional>
#include <QMap>

struct InterventionFilter {
    QString texte;
    QString statut;
    QString priorite;
    QString type;
    int idZone = 0;
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
    double heures = -1;
    int echantillon = 0;
    QString base;
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

class InterventionService {
public:
    static const QStringList &types();
    static const QStringList &priorites();
    static const QStringList &statuts();
    static QStringList transitionsFrom(const QString &statut);

    // CRUD
    static QStringList validate(const Intervention &i, const Intervention *ancien = nullptr);
    static bool add(Intervention &i, QString *err = nullptr);
    static bool update(const Intervention &i, const QString &justification, QString *err = nullptr);
    static bool remove(int id, QString *err = nullptr);
    static bool cancel(int id, QString *err = nullptr);
    static std::optional<Intervention> get(int id);
    static bool load(int id, Intervention &i);
    static QVector<Intervention> list(const InterventionFilter &f = {}, QString *err = nullptr);

    // Advanced logic
    static PriorityResult recalculerPriorite(int id, bool appliquer = true);
    static QVector<Intervention> interventionsEnRetard(bool onlyLate = false);
    static QStringList prerequis(const Intervention &i);
    static bool verifierPrerequis(int id, QString *err = nullptr);
    static DurationEstimate estimerDuree(const QString &type, int idZone);
    static QStringList controleCloture(const Intervention &i);
    static bool cloturer(int id, const QString &compteRendu, const QDateTime &fin, QString *err = nullptr);

    // Stats & References
    static Counts repartition(const QString &dimension, const InterventionFilter &f = {});
    static QVector<HistoryEntry> historique(int id);
    static QVector<ZoneItem> zones(bool activeOnly = false);
    static QVector<EmploiItem> emplois();
    static QString zoneNom(int idZone);

    // UI KPIs
    static int total();
    static int enCours();
    static QVector<Intervention> enRetard();
    static int critiques();
    static Counts parStatut();
    static Counts parPriorite();
    static Counts parZone();
    static QVector<IdLabel> equipements();
    static bool affecterEquipement(int idIntervention, int idEquipement, QString *err = nullptr);

private:
    static void log(int id, const QString &action, const QString &detail);
    static void libererEquipements(int idIntervention);

    static QMap<int, Intervention> s_interventions;
    static int s_nextId;
};
