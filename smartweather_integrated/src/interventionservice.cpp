#include "interventionservice.h"

#include <QMetaType>
#include <QObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QVariantMap>
#include <algorithm>

namespace {

const QString kSelect = QStringLiteral(
    "SELECT i.id_intervention, i.type, i.priorite, i.statut, i.date_debut, i.date_fin,"
    " i.description, i.responsable, i.compte_rendu, i.id_zone, COALESCE(i.id_emploi, 0), z.nom"
    " FROM intervention i JOIN zone z ON z.id_zone = i.id_zone ");

QString iso(const QDateTime &d) { return d.toString(Qt::ISODate); }

QVariant nullableId(int id)
{
    return id > 0 ? QVariant(id) : QVariant(QMetaType::fromType<int>());
}

Intervention fromQuery(const QSqlQuery &q)
{
    Intervention i;
    i.id          = q.value(0).toInt();
    i.type        = q.value(1).toString();
    i.priorite    = q.value(2).toString();
    i.statut      = q.value(3).toString();
    i.debut       = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
    i.fin         = QDateTime::fromString(q.value(5).toString(), Qt::ISODate);
    i.description = q.value(6).toString();
    i.responsable = q.value(7).toString();
    i.compteRendu = q.value(8).toString();
    i.idZone      = q.value(9).toInt();
    i.idEmploi    = q.value(10).toInt();
    i.zoneNom     = q.value(11).toString();
    return i;
}

// Construit les conditions WHERE communes à la liste et aux statistiques.
void buildWhere(const InterventionFilter &f, QStringList &cond, QVariantMap &binds)
{
    if (!f.texte.trimmed().isEmpty()) {
        cond << QStringLiteral(
            "(CAST(i.id_intervention AS TEXT) LIKE :t OR i.type LIKE :t OR i.description LIKE :t"
            " OR i.responsable LIKE :t OR i.statut LIKE :t OR i.priorite LIKE :t OR z.nom LIKE :t)");
        binds[":t"] = QStringLiteral("%") + f.texte.trimmed() + QStringLiteral("%");
    }
    if (!f.statut.isEmpty())   { cond << "i.statut = :statut";     binds[":statut"] = f.statut; }
    if (!f.priorite.isEmpty()) { cond << "i.priorite = :priorite"; binds[":priorite"] = f.priorite; }
    if (!f.type.isEmpty())     { cond << "i.type = :type";         binds[":type"] = f.type; }
    if (f.idZone > 0)          { cond << "i.id_zone = :zone";      binds[":zone"] = f.idZone; }
    if (f.periode && f.du.isValid() && f.au.isValid()) {
        cond << "i.date_debut >= :du AND i.date_debut <= :au";
        binds[":du"] = iso(f.du.startOfDay());
        binds[":au"] = iso(f.au.endOfDay());
    }
}

QString whereClause(const QStringList &cond)
{
    return cond.isEmpty() ? QString() : QStringLiteral(" WHERE ") + cond.join(QStringLiteral(" AND "));
}

void bindAll(QSqlQuery &q, const QVariantMap &binds)
{
    for (auto it = binds.cbegin(); it != binds.cend(); ++it)
        q.bindValue(it.key(), it.value());
}

QString messageErreurDate() { return QObject::tr("La date de fin ne peut pas précéder la date de début."); }

} // namespace

// ============================================================ valeurs contrôlées

const QStringList &InterventionService::types()
{
    static const QStringList t = {QStringLiteral("Inondation"), QStringLiteral("Chaleur"),
                                  QStringLiteral("Vent fort"), QStringLiteral("Maintenance station"),
                                  QStringLiteral("Inspection"), QStringLiteral("Autre")};
    return t;
}

const QStringList &InterventionService::priorites()
{
    static const QStringList p = {Priorite::Faible, Priorite::Moyenne, Priorite::Haute, Priorite::Critique};
    return p;
}

const QStringList &InterventionService::statuts()
{
    static const QStringList s = {Statut::Planifiee, Statut::EnCours, Statut::Terminee, Statut::Annulee};
    return s;
}

QStringList InterventionService::transitionsFrom(const QString &statut)
{
    if (statut == Statut::Planifiee) return {Statut::Planifiee, Statut::EnCours, Statut::Annulee};
    if (statut == Statut::EnCours)   return {Statut::EnCours, Statut::Terminee, Statut::Annulee};
    if (statut == Statut::Terminee)  return {Statut::Terminee, Statut::EnCours};   // réouverture justifiée
    return {statut};                                                               // Annulée : définitif
}

// ============================================================ validations

QStringList InterventionService::validate(const Intervention &i, const Intervention *ancien)
{
    QStringList e;

    if (!types().contains(i.type))         e << QObject::tr("Le type d'intervention est invalide.");
    if (!priorites().contains(i.priorite)) e << QObject::tr("La priorité doit appartenir à la liste contrôlée.");
    if (!statuts().contains(i.statut))     e << QObject::tr("Le statut est invalide.");

    // Statut / cycle de vie
    if (!ancien && i.statut != Statut::Planifiee)
        e << QObject::tr("Une nouvelle intervention commence au statut « Planifiée ».");
    if (ancien && ancien->statut != i.statut && !transitionsFrom(ancien->statut).contains(i.statut))
        e << QObject::tr("Transition de statut interdite : %1 → %2.").arg(ancien->statut, i.statut);

    // Zone active obligatoire
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT statut FROM zone WHERE id_zone = ?"));
    q.addBindValue(i.idZone);
    q.exec();
    if (!q.next()) {
        e << QObject::tr("La zone sélectionnée est introuvable.");
    } else if (q.value(0).toString() != QLatin1String("Active")) {
        const bool tolere = ancien && ancien->idZone == i.idZone
                            && (i.statut == Statut::Terminee || i.statut == Statut::Annulee);
        if (!tolere)
            e << QObject::tr("L'intervention doit être reliée à une zone active.");
    }

    // Dates
    if (!i.debut.isValid() || !i.fin.isValid())
        e << QObject::tr("Les dates de début et de fin sont obligatoires.");
    else if (i.fin < i.debut)
        e << messageErreurDate();

    // Une intervention en cours doit avoir un responsable ou un emploi
    if (i.statut == Statut::EnCours && i.responsable.trimmed().isEmpty() && i.idEmploi <= 0)
        e << QObject::tr("Une intervention en cours doit avoir un responsable ou un emploi affecté.");

    // Emploi lié
    if (i.idEmploi > 0) {
        QSqlQuery em;
        em.prepare(QStringLiteral("SELECT statut FROM emploi WHERE id_emploi = ?"));
        em.addBindValue(i.idEmploi);
        em.exec();
        if (!em.next())
            e << QObject::tr("L'emploi lié est introuvable.");
        else if (em.value(0).toString() == QLatin1String("Annulé"))
            e << QObject::tr("L'emploi lié est annulé.");
    }

    // Clôture
    if (i.statut == Statut::Terminee)
        e += controleCloture(i);

    e.removeDuplicates();
    return e;
}

// ============================================================ CRUD

bool InterventionService::add(Intervention &i, QString *err)
{
    const QStringList errs = validate(i, nullptr);
    if (!errs.isEmpty()) {
        if (err) *err = errs.join(QLatin1Char('\n'));
        return false;
    }
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "INSERT INTO intervention(type, priorite, statut, date_debut, date_fin, description,"
        " responsable, compte_rendu, id_zone, id_emploi) VALUES(?,?,?,?,?,?,?,?,?,?)"));
    q.addBindValue(i.type);
    q.addBindValue(i.priorite);
    q.addBindValue(i.statut);
    q.addBindValue(iso(i.debut));
    q.addBindValue(iso(i.fin));
    q.addBindValue(i.description);
    q.addBindValue(i.responsable);
    q.addBindValue(i.compteRendu);
    q.addBindValue(i.idZone);
    q.addBindValue(nullableId(i.idEmploi));
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return false;
    }
    i.id = q.lastInsertId().toInt();
    log(i.id, QObject::tr("Création"),
        QObject::tr("%1, priorité %2, statut %3").arg(i.type, i.priorite, i.statut));
    return true;
}

bool InterventionService::update(const Intervention &i, const QString &justification, QString *err)
{
    const auto ancien = get(i.id);
    if (!ancien) {
        if (err) *err = QObject::tr("Intervention introuvable.");
        return false;
    }
    const QStringList errs = validate(i, &*ancien);
    if (!errs.isEmpty()) {
        if (err) *err = errs.join(QLatin1Char('\n'));
        return false;
    }
    const bool reouverture = ancien->statut == Statut::Terminee && i.statut != Statut::Terminee;
    if (reouverture && justification.trimmed().isEmpty()) {
        if (err) *err = QObject::tr("La réouverture d'une intervention terminée exige une justification.");
        return false;
    }

    QSqlQuery q;
    q.prepare(QStringLiteral(
        "UPDATE intervention SET type=?, priorite=?, statut=?, date_debut=?, date_fin=?, description=?,"
        " responsable=?, compte_rendu=?, id_zone=?, id_emploi=? WHERE id_intervention=?"));
    q.addBindValue(i.type);
    q.addBindValue(i.priorite);
    q.addBindValue(i.statut);
    q.addBindValue(iso(i.debut));
    q.addBindValue(iso(i.fin));
    q.addBindValue(i.description);
    q.addBindValue(i.responsable);
    q.addBindValue(i.compteRendu);
    q.addBindValue(i.idZone);
    q.addBindValue(nullableId(i.idEmploi));
    q.addBindValue(i.id);
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return false;
    }

    // Historisation des changements importants
    QStringList diff;
    if (ancien->statut != i.statut)     diff << QObject::tr("statut %1 → %2").arg(ancien->statut, i.statut);
    if (ancien->priorite != i.priorite) diff << QObject::tr("priorité %1 → %2").arg(ancien->priorite, i.priorite);
    if (ancien->idZone != i.idZone)     diff << QObject::tr("zone #%1 → #%2").arg(ancien->idZone).arg(i.idZone);
    if (ancien->idEmploi != i.idEmploi) diff << QObject::tr("emploi #%1 → #%2").arg(ancien->idEmploi).arg(i.idEmploi);
    if (ancien->responsable != i.responsable) diff << QObject::tr("responsable modifié");
    if (ancien->debut != i.debut || ancien->fin != i.fin) diff << QObject::tr("dates modifiées");
    if (diff.isEmpty()) diff << QObject::tr("autres champs modifiés");
    log(i.id, reouverture ? QObject::tr("Réouverture") : QObject::tr("Modification"),
        diff.join(QStringLiteral(" ; ")) + (reouverture ? QObject::tr(" — justification : %1").arg(justification.trimmed()) : QString()));

    if (i.statut == Statut::Terminee || i.statut == Statut::Annulee)
        libererEquipements(i.id);
    return true;
}

bool InterventionService::remove(int id, QString *err)
{
    const auto cur = get(id);
    if (!cur) {
        if (err) *err = QObject::tr("Intervention introuvable.");
        return false;
    }
    if (cur->statut == Statut::EnCours || cur->statut == Statut::Terminee) {
        if (err) *err = QObject::tr("Suppression interdite pour une intervention « %1 » : utilisez l'annulation afin de conserver la traçabilité.").arg(cur->statut);
        return false;
    }
    libererEquipements(id);
    QSqlQuery q;
    q.prepare(QStringLiteral("DELETE FROM intervention WHERE id_intervention = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return false;
    }
    log(id, QObject::tr("Suppression"), QObject::tr("%1 (%2)").arg(cur->type, cur->statut));
    return true;
}

bool InterventionService::cancel(int id, QString *err)
{
    auto cur = get(id);
    if (!cur) {
        if (err) *err = QObject::tr("Intervention introuvable.");
        return false;
    }
    if (!transitionsFrom(cur->statut).contains(Statut::Annulee) || cur->statut == Statut::Annulee) {
        if (err) *err = QObject::tr("Une intervention « %1 » ne peut pas être annulée.").arg(cur->statut);
        return false;
    }
    QSqlQuery q;
    q.prepare(QStringLiteral("UPDATE intervention SET statut = ? WHERE id_intervention = ?"));
    q.addBindValue(Statut::Annulee);
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return false;
    }
    libererEquipements(id);
    log(id, QObject::tr("Annulation"), QObject::tr("statut %1 → %2").arg(cur->statut, Statut::Annulee));
    return true;
}

std::optional<Intervention> InterventionService::get(int id)
{
    QSqlQuery q;
    q.prepare(kSelect + QStringLiteral("WHERE i.id_intervention = ?"));
    q.addBindValue(id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}

QVector<Intervention> InterventionService::list(const InterventionFilter &f, QString *err)
{
    QStringList cond;
    QVariantMap binds;
    buildWhere(f, cond, binds);

    QSqlQuery q;
    q.prepare(kSelect + whereClause(cond) + QStringLiteral(" ORDER BY i.date_debut DESC"));
    bindAll(q, binds);

    QVector<Intervention> out;
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return out;
    }
    while (q.next())
        out.push_back(fromQuery(q));
    return out;
}

// ============================================================ métiers avancés

// Métier 1 — Recalcul de priorité : risque météo (règles 4.3) + vulnérabilité de la zone + retard.
PriorityResult InterventionService::recalculerPriorite(int id, bool appliquer)
{
    PriorityResult r;
    const auto cur = get(id);
    if (!cur) {
        r.explication = QObject::tr("Intervention introuvable.");
        return r;
    }
    r.ancienne = r.nouvelle = cur->priorite;
    if (cur->statut == Statut::Terminee || cur->statut == Statut::Annulee) {
        r.explication = QObject::tr("Recalcul non applicable : l'intervention est %1.").arg(cur->statut.toLower());
        return r;
    }

    QSqlQuery z;
    z.prepare(QStringLiteral("SELECT vulnerabilite FROM zone WHERE id_zone = ?"));
    z.addBindValue(cur->idZone);
    z.exec();
    const double vuln = z.next() ? z.value(0).toDouble() : 0.0;

    // Dernier relevé de chaque station de la zone
    bool chaleur = false, vent = false, inondation = false;
    QSqlQuery rel;
    rel.prepare(QStringLiteral(
        "SELECT r.temperature, r.humidite, r.pluie, r.vent FROM station s"
        " JOIN releve r ON r.id_releve = (SELECT r2.id_releve FROM releve r2"
        "   WHERE r2.id_station = s.id_station ORDER BY r2.horodatage DESC LIMIT 1)"
        " WHERE s.id_zone = ?"));
    rel.addBindValue(cur->idZone);
    rel.exec();
    while (rel.next()) {
        if (rel.value(0).toDouble() >= 40.0) chaleur = true;
        if (rel.value(3).toDouble() >= 80.0) vent = true;
        if (rel.value(2).toDouble() >= 50.0 && rel.value(1).toDouble() >= 85.0) inondation = true;
    }
    const int nbRisques = int(chaleur) + int(vent) + int(inondation);

    const QDateTime now = QDateTime::currentDateTime();
    const int retardJours = (cur->fin.isValid() && cur->fin < now) ? int(cur->fin.daysTo(now)) : 0;

    int score = nbRisques * 25 + qRound(vuln * 0.4) + std::min(retardJours, 5) * 8;
    if (nbRisques >= 2)
        score = std::max(score, 70);          // plusieurs risques simultanés => situation critique
    score = std::min(score, 100);

    QString niveau = Priorite::Faible;
    if (score >= 70)      niveau = Priorite::Critique;
    else if (score >= 45) niveau = Priorite::Haute;
    else if (score >= 20) niveau = Priorite::Moyenne;

    QStringList raisons;
    if (chaleur)    raisons << QObject::tr("chaleur ≥ 40 °C");
    if (vent)       raisons << QObject::tr("vent ≥ 80 km/h");
    if (inondation) raisons << QObject::tr("risque d'inondation");
    if (raisons.isEmpty()) raisons << QObject::tr("aucun risque météo actif");

    r.score = score;
    r.nouvelle = niveau;
    r.changee = (niveau != cur->priorite);
    r.explication = QObject::tr("Score %1/100 — %2 ; vulnérabilité zone %3 ; retard %4 j.")
                        .arg(score).arg(raisons.join(QStringLiteral(", ")))
                        .arg(vuln, 0, 'f', 0).arg(retardJours);

    if (appliquer && r.changee) {
        QSqlQuery u;
        u.prepare(QStringLiteral("UPDATE intervention SET priorite = ? WHERE id_intervention = ?"));
        u.addBindValue(niveau);
        u.addBindValue(id);
        u.exec();
        log(id, QObject::tr("Recalcul de priorité"),
            QObject::tr("%1 → %2. %3").arg(r.ancienne, niveau, r.explication));
    }
    return r;
}

// Métier 2 — Escalade de retard : missions non terminées dont la date de fin est dépassée.
QVector<Intervention> InterventionService::interventionsEnRetard()
{
    QSqlQuery q;
    q.prepare(kSelect + QStringLiteral(
        "WHERE i.statut IN (?, ?) AND i.date_fin < ? ORDER BY i.date_fin ASC"));
    q.addBindValue(Statut::Planifiee);
    q.addBindValue(Statut::EnCours);
    q.addBindValue(iso(QDateTime::currentDateTime()));
    QVector<Intervention> out;
    if (q.exec())
        while (q.next())
            out.push_back(fromQuery(q));
    return out;
}

// Métier 3 — Vérification des prérequis (emploi + équipements). Liste vide = mission prête.
QStringList InterventionService::prerequis(const Intervention &i)
{
    QStringList p;

    if (i.idEmploi <= 0) {
        p << QObject::tr("Aucun emploi n'est lié à l'intervention.");
    } else {
        QSqlQuery q;
        q.prepare(QStringLiteral(
            "SELECT e.statut, a.statut, a.prenom || ' ' || a.nom FROM emploi e"
            " JOIN agent a ON a.id_agent = e.id_agent WHERE e.id_emploi = ?"));
        q.addBindValue(i.idEmploi);
        q.exec();
        if (!q.next()) {
            p << QObject::tr("L'emploi lié est introuvable.");
        } else {
            const QString st = q.value(0).toString();
            if (st != QLatin1String("Planifié") && st != QLatin1String("En cours"))
                p << QObject::tr("L'emploi #%1 est « %2 » (Planifié ou En cours requis).").arg(i.idEmploi).arg(st);
            if (q.value(1).toString() != QLatin1String("disponible"))
                p << QObject::tr("L'agent %1 est indisponible.").arg(q.value(2).toString());
        }
    }

    QSqlQuery eq;
    eq.prepare(QStringLiteral("SELECT reference, statut FROM equipement WHERE id_intervention = ?"));
    eq.addBindValue(i.id);
    eq.exec();
    int nb = 0;
    while (eq.next()) {
        ++nb;
        const QString st = eq.value(1).toString();
        if (st == QLatin1String("Maintenance") || st == QLatin1String("Retiré"))
            p << QObject::tr("L'équipement %1 est indisponible (%2).").arg(eq.value(0).toString(), st);
    }
    if (nb == 0)
        p << QObject::tr("Aucun équipement n'est affecté à l'intervention.");
    return p;
}

// Métier 4 — Estimation de durée à partir des interventions terminées similaires.
DurationEstimate InterventionService::estimerDuree(const QString &type, int idZone)
{
    struct Niveau { QString sql; QVariantList args; QString base; int minimum; };
    const QString base = QStringLiteral(
        "SELECT (julianday(date_fin) - julianday(date_debut)) * 24.0 FROM intervention WHERE statut = ?");
    const QVector<Niveau> niveaux = {
        {base + " AND type = ? AND id_zone = ?", {Statut::Terminee, type, idZone},
         QObject::tr("même type et même zone"), 2},
        {base + " AND type = ?", {Statut::Terminee, type}, QObject::tr("même type"), 1},
        {base, {Statut::Terminee}, QObject::tr("toutes interventions terminées"), 1},
    };

    for (const Niveau &n : niveaux) {
        QSqlQuery q;
        q.prepare(n.sql);
        for (const QVariant &a : n.args)
            q.addBindValue(a);
        q.exec();
        double somme = 0;
        int nb = 0;
        while (q.next()) {
            somme += q.value(0).toDouble();
            ++nb;
        }
        if (nb >= n.minimum) {
            DurationEstimate d;
            d.heures = somme / nb;
            d.echantillon = nb;
            d.base = n.base;
            return d;
        }
    }
    return {};
}

// Métier 5 — Contrôle de clôture : compte-rendu complet + dates cohérentes.
QStringList InterventionService::controleCloture(const Intervention &i)
{
    QStringList e;
    if (i.compteRendu.trimmed().size() < 20)
        e << QObject::tr("Le compte-rendu est obligatoire (20 caractères minimum) pour clôturer.");
    if (!i.debut.isValid() || !i.fin.isValid())
        e << QObject::tr("Les dates de début et de fin sont obligatoires.");
    else {
        if (i.fin < i.debut)
            e << messageErreurDate();
        if (i.fin > QDateTime::currentDateTime().addSecs(60))
            e << QObject::tr("La date de fin réelle ne peut pas être dans le futur.");
    }
    return e;
}

bool InterventionService::cloturer(int id, const QString &compteRendu, const QDateTime &fin, QString *err)
{
    auto cur = get(id);
    if (!cur) {
        if (err) *err = QObject::tr("Intervention introuvable.");
        return false;
    }
    if (cur->statut != Statut::EnCours) {
        if (err) *err = QObject::tr("Seule une intervention « En cours » peut être clôturée (statut actuel : %1).").arg(cur->statut);
        return false;
    }
    Intervention nouvelle = *cur;
    nouvelle.statut = Statut::Terminee;
    nouvelle.compteRendu = compteRendu.trimmed();
    nouvelle.fin = fin;

    const QStringList errs = validate(nouvelle, &*cur);
    if (!errs.isEmpty()) {
        if (err) *err = errs.join(QLatin1Char('\n'));
        return false;
    }
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "UPDATE intervention SET statut = ?, compte_rendu = ?, date_fin = ? WHERE id_intervention = ?"));
    q.addBindValue(nouvelle.statut);
    q.addBindValue(nouvelle.compteRendu);
    q.addBindValue(iso(nouvelle.fin));
    q.addBindValue(id);
    if (!q.exec()) {
        if (err) *err = q.lastError().text();
        return false;
    }
    libererEquipements(id);
    log(id, QObject::tr("Clôture"), QObject::tr("terminée le %1").arg(nouvelle.fin.toString(QStringLiteral("dd/MM/yyyy HH:mm"))));
    return true;
}

// ============================================================ statistiques / historique / références

Counts InterventionService::repartition(const QString &dimension, const InterventionFilter &f)
{
    QString expr = QStringLiteral("i.statut");
    bool parMois = false;
    if (dimension == QLatin1String("priorite")) expr = QStringLiteral("i.priorite");
    else if (dimension == QLatin1String("type")) expr = QStringLiteral("i.type");
    else if (dimension == QLatin1String("zone")) expr = QStringLiteral("z.nom");
    else if (dimension == QLatin1String("mois")) { expr = QStringLiteral("strftime('%Y-%m', i.date_debut)"); parMois = true; }

    InterventionFilter filtre;       // seule la période s'applique aux statistiques
    filtre.periode = f.periode; filtre.du = f.du; filtre.au = f.au;
    QStringList cond;
    QVariantMap binds;
    buildWhere(filtre, cond, binds);

    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT ") + expr + QStringLiteral(" AS k, COUNT(*) FROM intervention i"
              " JOIN zone z ON z.id_zone = i.id_zone") + whereClause(cond)
              + QStringLiteral(" GROUP BY k ORDER BY ")
              + (parMois ? QStringLiteral("k ASC") : QStringLiteral("COUNT(*) DESC, k ASC")));
    bindAll(q, binds);

    Counts out;
    if (q.exec())
        while (q.next())
            out.push_back({q.value(0).toString(), q.value(1).toInt()});
    return out;
}

QVector<HistoryEntry> InterventionService::historique(int id)
{
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "SELECT date_evt, action, detail FROM historique_intervention"
        " WHERE id_intervention = ? ORDER BY id_histo DESC"));
    q.addBindValue(id);
    QVector<HistoryEntry> out;
    if (q.exec())
        while (q.next())
            out.push_back({QDateTime::fromString(q.value(0).toString(), Qt::ISODate),
                           q.value(1).toString(), q.value(2).toString()});
    return out;
}

QVector<ZoneItem> InterventionService::zones()
{
    QVector<ZoneItem> out;
    QSqlQuery q(QStringLiteral("SELECT id_zone, nom, statut FROM zone ORDER BY nom"));
    while (q.next())
        out.push_back({q.value(0).toInt(), q.value(1).toString(), q.value(2).toString() == QLatin1String("Active")});
    return out;
}

QVector<EmploiItem> InterventionService::emplois()
{
    QVector<EmploiItem> out;
    QSqlQuery q(QStringLiteral(
        "SELECT e.id_emploi, a.prenom || ' ' || a.nom, e.date_debut, e.date_fin, e.statut"
        " FROM emploi e JOIN agent a ON a.id_agent = e.id_agent"
        " WHERE e.statut <> 'Annulé' ORDER BY e.date_debut"));
    while (q.next()) {
        const QString d1 = QDateTime::fromString(q.value(2).toString(), Qt::ISODate).toString(QStringLiteral("dd/MM HH:mm"));
        const QString d2 = QDateTime::fromString(q.value(3).toString(), Qt::ISODate).toString(QStringLiteral("dd/MM HH:mm"));
        out.push_back({q.value(0).toInt(),
                       QStringLiteral("#%1 — %2 (%3 → %4) [%5]")
                           .arg(q.value(0).toInt()).arg(q.value(1).toString(), d1, d2, q.value(4).toString())});
    }
    return out;
}

// ============================================================ interne

void InterventionService::log(int id, const QString &action, const QString &detail)
{
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "INSERT INTO historique_intervention(id_intervention, date_evt, action, detail) VALUES(?,?,?,?)"));
    q.addBindValue(id);
    q.addBindValue(iso(QDateTime::currentDateTime()));
    q.addBindValue(action);
    q.addBindValue(detail);
    q.exec();
}

void InterventionService::libererEquipements(int idIntervention)
{
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "UPDATE equipement SET id_intervention = NULL,"
        " statut = CASE WHEN statut = 'En mission' THEN 'Disponible' ELSE statut END"
        " WHERE id_intervention = ?"));
    q.addBindValue(idIntervention);
    q.exec();
}
