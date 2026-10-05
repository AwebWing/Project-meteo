#include "intervention.h"

#include <QMap>
#include <QMetaType>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <algorithm>

namespace InterventionService {

namespace {

QVariant nullStr() { return QVariant(QMetaType::fromType<QString>()); }
QVariant nullInt() { return QVariant(QMetaType::fromType<int>()); }

QString iso(const QDateTime &d) { return d.toString(Qt::ISODate); }

bool fail(QString *err, const QString &msg)
{
    if (err) *err = msg;
    return false;
}

void log(int id, const QString &evenement)
{
    QSqlQuery q;
    q.prepare("INSERT INTO historique_intervention(id_intervention,date_evt,evenement) VALUES(?,?,?)");
    q.addBindValue(id);
    q.addBindValue(iso(QDateTime::currentDateTime()));
    q.addBindValue(evenement);
    q.exec();
}

// Libère le matériel d'une intervention terminée / annulée.
void syncEquipements(int id, const QString &statut)
{
    if (statut != "Terminée" && statut != "Annulée") return;
    QSqlQuery q;
    q.prepare("UPDATE equipement SET statut='Disponible' WHERE id_intervention=? AND statut='En mission'");
    q.addBindValue(id);
    q.exec();
    q.prepare("UPDATE equipement SET id_intervention=NULL WHERE id_intervention=?");
    q.addBindValue(id);
    q.exec();
}

Counts countBy(const QString &sql, const QStringList &ordre)
{
    QMap<QString, int> m;
    QSqlQuery q(sql);
    while (q.next()) m[q.value(0).toString()] = q.value(1).toInt();

    Counts out;
    if (!ordre.isEmpty()) {
        for (const QString &k : ordre) out.append({k, m.value(k, 0)});
    } else {
        for (auto it = m.cbegin(); it != m.cend(); ++it) out.append({it.key(), it.value()});
        std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
    }
    return out;
}

int scalar(const QString &sql)
{
    QSqlQuery q(sql);
    return q.next() ? q.value(0).toInt() : 0;
}

QString jolie(const QString &isoDate)
{
    const QDateTime d = QDateTime::fromString(isoDate, Qt::ISODate);
    return d.isValid() ? d.toString("dd/MM/yyyy HH:mm") : isoDate;
}

} // namespace

const QStringList &types()
{
    static const QStringList t = {"Inondation", "Vent fort", "Canicule", "Inspection station",
                                  "Maintenance préventive", "Dépannage"};
    return t;
}
const QStringList &priorites()
{
    static const QStringList p = {"Faible", "Moyenne", "Haute", "Critique"};
    return p;
}
const QStringList &statuts()
{
    static const QStringList s = {"Planifiée", "En cours", "Terminée", "Annulée"};
    return s;
}

// ------------------------------------------------------------------ CRUD

bool load(int id, Intervention &o)
{
    QSqlQuery q;
    q.prepare("SELECT id_intervention,type,priorite,statut,date_debut,date_fin,description,responsable,"
              "compte_rendu,id_zone,id_emploi FROM intervention WHERE id_intervention=?");
    q.addBindValue(id);
    if (!q.exec() || !q.next()) return false;
    o.id = q.value(0).toInt();
    o.type = q.value(1).toString();
    o.priorite = q.value(2).toString();
    o.statut = q.value(3).toString();
    o.debut = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
    o.fin = QDateTime::fromString(q.value(5).toString(), Qt::ISODate);
    o.description = q.value(6).toString();
    o.responsable = q.value(7).toString();
    o.compteRendu = q.value(8).toString();
    o.idZone = q.value(9).toInt();
    o.idEmploi = q.value(10).isNull() ? 0 : q.value(10).toInt();
    return true;
}

bool controleCloture(const Intervention &i, QString *err)
{
    if (i.compteRendu.trimmed().size() < 20)
        return fail(err, "Clôture refusée : le compte-rendu est obligatoire (20 caractères minimum).");
    if (!i.fin.isValid())
        return fail(err, "Clôture refusée : la date de fin est obligatoire.");
    if (i.fin < i.debut)
        return fail(err, "Clôture refusée : la date de fin précède la date de début.");
    return true;
}

bool validate(const Intervention &i, const QString &ancienStatut, QString *err)
{
    if (i.type.trimmed().isEmpty()) return fail(err, "Le type d'intervention est obligatoire.");
    if (!priorites().contains(i.priorite)) return fail(err, "Priorité invalide (valeurs : Faible, Moyenne, Haute, Critique).");
    if (!statuts().contains(i.statut)) return fail(err, "Statut invalide.");
    if (!i.debut.isValid()) return fail(err, "La date de début est obligatoire.");
    if (i.fin.isValid() && i.fin < i.debut)
        return fail(err, "La date de fin ne peut pas précéder la date de début.");

    QSqlQuery q;
    q.prepare("SELECT statut FROM zone WHERE id_zone=?");
    q.addBindValue(i.idZone);
    if (!q.exec() || !q.next()) return fail(err, "Zone introuvable : l'intervention doit être reliée à une zone.");
    if (q.value(0).toString() != "Active")
        return fail(err, "L'intervention doit être reliée à une zone active.");

    if (i.statut == "En cours" && i.responsable.trimmed().isEmpty() && i.idEmploi == 0)
        return fail(err, "Une intervention en cours doit avoir un responsable ou un emploi affecté.");

    if (ancienStatut == "Terminée" && i.statut != "Terminée")
        return fail(err, "Réouverture bloquée : une intervention terminée ne peut pas être rouverte sans justification.");

    if (i.statut == "Terminée") return controleCloture(i, err);
    return true;
}

bool add(const Intervention &i, QString *err, int *newId)
{
    if (!validate(i, QString(), err)) return false;

    QSqlQuery q;
    q.prepare("INSERT INTO intervention(type,priorite,statut,date_debut,date_fin,description,responsable,"
              "compte_rendu,id_zone,id_emploi) VALUES(?,?,?,?,?,?,?,?,?,?)");
    q.addBindValue(i.type);
    q.addBindValue(i.priorite);
    q.addBindValue(i.statut);
    q.addBindValue(iso(i.debut));
    q.addBindValue(i.fin.isValid() ? QVariant(iso(i.fin)) : nullStr());
    q.addBindValue(i.description);
    q.addBindValue(i.responsable.trimmed().isEmpty() ? nullStr() : QVariant(i.responsable.trimmed()));
    q.addBindValue(i.compteRendu.trimmed().isEmpty() ? nullStr() : QVariant(i.compteRendu.trimmed()));
    q.addBindValue(i.idZone);
    q.addBindValue(i.idEmploi > 0 ? QVariant(i.idEmploi) : nullInt());
    if (!q.exec()) return fail(err, q.lastError().text());

    const int id = q.lastInsertId().toInt();
    log(id, QString("Création (statut %1, priorité %2)").arg(i.statut, i.priorite));
    syncEquipements(id, i.statut);
    if (newId) *newId = id;
    return true;
}

bool update(const Intervention &i, QString *err)
{
    Intervention old;
    if (!load(i.id, old)) return fail(err, "Intervention introuvable.");
    if (!validate(i, old.statut, err)) return false;

    QSqlQuery q;
    q.prepare("UPDATE intervention SET type=?,priorite=?,statut=?,date_debut=?,date_fin=?,description=?,"
              "responsable=?,compte_rendu=?,id_zone=?,id_emploi=? WHERE id_intervention=?");
    q.addBindValue(i.type);
    q.addBindValue(i.priorite);
    q.addBindValue(i.statut);
    q.addBindValue(iso(i.debut));
    q.addBindValue(i.fin.isValid() ? QVariant(iso(i.fin)) : nullStr());
    q.addBindValue(i.description);
    q.addBindValue(i.responsable.trimmed().isEmpty() ? nullStr() : QVariant(i.responsable.trimmed()));
    q.addBindValue(i.compteRendu.trimmed().isEmpty() ? nullStr() : QVariant(i.compteRendu.trimmed()));
    q.addBindValue(i.idZone);
    q.addBindValue(i.idEmploi > 0 ? QVariant(i.idEmploi) : nullInt());
    q.addBindValue(i.id);
    if (!q.exec()) return fail(err, q.lastError().text());

    QStringList ch;
    if (old.statut != i.statut) ch << QString("statut %1 → %2").arg(old.statut, i.statut);
    if (old.priorite != i.priorite) ch << QString("priorité %1 → %2").arg(old.priorite, i.priorite);
    if (old.idZone != i.idZone) ch << "zone modifiée";
    log(i.id, ch.isEmpty() ? QString("Modification") : "Modification : " + ch.join(", "));
    syncEquipements(i.id, i.statut);
    return true;
}

bool remove(int id, QString *err)
{
    Intervention i;
    if (!load(id, i)) return fail(err, "Intervention introuvable.");
    if (i.statut == "En cours" || i.statut == "Terminée")
        return fail(err, "Suppression interdite pour une intervention en cours ou terminée : "
                         "utilisez l'annulation afin de conserver la traçabilité.");

    QSqlQuery q;
    q.prepare("UPDATE equipement SET statut='Disponible' WHERE id_intervention=? AND statut='En mission'");
    q.addBindValue(id);
    q.exec();
    q.prepare("DELETE FROM intervention WHERE id_intervention=?");
    q.addBindValue(id);
    if (!q.exec()) return fail(err, q.lastError().text());
    return true;
}

// ------------------------------------------------------------------ métiers

// Score = 0,6 × vulnérabilité de la zone + bonus de type + bonus de délai.
// Le score ne dépend pas de la priorité actuelle : le recalcul est donc idempotent.
bool recalculerPriorite(int id, QString *message)
{
    Intervention i;
    if (!load(id, i)) return fail(message, "Intervention introuvable.");
    if (i.statut == "Terminée" || i.statut == "Annulée")
        return fail(message, "Intervention clôturée ou annulée : la priorité n'est pas recalculée.");

    QSqlQuery q;
    q.prepare("SELECT vulnerabilite FROM zone WHERE id_zone=?");
    q.addBindValue(i.idZone);
    q.exec();
    const double vuln = q.next() ? q.value(0).toDouble() : 0.0;

    double typeBonus = 0;
    if (i.type == "Inondation" || i.type == "Vent fort" || i.type == "Canicule") typeBonus = 20;
    else if (i.type == "Dépannage") typeBonus = 8;

    const QDateTime now = QDateTime::currentDateTime();
    double delaiBonus = 0;
    QString delai = "aucun";
    if (i.statut == "Planifiée" && i.debut < now) { delaiBonus = 15; delai = "début dépassé"; }
    else if (i.statut == "Planifiée" && i.debut < now.addSecs(24 * 3600)) { delaiBonus = 8; delai = "début dans moins de 24 h"; }
    else if (i.statut == "En cours" && i.fin.isValid() && i.fin < now) { delaiBonus = 15; delai = "échéance dépassée"; }

    const double score = 0.6 * vuln + typeBonus + delaiBonus;
    const QString np = score >= 75 ? "Critique" : score >= 55 ? "Haute" : score >= 35 ? "Moyenne" : "Faible";
    const QString detail = QString("Score %1 (vulnérabilité %2 × 0,6 + type %3 + délai %4 [%5]).")
                               .arg(score, 0, 'f', 1).arg(vuln, 0, 'f', 0).arg(typeBonus, 0, 'f', 0)
                               .arg(delaiBonus, 0, 'f', 0).arg(delai);

    if (np == i.priorite) {
        if (message) *message = QString("Priorité inchangée : %1.\n%2").arg(np, detail);
        return true;
    }
    q.prepare("UPDATE intervention SET priorite=? WHERE id_intervention=?");
    q.addBindValue(np);
    q.addBindValue(id);
    if (!q.exec()) return fail(message, q.lastError().text());
    log(id, QString("Recalcul de priorité : %1 → %2 (score %3)").arg(i.priorite, np).arg(score, 0, 'f', 1));
    if (message) *message = QString("Priorité ajustée : %1 → %2.\n%3").arg(i.priorite, np, detail);
    return true;
}

QList<int> enRetard(bool signaler)
{
    QList<int> ids;
    const QString now = iso(QDateTime::currentDateTime());
    QSqlQuery q;
    q.prepare("SELECT id_intervention FROM intervention WHERE "
              "(statut='En cours' AND date_fin IS NOT NULL AND date_fin < ?) OR "
              "(statut='Planifiée' AND date_debut < ?) ORDER BY date_debut");
    q.addBindValue(now);
    q.addBindValue(now);
    q.exec();
    while (q.next()) ids << q.value(0).toInt();

    if (!signaler) return ids;

    // Une seule trace d'escalade par intervention et par jour.
    const QString jour = iso(QDateTime(QDate::currentDate(), QTime(0, 0)));
    for (int id : ids) {
        QSqlQuery c;
        c.prepare("SELECT 1 FROM historique_intervention WHERE id_intervention=? "
                  "AND evenement LIKE 'Escalade%' AND date_evt >= ?");
        c.addBindValue(id);
        c.addBindValue(jour);
        c.exec();
        if (!c.next()) log(id, "Escalade de retard signalée au responsable");
    }
    return ids;
}

Prerequis verifierPrerequis(int id)
{
    Prerequis r;
    Intervention i;
    if (!load(id, i)) { r.problemes << "Intervention introuvable."; return r; }
    if (i.statut == "Terminée" || i.statut == "Annulée") {
        r.problemes << QString("L'intervention est déjà %1.").arg(i.statut.toLower());
        return r;
    }

    QSqlQuery q;
    q.prepare("SELECT statut FROM zone WHERE id_zone=?");
    q.addBindValue(i.idZone);
    q.exec();
    if (!q.next() || q.value(0).toString() != "Active") r.problemes << "La zone n'est pas active.";

    if (i.idEmploi == 0 && i.responsable.trimmed().isEmpty()) {
        r.problemes << "Aucun emploi ni responsable affecté.";
    } else if (i.idEmploi != 0) {
        q.prepare("SELECT statut FROM emploi WHERE id_emploi=?");
        q.addBindValue(i.idEmploi);
        q.exec();
        if (!q.next()) r.problemes << "L'emploi lié est introuvable.";
        else if (q.value(0).toString() == "Annulé") r.problemes << QString("L'emploi lié #%1 est annulé.").arg(i.idEmploi);
    }

    q.prepare("SELECT reference, statut FROM equipement WHERE id_intervention=?");
    q.addBindValue(id);
    q.exec();
    int nb = 0;
    while (q.next()) {
        ++nb;
        const QString st = q.value(1).toString();
        if (st == "Maintenance" || st == "Retiré")
            r.problemes << QString("L'équipement %1 est indisponible (%2).").arg(q.value(0).toString(), st);
    }
    if (nb == 0) r.problemes << "Aucun équipement affecté.";

    r.pret = r.problemes.isEmpty();
    return r;
}

Estimation estimerDuree(const QString &type, int idZone)
{
    Estimation e;
    const QString base =
        "SELECT AVG((julianday(date_fin)-julianday(date_debut))*24.0), COUNT(*) FROM intervention "
        "WHERE statut='Terminée' AND date_fin IS NOT NULL AND type=?";
    for (int pass = 0; pass < 2; ++pass) {
        QSqlQuery q;
        q.prepare(base + (pass == 0 ? " AND id_zone=?" : ""));
        q.addBindValue(type);
        if (pass == 0) q.addBindValue(idZone);
        q.exec();
        if (q.next() && q.value(1).toInt() > 0) {
            e.heures = q.value(0).toDouble();
            e.echantillon = q.value(1).toInt();
            e.memeZone = (pass == 0);
            return e;
        }
    }
    return e;
}

bool cloturer(int id, const QString &compteRendu, QString *err)
{
    Intervention i;
    if (!load(id, i)) return fail(err, "Intervention introuvable.");
    if (i.statut == "Terminée") return fail(err, "L'intervention est déjà terminée.");
    if (i.statut == "Annulée") return fail(err, "Une intervention annulée ne peut pas être clôturée.");
    i.compteRendu = compteRendu;
    i.statut = "Terminée";
    if (!i.fin.isValid() || i.fin < i.debut) i.fin = QDateTime::currentDateTime();
    if (i.fin < i.debut) i.fin = i.debut;
    return update(i, err);
}

// ------------------------------------------------------------------ équipements

bool affecterEquipement(int idIntervention, int idEquipement, QString *err)
{
    Intervention i;
    if (!load(idIntervention, i)) return fail(err, "Intervention introuvable.");
    if (i.statut == "Terminée" || i.statut == "Annulée")
        return fail(err, "Impossible d'affecter un équipement à une intervention terminée ou annulée.");

    QSqlQuery q;
    q.prepare("SELECT reference, statut FROM equipement WHERE id_equipement=?");
    q.addBindValue(idEquipement);
    if (!q.exec() || !q.next()) return fail(err, "Équipement introuvable.");
    const QString ref = q.value(0).toString();
    const QString st = q.value(1).toString();
    if (st != "Disponible")
        return fail(err, QString("Affectation refusée : l'équipement %1 est « %2 ».").arg(ref, st));

    q.prepare("UPDATE equipement SET statut='En mission', id_intervention=? WHERE id_equipement=?");
    q.addBindValue(idIntervention);
    q.addBindValue(idEquipement);
    if (!q.exec()) return fail(err, q.lastError().text());
    log(idIntervention, QString("Équipement %1 affecté").arg(ref));
    return true;
}

// ------------------------------------------------------------------ référence / stats

IdLabel zones(bool actives)
{
    IdLabel out;
    QSqlQuery q("SELECT id_zone, nom, statut FROM zone ORDER BY nom");
    while (q.next()) {
        const bool ok = q.value(2).toString() == "Active";
        if (actives && !ok) continue;
        out.append({q.value(0).toInt(), q.value(1).toString() + (ok ? "" : " (inactive)")});
    }
    return out;
}

IdLabel emplois()
{
    IdLabel out;
    QSqlQuery q("SELECT e.id_emploi, a.prenom || ' ' || a.nom, e.date_debut, e.statut "
                "FROM emploi e JOIN agent a ON a.id_agent=e.id_agent WHERE e.statut<>'Annulé' ORDER BY e.date_debut");
    while (q.next())
        out.append({q.value(0).toInt(), QString("#%1 — %2 — %3 (%4)").arg(q.value(0).toInt())
                                            .arg(q.value(1).toString(), jolie(q.value(2).toString()), q.value(3).toString())});
    return out;
}

IdLabel equipements()
{
    IdLabel out;
    QSqlQuery q("SELECT id_equipement, reference, type, statut FROM equipement ORDER BY reference");
    while (q.next())
        out.append({q.value(0).toInt(), QString("%1 — %2 [%3]").arg(q.value(1).toString(), q.value(2).toString(), q.value(3).toString())});
    return out;
}

QString zoneNom(int idZone)
{
    QSqlQuery q;
    q.prepare("SELECT nom FROM zone WHERE id_zone=?");
    q.addBindValue(idZone);
    q.exec();
    return q.next() ? q.value(0).toString() : QString("?");
}

QStringList historique(int idIntervention)
{
    QStringList out;
    QSqlQuery q;
    q.prepare("SELECT date_evt, evenement FROM historique_intervention WHERE id_intervention=? ORDER BY date_evt DESC, id DESC");
    q.addBindValue(idIntervention);
    q.exec();
    while (q.next()) out << jolie(q.value(0).toString()) + "  —  " + q.value(1).toString();
    return out;
}

Counts parStatut() { return countBy("SELECT statut, COUNT(*) FROM intervention GROUP BY statut", statuts()); }
Counts parPriorite() { return countBy("SELECT priorite, COUNT(*) FROM intervention GROUP BY priorite", priorites()); }
Counts parZone()
{
    return countBy("SELECT z.nom, COUNT(i.id_intervention) FROM zone z LEFT JOIN intervention i ON i.id_zone=z.id_zone "
                   "WHERE z.statut='Active' GROUP BY z.id_zone", QStringList());
}

int total() { return scalar("SELECT COUNT(*) FROM intervention"); }
int enCours() { return scalar("SELECT COUNT(*) FROM intervention WHERE statut='En cours'"); }
int critiques() { return scalar("SELECT COUNT(*) FROM intervention WHERE priorite='Critique' AND statut IN ('Planifiée','En cours')"); }

} // namespace InterventionService
