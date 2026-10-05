#include "database.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantList>

namespace {

bool run(const QString &sql, QString *err)
{
    QSqlQuery q;
    if (!q.exec(sql)) {
        if (err) *err = q.lastError().text() + "\n" + sql;
        return false;
    }
    return true;
}

void ins(const QString &sql, const QVariantList &values)
{
    QSqlQuery q;
    q.prepare(sql);
    for (const QVariant &v : values) q.addBindValue(v);
    q.exec();
}

QString iso(const QDateTime &d) { return d.toString(Qt::ISODate); }

void seed()
{
    QSqlQuery q("SELECT COUNT(*) FROM zone");
    if (q.next() && q.value(0).toInt() > 0) return;

    const QDateTime now = QDateTime::currentDateTime();
    const QString zsql = "INSERT INTO zone(nom,code,vulnerabilite,statut) VALUES(?,?,?,?)";
    ins(zsql, {"Tunis Centre", "TUN-C", 72, "Active"});
    ins(zsql, {"Ariana", "ARI", 55, "Active"});
    ins(zsql, {"Ben Arous", "BEN", 48, "Active"});
    ins(zsql, {"Manouba", "MAN", 40, "Active"});
    ins(zsql, {"Bizerte", "BIZ", 65, "Active"});
    ins(zsql, {"Nabeul", "NAB", 60, "Active"});
    ins(zsql, {"Sousse Sud", "SOU-S", 30, "Inactive"});

    const QString asql = "INSERT INTO agent(nom,prenom,statut) VALUES(?,?,?)";
    ins(asql, {"Ben Ali", "Sami", "Disponible"});
    ins(asql, {"Trabelsi", "Rania", "Disponible"});
    ins(asql, {"Mejri", "Karim", "Disponible"});
    ins(asql, {"Gharbi", "Nour", "Disponible"});
    ins(asql, {"Jlassi", "Mehdi", "Indisponible"});

    const QString esql = "INSERT INTO emploi(date_debut,date_fin,statut,priorite,id_zone,id_agent) VALUES(?,?,?,?,?,?)";
    ins(esql, {iso(now.addSecs(-6 * 3600)), iso(now.addSecs(3 * 3600)), "En cours", "Haute", 1, 1});
    ins(esql, {iso(now.addSecs(2 * 3600)), iso(now.addSecs(10 * 3600)), "Planifié", "Critique", 5, 2});
    ins(esql, {iso(now.addDays(-10)), iso(now.addDays(-10).addSecs(4 * 3600)), "Terminé", "Faible", 3, 3});
    ins(esql, {iso(now.addSecs(-3 * 3600)), iso(now.addSecs(7 * 3600)), "Planifié", "Moyenne", 6, 5});

    const QString isql =
        "INSERT INTO intervention(type,priorite,statut,date_debut,date_fin,description,responsable,compte_rendu,id_zone,id_emploi) "
        "VALUES(?,?,?,?,?,?,?,?,?,?)";
    const QVariant nul(QMetaType::fromType<QString>());
    const QVariant nulInt(QMetaType::fromType<int>());
    ins(isql, {"Inondation", "Haute", "En cours", iso(now.addSecs(-5 * 3600)), iso(now.addSecs(-3600)),
               "Obstruction des avaloirs sur l'avenue principale.", "Sami Ben Ali", nul, 1, 1});
    ins(isql, {"Vent fort", "Critique", "Planifiée", iso(now.addSecs(3 * 3600)), iso(now.addSecs(9 * 3600)),
               "Sécurisation des installations exposées au vent.", "Rania Trabelsi", nul, 5, 2});
    ins(isql, {"Inspection station", "Moyenne", "Planifiée", iso(now.addDays(2)), iso(now.addDays(2).addSecs(4 * 3600)),
               "Contrôle de routine de la station.", nul, nul, 2, nulInt});
    ins(isql, {"Maintenance préventive", "Faible", "Terminée", iso(now.addDays(-10)), iso(now.addDays(-10).addSecs(3 * 3600)),
               "Nettoyage des capteurs.", "Karim Mejri",
               "Nettoyage des capteurs et remplacement du filtre ; station de nouveau opérationnelle.", 3, 3});
    ins(isql, {"Inondation", "Haute", "Terminée", iso(now.addDays(-20)), iso(now.addDays(-20).addSecs(6 * 3600)),
               "Accumulation d'eau sur la chaussée.", "Nour Gharbi",
               "Pompage effectué, route rouverte à la circulation après contrôle.", 1, nulInt});
    ins(isql, {"Canicule", "Moyenne", "Planifiée", iso(now.addSecs(-2 * 3600)), iso(now.addSecs(6 * 3600)),
               "Surveillance renforcée des zones sensibles.", "Mehdi Jlassi", nul, 6, 4});
    ins(isql, {"Dépannage", "Moyenne", "Annulée", iso(now.addDays(-5)), nul,
               "Demande annulée par le service demandeur.", nul, nul, 4, nulInt});
    ins(isql, {"Inondation", "Moyenne", "Terminée", iso(now.addDays(-40)), iso(now.addDays(-40).addSecs(4 * 3600)),
               "Débordement d'un canal.", "Sami Ben Ali",
               "Curage du canal et vérification des berges effectués sans incident.", 5, nulInt});

    const QString qsql = "INSERT INTO equipement(reference,type,statut,id_intervention) VALUES(?,?,?,?)";
    ins(qsql, {"PLU-001", "Pluviomètre", "Disponible", nulInt});
    ins(qsql, {"ANE-002", "Anémomètre", "En mission", 2});
    ins(qsql, {"GEN-003", "Groupe électrogène", "En mission", 1});
    ins(qsql, {"POM-004", "Pompe d'évacuation", "En mission", 1});
    ins(qsql, {"DRN-005", "Drone d'inspection", "Maintenance", nulInt});
    ins(qsql, {"VEH-006", "Véhicule 4x4", "Disponible", nulInt});
    ins(qsql, {"KIT-007", "Kit capteurs", "Retiré", nulInt});

    ins("INSERT INTO historique_intervention(id_intervention,date_evt,evenement) "
        "SELECT id_intervention, ?, 'Création (données de démonstration)' FROM intervention",
        {iso(now)});
}

} // namespace

bool Database::open(const QString &path, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) {
        if (error) *error = db.lastError().text();
        return false;
    }

    const QStringList ddl = {
        "PRAGMA foreign_keys = ON",
        "CREATE TABLE IF NOT EXISTS zone("
        " id_zone INTEGER PRIMARY KEY AUTOINCREMENT,"
        " nom TEXT NOT NULL,"
        " code TEXT NOT NULL UNIQUE,"
        " vulnerabilite REAL NOT NULL DEFAULT 0 CHECK(vulnerabilite BETWEEN 0 AND 100),"
        " statut TEXT NOT NULL DEFAULT 'Active' CHECK(statut IN ('Active','Inactive')))",
        "CREATE TABLE IF NOT EXISTS agent("
        " id_agent INTEGER PRIMARY KEY AUTOINCREMENT,"
        " nom TEXT NOT NULL, prenom TEXT NOT NULL,"
        " statut TEXT NOT NULL DEFAULT 'Disponible' CHECK(statut IN ('Disponible','Indisponible')))",
        "CREATE TABLE IF NOT EXISTS emploi("
        " id_emploi INTEGER PRIMARY KEY AUTOINCREMENT,"
        " date_debut TEXT NOT NULL, date_fin TEXT NOT NULL,"
        " statut TEXT NOT NULL CHECK(statut IN ('Brouillon','Planifié','En cours','Terminé','Annulé')),"
        " priorite TEXT NOT NULL DEFAULT 'Moyenne',"
        " id_zone INTEGER REFERENCES zone(id_zone),"
        " id_agent INTEGER NOT NULL REFERENCES agent(id_agent),"
        " CHECK(date_fin > date_debut))",
        "CREATE TABLE IF NOT EXISTS intervention("
        " id_intervention INTEGER PRIMARY KEY AUTOINCREMENT,"
        " type TEXT NOT NULL,"
        " priorite TEXT NOT NULL CHECK(priorite IN ('Faible','Moyenne','Haute','Critique')),"
        " statut TEXT NOT NULL CHECK(statut IN ('Planifiée','En cours','Terminée','Annulée')),"
        " date_debut TEXT NOT NULL, date_fin TEXT,"
        " description TEXT, responsable TEXT, compte_rendu TEXT,"
        " id_zone INTEGER NOT NULL REFERENCES zone(id_zone),"
        " id_emploi INTEGER REFERENCES emploi(id_emploi) ON DELETE SET NULL,"
        " CHECK(date_fin IS NULL OR date_fin >= date_debut))",
        "CREATE TABLE IF NOT EXISTS equipement("
        " id_equipement INTEGER PRIMARY KEY AUTOINCREMENT,"
        " reference TEXT NOT NULL UNIQUE, type TEXT NOT NULL,"
        " statut TEXT NOT NULL CHECK(statut IN ('Disponible','En mission','Maintenance','Retiré')),"
        " id_intervention INTEGER REFERENCES intervention(id_intervention) ON DELETE SET NULL)",
        "CREATE TABLE IF NOT EXISTS historique_intervention("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " id_intervention INTEGER NOT NULL REFERENCES intervention(id_intervention) ON DELETE CASCADE,"
        " date_evt TEXT NOT NULL, evenement TEXT NOT NULL)",
    };
    for (const QString &sql : ddl)
        if (!run(sql, error)) return false;

    seed();
    return true;
}

void Database::close()
{
    QSqlDatabase::removeDatabase(QSqlDatabase::database().connectionName());
}
