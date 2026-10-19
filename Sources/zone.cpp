#include "zone.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

// ------------------------------------------------------------
//  Messages d'erreur lisibles (codes Oracle courants)
// ------------------------------------------------------------
QString messageErreur(const QSqlError &e)
{
    const QString txt = e.text();
    auto has = [&](const char *ora, const char *nativeCode) {
        return txt.contains(QLatin1String(ora)) || e.nativeErrorCode() == QLatin1String(nativeCode);
    };
    if (has("ORA-00001", "1"))
        return QStringLiteral("Ce code de zone existe déjà : le code doit être unique.");
    if (has("ORA-02292", "2292"))
        return QStringLiteral("Des données (stations, interventions...) dépendent de cette zone.");
    if (has("ORA-02290", "2290"))
        return QStringLiteral("Une valeur ne respecte pas les contraintes de la base de données.");
    if (!QSqlDatabase::database().isOpen())
        return QStringLiteral("La base de données n'est pas connectée.");
    return txt.isEmpty() ? QStringLiteral("Erreur inconnue de la base de données.") : txt;
}

bool fail(QString *erreur, const QSqlError &e)
{
    if (erreur) *erreur = messageErreur(e);
    return false;
}

// Execute une requete deja preparee/liee ; en cas d'echec, remplit "erreur".
bool run(QSqlQuery &q, QString *erreur)
{
    if (q.exec()) return true;
    return fail(erreur, q.lastError());
}

// ------------------------------------------------------------
//  Transaction RAII : rollback automatique si commit() n'est pas appele.
//  (si le pilote ne gere pas les transactions, on continue sans)
// ------------------------------------------------------------
class Transaction
{
public:
    Transaction() : m_db(QSqlDatabase::database()), m_active(m_db.isOpen() && m_db.transaction()) {}
    ~Transaction() { if (m_active) m_db.rollback(); }
    bool commit()
    {
        if (!m_active) return true;
        m_active = false;
        return m_db.commit();
    }
private:
    QSqlDatabase m_db;
    bool m_active;
};

// ------------------------------------------------------------
//  Historique
// ------------------------------------------------------------
bool logChange(int idZone, const QString &champ, const QString &ancien, const QString &nouveau,
               QString *erreur)
{
    QSqlQuery q;
    q.prepare(QStringLiteral("INSERT INTO ZONE_HISTORIQUE (ID_ZONE, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR) "
                             "VALUES (:id, :champ, :ancien, :nouveau)"));
    q.bindValue(QStringLiteral(":id"), idZone);
    q.bindValue(QStringLiteral(":champ"), champ);
    q.bindValue(QStringLiteral(":ancien"), ancien);
    q.bindValue(QStringLiteral(":nouveau"), nouveau);
    return run(q, erreur);
}

Zone lireZone(const QSqlQuery &q)
{
    Zone z;
    z.id = q.value(0).toInt();
    z.nom = q.value(1).toString();
    z.code = q.value(2).toString();
    z.vulnerabilite = q.value(3).toDouble();
    z.latitude = q.value(4).toDouble();
    z.longitude = q.value(5).toDouble();
    z.statut = q.value(6).toString();
    return z;
}

QString num(double v) { return QString::number(v); }

} // namespace

// ============================================================
//  CREATE
// ============================================================
bool Zone::ajouter(QString *erreur)
{
    Transaction tx;

    // 1) Nouvelle cle primaire (sequence Oracle) : on la garde pour l'historique
    QSqlQuery seq;
    seq.prepare(QStringLiteral("SELECT SEQ_ZONE.NEXTVAL FROM DUAL"));
    if (!run(seq, erreur)) return false;
    if (!seq.next()) return fail(erreur, seq.lastError());
    const int nouvelId = seq.value(0).toInt();

    // 2) Insertion (requete preparee)
    QSqlQuery q;
    q.prepare(QStringLiteral("INSERT INTO ZONE (ID_ZONE, NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT) "
                             "VALUES (:id, :nom, :code, :vuln, :lat, :lon, :statut)"));
    q.bindValue(QStringLiteral(":id"), nouvelId);
    q.bindValue(QStringLiteral(":nom"), nom);
    q.bindValue(QStringLiteral(":code"), code);
    q.bindValue(QStringLiteral(":vuln"), vulnerabilite);
    q.bindValue(QStringLiteral(":lat"), latitude);
    q.bindValue(QStringLiteral(":lon"), longitude);
    q.bindValue(QStringLiteral(":statut"), statut);
    if (!run(q, erreur)) return false;

    // 3) Historique (metier 5)
    if (!logChange(nouvelId, QStringLiteral("Création"), QString(), nom, erreur)) return false;

    if (!tx.commit()) return fail(erreur, QSqlDatabase::database().lastError());
    id = nouvelId;
    return true;
}

// ============================================================
//  READ
// ============================================================
QList<Zone> Zone::afficher(QString *erreur)
{
    QList<Zone> out;
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT ID_ZONE, NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT "
                             "FROM ZONE ORDER BY ID_ZONE"));
    if (!run(q, erreur)) return out;
    while (q.next()) out << lireZone(q);
    return out;
}

// ============================================================
//  UPDATE
// ============================================================
bool Zone::modifier(QString *erreur) const
{
    Transaction tx;

    // 1) Valeurs actuelles, pour savoir quels champs ont change
    QSqlQuery old;
    old.prepare(QStringLiteral("SELECT ID_ZONE, NOM, CODE, VULNERABILITE, LATITUDE, LONGITUDE, STATUT "
                               "FROM ZONE WHERE ID_ZONE = :id"));
    old.bindValue(QStringLiteral(":id"), id);
    if (!run(old, erreur)) return false;
    if (!old.next()) {
        if (erreur) *erreur = QStringLiteral("Zone introuvable.");
        return false;
    }
    const Zone avant = lireZone(old);

    // 2) Mise a jour (requete preparee)
    QSqlQuery q;
    q.prepare(QStringLiteral("UPDATE ZONE SET NOM = :nom, CODE = :code, VULNERABILITE = :vuln, "
                             "LATITUDE = :lat, LONGITUDE = :lon, STATUT = :statut WHERE ID_ZONE = :id"));
    q.bindValue(QStringLiteral(":nom"), nom);
    q.bindValue(QStringLiteral(":code"), code);
    q.bindValue(QStringLiteral(":vuln"), vulnerabilite);
    q.bindValue(QStringLiteral(":lat"), latitude);
    q.bindValue(QStringLiteral(":lon"), longitude);
    q.bindValue(QStringLiteral(":statut"), statut);
    q.bindValue(QStringLiteral(":id"), id);
    if (!run(q, erreur)) return false;

    // 3) Historique (metier 5) : un enregistrement par champ modifie
    if (avant.nom != nom && !logChange(id, QStringLiteral("Nom"), avant.nom, nom, erreur)) return false;
    if (avant.code != code && !logChange(id, QStringLiteral("Code"), avant.code, code, erreur)) return false;
    if (avant.statut != statut && !logChange(id, QStringLiteral("Statut"), avant.statut, statut, erreur)) return false;
    if (!qFuzzyCompare(avant.vulnerabilite + 1.0, vulnerabilite + 1.0)
        && !logChange(id, QStringLiteral("Vulnérabilité"), num(avant.vulnerabilite), num(vulnerabilite), erreur))
        return false;
    if ((avant.latitude != latitude || avant.longitude != longitude)
        && !logChange(id, QStringLiteral("Coordonnées"),
                      QStringLiteral("%1 ; %2").arg(avant.latitude).arg(avant.longitude),
                      QStringLiteral("%1 ; %2").arg(latitude).arg(longitude), erreur))
        return false;

    if (!tx.commit()) return fail(erreur, QSqlDatabase::database().lastError());
    return true;
}

// ============================================================
//  DELETE
// ============================================================
bool Zone::supprimer(int id, QString *erreur)
{
    QSqlQuery q;
    q.prepare(QStringLiteral("DELETE FROM ZONE WHERE ID_ZONE = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!run(q, erreur)) return false;
    if (q.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Zone introuvable.");
        return false;
    }
    return true;
}

// ============================================================
//  Historique d'une zone
// ============================================================
QList<ZoneHistory> ZoneHistory::pourZone(int idZone, QString *erreur)
{
    QList<ZoneHistory> out;
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR "
                             "FROM ZONE_HISTORIQUE WHERE ID_ZONE = :id ORDER BY DATE_CHANGEMENT DESC, ID_HIST DESC"));
    q.bindValue(QStringLiteral(":id"), idZone);
    if (!run(q, erreur)) return out;
    while (q.next()) {
        ZoneHistory h;
        h.idZone = idZone;
        h.date = q.value(0).toDateTime();
        h.champ = q.value(1).toString();
        h.ancien = q.value(2).toString();
        h.nouveau = q.value(3).toString();
        out << h;
    }
    return out;
}
