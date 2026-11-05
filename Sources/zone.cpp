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

} // namespace

// ============================================================
//  CREATE
// ============================================================
bool Zone::ajouter(QString *erreur)
{
    // 1) Nouvelle cle primaire (sequence Oracle)
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
    // Mise a jour (requete preparee)
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
    if (q.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Zone introuvable.");
        return false;
    }
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
