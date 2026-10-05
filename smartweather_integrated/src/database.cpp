#include "database.h"

#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

bool run(const QString &sql, const QVariantList &args, QString *err)
{
    QSqlQuery q;
    q.prepare(sql);
    for (const QVariant &a : args)
        q.addBindValue(a);
    if (!q.exec()) {
        if (err)
            *err = q.lastError().text() + QStringLiteral(" — ") + sql;
        return false;
    }
    return true;
}

QString iso(const QDateTime &d) { return d.toString(Qt::ISODate); }

} // namespace

bool Database::open(const QString &path, QString *error)
{
    QSqlDatabase db = QSqlDatabase::contains(QSqlDatabase::defaultConnection)
                          ? QSqlDatabase::database()
                          : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(path);
    if (!db.open()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }
    QSqlQuery pragma;
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    return createSchema(error) && seedIfEmpty(error);
}

void Database::close()
{
    {
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

bool Database::createSchema(QString *error)
{
    const QStringList ddl = {
        // --- tables support (autres membres) ---
        "CREATE TABLE IF NOT EXISTS zone ("
        " id_zone INTEGER PRIMARY KEY AUTOINCREMENT,"
        " nom TEXT NOT NULL,"
        " code TEXT NOT NULL UNIQUE,"
        " vulnerabilite REAL NOT NULL DEFAULT 0 CHECK (vulnerabilite BETWEEN 0 AND 100),"
        " statut TEXT NOT NULL DEFAULT 'Active' CHECK (statut IN ('Active','Inactive')))",

        "CREATE TABLE IF NOT EXISTS agent ("
        " id_agent INTEGER PRIMARY KEY AUTOINCREMENT,"
        " nom TEXT NOT NULL, prenom TEXT NOT NULL,"
        " statut TEXT NOT NULL DEFAULT 'disponible' CHECK (statut IN ('disponible','indisponible')))",

        "CREATE TABLE IF NOT EXISTS station ("
        " id_station INTEGER PRIMARY KEY AUTOINCREMENT,"
        " code TEXT NOT NULL UNIQUE,"
        " statut TEXT NOT NULL DEFAULT 'Opérationnelle',"
        " id_zone INTEGER NOT NULL REFERENCES zone(id_zone))",

        "CREATE TABLE IF NOT EXISTS releve ("
        " id_releve INTEGER PRIMARY KEY AUTOINCREMENT,"
        " horodatage TEXT NOT NULL,"
        " temperature REAL, humidite REAL, pluie REAL, vent REAL,"
        " id_station INTEGER NOT NULL REFERENCES station(id_station))",

        "CREATE TABLE IF NOT EXISTS emploi ("
        " id_emploi INTEGER PRIMARY KEY AUTOINCREMENT,"
        " date_debut TEXT NOT NULL, date_fin TEXT NOT NULL,"
        " statut TEXT NOT NULL CHECK (statut IN ('Brouillon','Planifié','En cours','Terminé','Annulé')),"
        " priorite TEXT NOT NULL DEFAULT 'Moyenne',"
        " id_zone INTEGER NOT NULL REFERENCES zone(id_zone),"
        " id_agent INTEGER NOT NULL REFERENCES agent(id_agent),"
        " CHECK (date_fin > date_debut))",

        // --- table du module Interventions ---
        "CREATE TABLE IF NOT EXISTS intervention ("
        " id_intervention INTEGER PRIMARY KEY AUTOINCREMENT,"
        " type TEXT NOT NULL,"
        " priorite TEXT NOT NULL CHECK (priorite IN ('Faible','Moyenne','Haute','Critique')),"
        " statut TEXT NOT NULL CHECK (statut IN ('Planifiée','En cours','Terminée','Annulée')),"
        " date_debut TEXT NOT NULL,"
        " date_fin TEXT NOT NULL,"
        " description TEXT NOT NULL DEFAULT '',"
        " responsable TEXT NOT NULL DEFAULT '',"
        " compte_rendu TEXT NOT NULL DEFAULT '',"
        " id_zone INTEGER NOT NULL REFERENCES zone(id_zone),"
        " id_emploi INTEGER REFERENCES emploi(id_emploi),"
        " CHECK (date_fin >= date_debut))",

        "CREATE TABLE IF NOT EXISTS historique_intervention ("
        " id_histo INTEGER PRIMARY KEY AUTOINCREMENT,"
        " id_intervention INTEGER NOT NULL,"
        " date_evt TEXT NOT NULL,"
        " action TEXT NOT NULL,"
        " detail TEXT NOT NULL DEFAULT '')",

        // --- table support (Equipements) ---
        "CREATE TABLE IF NOT EXISTS equipement ("
        " id_equipement INTEGER PRIMARY KEY AUTOINCREMENT,"
        " reference TEXT NOT NULL UNIQUE,"
        " type TEXT NOT NULL,"
        " statut TEXT NOT NULL CHECK (statut IN ('Disponible','En mission','Maintenance','Retiré')),"
        " id_intervention INTEGER REFERENCES intervention(id_intervention) ON DELETE SET NULL)",
    };

    for (const QString &sql : ddl)
        if (!run(sql, {}, error))
            return false;
    return true;
}

bool Database::seedIfEmpty(QString *error)
{
    QSqlQuery count(QStringLiteral("SELECT COUNT(*) FROM zone"));
    if (count.next() && count.value(0).toInt() > 0)
        return true;

    const QDateTime now = QDateTime::currentDateTime();

    // Zones
    const QString zsql = "INSERT INTO zone(nom,code,vulnerabilite,statut) VALUES(?,?,?,?)";
    if (!run(zsql, {"Tunis Nord", "TN-01", 72, "Active"}, error)) return false;
    if (!run(zsql, {"Sfax Côte", "SF-01", 55, "Active"}, error)) return false;
    if (!run(zsql, {"Gabès Industriel", "GB-01", 40, "Inactive"}, error)) return false;
    if (!run(zsql, {"Kairouan Plaine", "KR-01", 30, "Active"}, error)) return false;

    // Agents
    const QString asql = "INSERT INTO agent(nom,prenom,statut) VALUES(?,?,?)";
    if (!run(asql, {"Ben Salah", "Ahmed", "disponible"}, error)) return false;
    if (!run(asql, {"Trabelsi", "Sana", "disponible"}, error)) return false;
    if (!run(asql, {"Jlassi", "Karim", "indisponible"}, error)) return false;

    // Stations + relevés (la station de Tunis Nord mesure 42,5 °C => risque chaleur)
    if (!run("INSERT INTO station(code,id_zone) VALUES(?,?)", {"ST-TN-01", 1}, error)) return false;
    if (!run("INSERT INTO station(code,id_zone) VALUES(?,?)", {"ST-SF-01", 2}, error)) return false;
    const QString rsql = "INSERT INTO releve(horodatage,temperature,humidite,pluie,vent,id_station) VALUES(?,?,?,?,?,?)";
    if (!run(rsql, {iso(now.addSecs(-6 * 3600)), 30.0, 45, 0, 20, 1}, error)) return false;
    if (!run(rsql, {iso(now.addSecs(-3600)), 42.5, 40, 0, 30, 1}, error)) return false;
    if (!run(rsql, {iso(now.addSecs(-3600)), 28.0, 90, 12, 35, 2}, error)) return false;

    // Emplois
    const QString esql = "INSERT INTO emploi(date_debut,date_fin,statut,priorite,id_zone,id_agent) VALUES(?,?,?,?,?,?)";
    if (!run(esql, {iso(now.addDays(-1)), iso(now.addDays(2)), "En cours", "Haute", 1, 1}, error)) return false;
    if (!run(esql, {iso(now.addDays(1)), iso(now.addDays(3)), "Planifié", "Moyenne", 2, 2}, error)) return false;
    if (!run(esql, {iso(now.addDays(2)), iso(now.addDays(4)), "Brouillon", "Faible", 4, 3}, error)) return false;

    // Interventions
    const QString isql =
        "INSERT INTO intervention(type,priorite,statut,date_debut,date_fin,description,responsable,compte_rendu,id_zone,id_emploi)"
        " VALUES(?,?,?,?,?,?,?,?,?,?)";
    const QVariant noEmploi = QVariant(QMetaType::fromType<int>());
    const QString cr = "Intervention réalisée, situation stabilisée et zone sécurisée.";
    // 1 : en cours, chaleur, emploi 1 + équipement EQ-001
    if (!run(isql, {"Chaleur", "Moyenne", "En cours", iso(now.addDays(-1)), iso(now.addDays(1)),
                    "Surveillance renforcée suite à 42 °C", "Ahmed Ben Salah", "", 1, 1}, error)) return false;
    // 2 : planifiée, emploi 2, aucun équipement => non prête
    if (!run(isql, {"Inondation", "Moyenne", "Planifiée", iso(now.addDays(1)), iso(now.addDays(2)),
                    "Préparer les pompes avant les pluies", "", "", 2, 2}, error)) return false;
    // 3 : planifiée EN RETARD, sans emploi
    if (!run(isql, {"Vent fort", "Moyenne", "Planifiée", iso(now.addDays(-3)), iso(now.addDays(-1)),
                    "Vérifier les installations exposées", "", "", 4, noEmploi}, error)) return false;
    // 4, 5, 6 : terminées (historique pour l'estimation de durée : 6 h, 10 h, 3 h)
    if (!run(isql, {"Inondation", "Haute", "Terminée", iso(now.addDays(-10)), iso(now.addDays(-10).addSecs(6 * 3600)),
                    "Curage des canalisations", "Sana Trabelsi", cr, 2, noEmploi}, error)) return false;
    if (!run(isql, {"Inondation", "Haute", "Terminée", iso(now.addDays(-20)), iso(now.addDays(-20).addSecs(10 * 3600)),
                    "Pompage zone basse", "Sana Trabelsi", cr, 2, noEmploi}, error)) return false;
    if (!run(isql, {"Maintenance station", "Faible", "Terminée", iso(now.addDays(-5)), iso(now.addDays(-5).addSecs(3 * 3600)),
                    "Remplacement du capteur de vent", "Ahmed Ben Salah", cr, 1, noEmploi}, error)) return false;
    // 7 : annulée
    if (!run(isql, {"Chaleur", "Faible", "Annulée", iso(now.addDays(-2)), iso(now.addDays(-2).addSecs(7200)),
                    "Annulée : alerte levée", "", "", 4, noEmploi}, error)) return false;

    // Equipements
    const QString qsql = "INSERT INTO equipement(reference,type,statut,id_intervention) VALUES(?,?,?,?)";
    if (!run(qsql, {"EQ-001", "Pompe", "En mission", 1}, error)) return false;
    if (!run(qsql, {"EQ-002", "Groupe électrogène", "Disponible", noEmploi}, error)) return false;
    if (!run(qsql, {"EQ-003", "Drone", "Maintenance", noEmploi}, error)) return false;
    if (!run(qsql, {"EQ-004", "Pompe", "Disponible", noEmploi}, error)) return false;

    return true;
}
