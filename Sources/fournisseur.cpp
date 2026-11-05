#include "fournisseur.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

// Colonnes lues par afficher() ; l'ordre doit correspondre à lireFournisseur().
const char *const SELECT_COLONNES =
    "ID_FOURNISSEUR, NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, "
    "NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT";

// ------------------------------------------------------------
//  Messages d'erreur lisibles (codes Oracle courants)
// ------------------------------------------------------------
QString messageErreur(const QSqlError &e)
{
    const QString txt = e.text();
    auto has = [&](const char *ora) { return txt.contains(QLatin1String(ora)); };

    if (has("ORA-00942"))
        return QStringLiteral("La table FOURNISSEUR est introuvable. Exécutez SQL/fournisseur_oracle.sql "
                              "avec l'utilisateur METEO (SQL Developer, F5).");
    if (has("ORA-00904"))
        return QStringLiteral("Une colonne n'existe pas : la table FOURNISSEUR n'a pas la bonne structure. "
                              "Ré-exécutez SQL/fournisseur_oracle.sql.\n\n%1").arg(txt);
    if (has("ORA-02290"))
        return QStringLiteral("Une valeur ne respecte pas les contraintes de la base "
                              "(évaluation de 1 à 5, statut valide, fin de contrat après le début).");
    if (has("ORA-12899"))
        return QStringLiteral("Une valeur est trop longue pour sa colonne.");
    if (has("ORA-01438"))
        return QStringLiteral("Un nombre dépasse la précision autorisée par la colonne.");
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

Fournisseur lireFournisseur(const QSqlQuery &q)
{
    Fournisseur f;
    f.id          = q.value(0).toInt();
    f.nom         = q.value(1).toString();
    f.type        = q.value(2).toString();
    f.contact     = q.value(3).toString();
    f.email       = q.value(4).toString();
    f.telephone   = q.value(5).toString();
    f.adresse     = q.value(6).toString();
    f.gouvernorat = q.value(7).toString();
    f.note        = q.value(8).toInt();
    f.delai       = q.value(9).toInt();
    // Oracle stocke DATE sans fuseau → toDate()
    f.debut       = q.value(10).toDate();
    f.fin         = q.value(11).toDate();
    f.statut      = q.value(12).toString();
    return f;
}

} // namespace

// ============================================================
//  CREATE
// ============================================================
bool Fournisseur::ajouter(QString *erreur)
{
    // 1) Nouvelle clé primaire (séquence Oracle)
    QSqlQuery seq;
    seq.prepare(QStringLiteral("SELECT SEQ_FOURNISSEUR.NEXTVAL FROM DUAL"));
    if (!run(seq, erreur)) return false;
    if (!seq.next()) return fail(erreur, seq.lastError());
    const int nouvelId = seq.value(0).toInt();
    seq.finish();

    // 2) Insertion (requête préparée)
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "INSERT INTO FOURNISSEUR "
        "(ID_FOURNISSEUR, NOM, TYPE, CONTACT, EMAIL, TELEPHONE, ADRESSE, GOUVERNORAT, "
        " NOTE, DELAI, DATE_DEBUT, DATE_FIN, STATUT) "
        "VALUES (:id, :nom, :type, :contact, :email, :tel, :adresse, :gouv, "
        " :note, :delai, :debut, :fin, :statut)"));
    q.bindValue(QStringLiteral(":id"),      nouvelId);
    q.bindValue(QStringLiteral(":nom"),     nom);
    q.bindValue(QStringLiteral(":type"),    type);
    q.bindValue(QStringLiteral(":contact"), contact);
    q.bindValue(QStringLiteral(":email"),   email);
    q.bindValue(QStringLiteral(":tel"),     telephone);
    q.bindValue(QStringLiteral(":adresse"), adresse);
    q.bindValue(QStringLiteral(":gouv"),    gouvernorat);
    q.bindValue(QStringLiteral(":note"),    note);
    q.bindValue(QStringLiteral(":delai"),   delai);
    q.bindValue(QStringLiteral(":debut"),   debut);
    q.bindValue(QStringLiteral(":fin"),     fin);
    q.bindValue(QStringLiteral(":statut"),  statut);
    if (!run(q, erreur)) return false;

    id = nouvelId;
    return true;
}

// ============================================================
//  READ
// ============================================================
QList<Fournisseur> Fournisseur::afficher(QString *erreur)
{
    QList<Fournisseur> out;
    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT %1 FROM FOURNISSEUR ORDER BY ID_FOURNISSEUR")
                  .arg(QLatin1String(SELECT_COLONNES)));
    if (!run(q, erreur)) return out;
    while (q.next()) out << lireFournisseur(q);
    return out;
}

// ============================================================
//  UPDATE
// ============================================================
bool Fournisseur::modifier(QString *erreur) const
{
    // Mise à jour (requête préparée)
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "UPDATE FOURNISSEUR SET NOM = :nom, TYPE = :type, CONTACT = :contact, "
        "EMAIL = :email, TELEPHONE = :tel, ADRESSE = :adresse, GOUVERNORAT = :gouv, "
        "NOTE = :note, DELAI = :delai, DATE_DEBUT = :debut, DATE_FIN = :fin, "
        "STATUT = :statut WHERE ID_FOURNISSEUR = :id"));
    q.bindValue(QStringLiteral(":nom"),     nom);
    q.bindValue(QStringLiteral(":type"),    type);
    q.bindValue(QStringLiteral(":contact"), contact);
    q.bindValue(QStringLiteral(":email"),   email);
    q.bindValue(QStringLiteral(":tel"),     telephone);
    q.bindValue(QStringLiteral(":adresse"), adresse);
    q.bindValue(QStringLiteral(":gouv"),    gouvernorat);
    q.bindValue(QStringLiteral(":note"),    note);
    q.bindValue(QStringLiteral(":delai"),   delai);
    q.bindValue(QStringLiteral(":debut"),   debut);
    q.bindValue(QStringLiteral(":fin"),     fin);
    q.bindValue(QStringLiteral(":statut"),  statut);
    q.bindValue(QStringLiteral(":id"),      id);
    if (!run(q, erreur)) return false;

    if (q.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Fournisseur introuvable.");
        return false;
    }
    return true;
}

// ============================================================
//  DELETE
// ============================================================
bool Fournisseur::supprimer(int id, QString *erreur)
{
    QSqlQuery q;
    q.prepare(QStringLiteral("DELETE FROM FOURNISSEUR WHERE ID_FOURNISSEUR = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!run(q, erreur)) return false;
    if (q.numRowsAffected() == 0) {
        if (erreur) *erreur = QStringLiteral("Fournisseur introuvable.");
        return false;
    }
    return true;
}
