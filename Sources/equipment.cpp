#include "equipment.h"

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
        return QStringLiteral("Ce code d'équipement existe déjà : le code doit être unique.");
    if (has("ORA-02292", "2292"))
        return QStringLiteral("Des données dépendent de cet équipement et empêchent sa suppression.");
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

// Exécute une requête déjà préparée/liée ; en cas d'échec, remplit "erreur".
bool run(QSqlQuery &q, QString *erreur)
{
    if (q.exec()) return true;
    return fail(erreur, q.lastError());
}

// ------------------------------------------------------------
//  Transaction RAII : rollback automatique si commit() n'est pas appelé.
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
//  Historique interne
// ------------------------------------------------------------
bool logChange(int idEquipment, const QString &champ,
               const QString &ancien, const QString &nouveau,
               QString *erreur)
{
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "INSERT INTO EQUIPEMENT_HISTORIQUE "
        "(ID_EQUIPEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR) "
        "VALUES (:id, :champ, :ancien, :nouveau)"));
    q.bindValue(QStringLiteral(":id"),     idEquipment);
    q.bindValue(QStringLiteral(":champ"),  champ);
    q.bindValue(QStringLiteral(":ancien"), ancien);
    q.bindValue(QStringLiteral(":nouveau"),nouveau);
    return run(q, erreur);
}

Equipment lireEquipment(const QSqlQuery &q)
{
    Equipment e;
    e.id             = q.value(0).toInt();
    e.nom            = q.value(1).toString();
    e.code           = q.value(2).toString();
    e.categorie      = q.value(3).toString();
    e.statut         = q.value(4).toString();
    e.localisation   = q.value(5).toString();
    // Oracle stocke DATE sans heure → toDate()
    const QVariant vDate = q.value(6);
    e.derniereMaintenance = vDate.isNull() ? QDate() : vDate.toDate();
    e.description    = q.value(7).toString();
    return e;
}

} // namespace

// ============================================================
//  CREATE
// ============================================================
bool Equipment::ajouter(QString *erreur)
{
    Transaction tx;

    // 1) Nouvelle clé primaire (séquence Oracle)
    QSqlQuery seq;
    seq.prepare(QStringLiteral("SELECT SEQ_EQUIPEMENT.NEXTVAL FROM DUAL"));
    if (!run(seq, erreur)) return false;
    if (!seq.next()) return fail(erreur, seq.lastError());
    const int nouvelId = seq.value(0).toInt();

    // 2) Insertion (requête préparée)
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "INSERT INTO EQUIPEMENT "
        "(ID_EQUIPEMENT, NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, DESCRIPTION) "
        "VALUES (:id, :nom, :code, :cat, :statut, :loc, :maint, :desc)"));
    q.bindValue(QStringLiteral(":id"),     nouvelId);
    q.bindValue(QStringLiteral(":nom"),    nom);
    q.bindValue(QStringLiteral(":code"),   code.trimmed().isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : code);
    q.bindValue(QStringLiteral(":cat"),    categorie);
    q.bindValue(QStringLiteral(":statut"), statut);
    q.bindValue(QStringLiteral(":loc"),    localisation);
    q.bindValue(QStringLiteral(":maint"),  derniereMaintenance.isValid()
                                             ? QVariant(derniereMaintenance)
                                             : QVariant(QMetaType(QMetaType::QDate)));
    q.bindValue(QStringLiteral(":desc"),   description);
    if (!run(q, erreur)) return false;

    // 3) Historique de création (métier 5)
    if (!logChange(nouvelId, QStringLiteral("Création"), QString(), nom, erreur)) return false;

    if (!tx.commit()) return fail(erreur, QSqlDatabase::database().lastError());
    id = nouvelId;
    return true;
}

// ============================================================
//  READ
// ============================================================
QList<Equipment> Equipment::afficher(QString *erreur)
{
    QList<Equipment> out;
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "SELECT ID_EQUIPEMENT, NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, DESCRIPTION "
        "FROM EQUIPEMENT ORDER BY ID_EQUIPEMENT"));
    if (!run(q, erreur)) return out;
    while (q.next()) out << lireEquipment(q);
    return out;
}

// ============================================================
//  UPDATE
// ============================================================
bool Equipment::modifier(QString *erreur) const
{
    Transaction tx;

    // 1) Valeurs actuelles (pour détecter les champs modifiés → historique)
    QSqlQuery old;
    old.prepare(QStringLiteral(
        "SELECT ID_EQUIPEMENT, NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, DESCRIPTION "
        "FROM EQUIPEMENT WHERE ID_EQUIPEMENT = :id"));
    old.bindValue(QStringLiteral(":id"), id);
    if (!run(old, erreur)) return false;
    if (!old.next()) {
        if (erreur) *erreur = QStringLiteral("Équipement introuvable.");
        return false;
    }
    const Equipment avant = lireEquipment(old);

    // 2) Mise à jour
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "UPDATE EQUIPEMENT SET NOM = :nom, CODE = :code, CATEGORIE = :cat, "
        "STATUT = :statut, LOCALISATION = :loc, DERNIERE_MAINT = :maint, "
        "DESCRIPTION = :desc WHERE ID_EQUIPEMENT = :id"));
    q.bindValue(QStringLiteral(":nom"),    nom);
    q.bindValue(QStringLiteral(":code"),   code.trimmed().isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : code);
    q.bindValue(QStringLiteral(":cat"),    categorie);
    q.bindValue(QStringLiteral(":statut"), statut);
    q.bindValue(QStringLiteral(":loc"),    localisation);
    q.bindValue(QStringLiteral(":maint"),  derniereMaintenance.isValid()
                                             ? QVariant(derniereMaintenance)
                                             : QVariant(QMetaType(QMetaType::QDate)));
    q.bindValue(QStringLiteral(":desc"),   description);
    q.bindValue(QStringLiteral(":id"),     id);
    if (!run(q, erreur)) return false;

    // 3) Historique : un enregistrement par champ modifié (métier 5)
    if (avant.nom != nom
        && !logChange(id, QStringLiteral("Nom"), avant.nom, nom, erreur)) return false;
    if (avant.code != code
        && !logChange(id, QStringLiteral("Code"), avant.code, code, erreur)) return false;
    if (avant.statut != statut
        && !logChange(id, QStringLiteral("Statut"), avant.statut, statut, erreur)) return false;
    if (avant.categorie != categorie
        && !logChange(id, QStringLiteral("Catégorie"), avant.categorie, categorie, erreur)) return false;
    if (avant.localisation != localisation
        && !logChange(id, QStringLiteral("Localisation"), avant.localisation, localisation, erreur)) return false;
    if (avant.description != description
        && !logChange(id, QStringLiteral("Description"), avant.description, description, erreur)) return false;
    if (avant.derniereMaintenance != derniereMaintenance
        && !logChange(id, QStringLiteral("Dernière maintenance"),
                      avant.derniereMaintenance.toString(Qt::ISODate),
                      derniereMaintenance.toString(Qt::ISODate), erreur)) return false;

    if (!tx.commit()) return fail(erreur, QSqlDatabase::database().lastError());
    return true;
}

// ============================================================
//  DELETE
// ============================================================
bool Equipment::supprimer(int id, QString *erreur)
{
    QSqlQuery q;
    q.prepare(QStringLiteral("DELETE FROM EQUIPEMENT WHERE ID_EQUIPEMENT = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!run(q, erreur)) return false;
    if (q.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Équipement introuvable.");
        return false;
    }
    return true;
}

// ============================================================
//  Historique d'un équipement
// ============================================================
QList<EquipmentHistory> EquipmentHistory::pourEquipement(int idEquipment, QString *erreur)
{
    QList<EquipmentHistory> out;
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "SELECT DATE_CHANGEMENT, CHAMP, ANCIENNE_VALEUR, NOUVELLE_VALEUR "
        "FROM EQUIPEMENT_HISTORIQUE "
        "WHERE ID_EQUIPEMENT = :id ORDER BY DATE_CHANGEMENT DESC, ID_HIST DESC"));
    q.bindValue(QStringLiteral(":id"), idEquipment);
    if (!run(q, erreur)) return out;
    while (q.next()) {
        EquipmentHistory h;
        h.idEquipment = idEquipment;
        h.date   = q.value(0).toDateTime();
        h.champ  = q.value(1).toString();
        h.ancien = q.value(2).toString();
        h.nouveau= q.value(3).toString();
        out << h;
    }
    return out;
}
