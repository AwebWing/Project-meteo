#include "interventionservice.h"
#include <QDateTime>
#include <QMap>
#include <algorithm>

namespace {
Intervention createStubIntervention(int id, const QString &type, const QString &priorite, const QString &statut, const QString &zoneNom) {
    Intervention i;
    i.id = id; i.type = type; i.priorite = priorite; i.statut = statut;
    i.debut = QDateTime::currentDateTime().addDays(-1);
    i.fin = QDateTime::currentDateTime().addDays(1);
    i.description = "Intervention de démonstration";
    i.responsable = "Agent Demo";
    i.idZone = 1; i.zoneNom = zoneNom;
    return i;
}
}

QMap<int, Intervention> InterventionService::s_interventions;
int InterventionService::s_nextId = 1;

const QStringList &InterventionService::types() {
    static const QStringList t = {"Inondation", "Chaleur", "Vent fort", "Maintenance station", "Inspection", "Autre"};
    return t;
}

const QStringList &InterventionService::priorites() {
    static const QStringList p = {Priorite::Faible, Priorite::Moyenne, Priorite::Haute, Priorite::Critique};
    return p;
}

const QStringList &InterventionService::statuts() {
    static const QStringList s = {Statut::Planifiee, Statut::EnCours, Statut::Terminee, Statut::Annulee};
    return s;
}

QStringList InterventionService::transitionsFrom(const QString &statut) {
    if (statut == Statut::Planifiee) return {Statut::Planifiee, Statut::EnCours, Statut::Annulee};
    if (statut == Statut::EnCours)   return {Statut::EnCours, Statut::Terminee, Statut::Annulee};
    if (statut == Statut::Terminee)  return {Statut::Terminee, Statut::EnCours};
    return {statut};
}

QStringList InterventionService::validate(const Intervention &i, const Intervention *ancien) {
    QStringList e;
    if (!types().contains(i.type)) e << "Le type d'intervention est invalide.";
    if (!priorites().contains(i.priorite)) e << "La priorité est invalide.";
    if (!statuts().contains(i.statut)) e << "Le statut est invalide.";
    if (!ancien && i.statut != Statut::Planifiee) e << "Une nouvelle intervention commence au statut Planifiée.";
    if (ancien && ancien->statut != i.statut && !transitionsFrom(ancien->statut).contains(i.statut))
        e << QString("Transition interdite: %1 -> %2").arg(ancien->statut, i.statut);
    if (!i.debut.isValid() || !i.fin.isValid()) e << "Dates obligatoires.";
    else if (i.fin < i.debut) e << "La fin ne peut précéder le début.";
    if (i.statut == Statut::Terminee) e += controleCloture(i);
    return e;
}

bool InterventionService::add(Intervention &i, QString *err) {
    QStringList errs = validate(i, nullptr);
    if (!errs.isEmpty()) { if (err) *err = errs.join("\n"); return false; }
    i.id = s_nextId++;
    s_interventions[i.id] = i;
    return true;
}

bool InterventionService::update(const Intervention &i, [[maybe_unused]] const QString &justification, QString *err) {
    auto ancien = get(i.id);
    if (!ancien) { if (err) *err = "Introuvable"; return false; }
    QStringList errs = validate(i, &*ancien);
    if (!errs.isEmpty()) { if (err) *err = errs.join("\n"); return false; }
    s_interventions[i.id] = i;
    return true;
}

bool InterventionService::remove([[maybe_unused]] int id, [[maybe_unused]] QString *err) {
    if (s_interventions.contains(id)) { s_interventions.remove(id); return true; }
    if (err) *err = "Introuvable"; return false;
}

bool InterventionService::cancel([[maybe_unused]] int id, [[maybe_unused]] QString *err) {
    auto it = s_interventions.find(id);
    if (it == s_interventions.end()) return false;
    Intervention i = it.value(); i.statut = Statut::Annulee;
    s_interventions[id] = i;
    return true;
}

std::optional<Intervention> InterventionService::get(int id) {
    if (s_interventions.isEmpty()) {
        s_interventions[1] = createStubIntervention(1, "Inondation", "Haute", "En cours", "Tunis Centre");
        s_interventions[2] = createStubIntervention(2, "Vent fort", "Critique", "Planifiée", "Ariana");
        s_interventions[3] = createStubIntervention(3, "Maintenance", "Faible", "Terminée", "Ben Arous");
    }
    return s_interventions.contains(id) ? std::optional<Intervention>(s_interventions[id]) : std::nullopt;
}

bool InterventionService::load(int id, Intervention &i) {
    auto res = get(id);
    if (res) { i = *res; return true; }
    return false;
}

QVector<Intervention> InterventionService::list(const InterventionFilter &f, QString *err) {
    if (s_interventions.isEmpty()) {
        s_interventions[1] = createStubIntervention(1, "Inondation", "Haute", "En cours", "Tunis Centre");
        s_interventions[2] = createStubIntervention(2, "Vent fort", "Critique", "Planifiée", "Ariana");
        s_interventions[3] = createStubIntervention(3, "Maintenance", "Faible", "Terminée", "Ben Arous");
    }
    QVector<Intervention> out;
    for (auto it = s_interventions.begin(); it != s_interventions.end(); ++it) {
        out.push_back(it.value());
    }
    return out;
}

PriorityResult InterventionService::recalculerPriorite(int id, bool appliquer) {
    PriorityResult r; r.ancienne = "Moyenne"; r.nouvelle = "Haute"; r.score = 65; r.changee = true; r.explication = "Dummy";
    if (appliquer) { auto i = get(id); if (i) { i->priorite = r.nouvelle; s_interventions[id] = *i; } }
    return r;
}

QVector<Intervention> InterventionService::interventionsEnRetard(bool onlyLate) {
    QVector<Intervention> out;
    for (auto it = s_interventions.begin(); it != s_interventions.end(); ++it) {
        const Intervention &val = it.value();
        if (val.statut == Statut::EnCours && val.fin < QDateTime::currentDateTime()) out << val;
    }
    return out;
}

QStringList InterventionService::prerequis(const Intervention &i) {
    return {"Données de démonstration : prêt."};
}

bool InterventionService::verifierPrerequis(int id, QString *err) {
    return get(id).has_value();
}

DurationEstimate InterventionService::estimerDuree(const QString &type, int idZone) {
    DurationEstimate d; d.heures = 12.5; d.echantillon = 5; d.base = "Dummy"; return d;
}

QStringList InterventionService::controleCloture(const Intervention &i) {
    QStringList e;
    if (i.compteRendu.trimmed().size() < 20) e << "Compte-rendu trop court (20 min).";
    return e;
}

bool InterventionService::cloturer(int id, const QString &compteRendu, const QDateTime &fin, QString *err) {
    auto it = s_interventions.find(id);
    if (it == s_interventions.end()) return false;
    Intervention i = it.value(); i.statut = Statut::Terminee; i.compteRendu = compteRendu; i.fin = fin;
    s_interventions[id] = i;
    return true;
}

Counts InterventionService::repartition(const QString &dimension, const InterventionFilter &f) {
    return { {"En cours", 1}, {"Planifiée", 2} };
}

QVector<HistoryEntry> InterventionService::historique(int id) {
    return { {QDateTime::currentDateTime(), "Création", "Dummy"} };
}

QVector<ZoneItem> InterventionService::zones(bool activeOnly) {
    QVector<ZoneItem> out = { {1, "Tunis Centre", true}, {2, "Ariana", true}, {3, "Sousse", false} };
    if (activeOnly) {
        QVector<ZoneItem> filtered;
        for(auto &z : out) if(z.active) filtered << z;
        return filtered;
    }
    return out;
}

QVector<EmploiItem> InterventionService::emplois() {
    return { {1, "Agent Demo"} };
}

QString InterventionService::zoneNom(int idZone) {
    for(auto &z : zones()) if(z.id == idZone) return z.nom;
    return "Inconnue";
}

int InterventionService::total() { return s_interventions.size(); }
int InterventionService::enCours() {
    int c = 0;
    for(auto &i : s_interventions) if(i.statut == Statut::EnCours) c++;
    return c;
}
QVector<Intervention> InterventionService::enRetard() { return interventionsEnRetard(true); }
int InterventionService::critiques() {
    int c = 0;
    for(auto &i : s_interventions) if(i.priorite == Priorite::Critique) c++;
    return c;
}
Counts InterventionService::parStatut() { return repartition("statut"); }
Counts InterventionService::parPriorite() { return repartition("priorite"); }
Counts InterventionService::parZone() { return repartition("zone"); }

QVector<IdLabel> InterventionService::equipements() {
    return { {1, "Equipement 1"}, {2, "Equipement 2"} };
}

bool InterventionService::affecterEquipement(int idIntervention, int idEquipement, QString *err) {
    return true;
}

void InterventionService::log(int id, const QString &action, const QString &detail) {}
void InterventionService::libererEquipements(int idIntervention) {}
