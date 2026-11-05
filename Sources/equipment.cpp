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
    if (has("ORA-02291", "2291"))
        return QStringLiteral("Le fournisseur choisi n'existe plus : choisissez un fournisseur valide.");
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
    e.idFournisseur  = q.value(8).toInt();
    e.fournisseurNom = q.value(9).toString();
    return e;
}

} // namespace

// ============================================================
//  CREATE
// ============================================================
bool Equipment::ajouter(QString *erreur)
{
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
        "(ID_EQUIPEMENT, NOM, CODE, CATEGORIE, STATUT, LOCALISATION, DERNIERE_MAINT, DESCRIPTION, ID_FOURNISSEUR) "
        "VALUES (:id, :nom, :code, :cat, :statut, :loc, :maint, :desc, :fourn)"));
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
    q.bindValue(QStringLiteral(":fourn"),  idFournisseur);
    if (!run(q, erreur)) return false;

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
        "SELECT E.ID_EQUIPEMENT, E.NOM, E.CODE, E.CATEGORIE, E.STATUT, E.LOCALISATION, E.DERNIERE_MAINT, "
        "E.DESCRIPTION, E.ID_FOURNISSEUR, F.NOM "
        "FROM EQUIPEMENT E JOIN FOURNISSEUR F ON F.ID_FOURNISSEUR = E.ID_FOURNISSEUR ORDER BY E.ID_EQUIPEMENT"));
    if (!run(q, erreur)) return out;
    while (q.next()) out << lireEquipment(q);
    return out;
}

// ============================================================
//  Fournisseurs sélectionnables (liste déroulante du formulaire)
// ============================================================
QList<QPair<int, QString>> Equipment::fournisseurs(QString *erreur)
{
    QList<QPair<int, QString>> out;
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT ID_FOURNISSEUR, NOM FROM FOURNISSEUR ORDER BY NOM"));
    if (!run(q, erreur)) return out;
    while (q.next()) out << qMakePair(q.value(0).toInt(), q.value(1).toString());
    return out;
}

// ============================================================
//  UPDATE
// ============================================================
bool Equipment::modifier(QString *erreur) const
{
    // Mise à jour (requête préparée)
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "UPDATE EQUIPEMENT SET NOM = :nom, CODE = :code, CATEGORIE = :cat, "
        "STATUT = :statut, LOCALISATION = :loc, DERNIERE_MAINT = :maint, "
        "DESCRIPTION = :desc, ID_FOURNISSEUR = :fourn WHERE ID_EQUIPEMENT = :id"));
    q.bindValue(QStringLiteral(":nom"),    nom);
    q.bindValue(QStringLiteral(":code"),   code.trimmed().isEmpty() ? QVariant(QMetaType(QMetaType::QString)) : code);
    q.bindValue(QStringLiteral(":cat"),    categorie);
    q.bindValue(QStringLiteral(":statut"), statut);
    q.bindValue(QStringLiteral(":loc"),    localisation);
    q.bindValue(QStringLiteral(":maint"),  derniereMaintenance.isValid()
                                             ? QVariant(derniereMaintenance)
                                             : QVariant(QMetaType(QMetaType::QDate)));
    q.bindValue(QStringLiteral(":desc"),   description);
    q.bindValue(QStringLiteral(":fourn"),  idFournisseur);
    q.bindValue(QStringLiteral(":id"),     id);
    if (!run(q, erreur)) return false;

    if (q.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Équipement introuvable.");
        return false;
    }
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
