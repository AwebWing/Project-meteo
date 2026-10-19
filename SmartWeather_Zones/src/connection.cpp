#include "connection.h"
#include <QSqlError>
#include <QSettings>
#include <QDebug>

static const char *DSN_NAME    = "OracleDSN";
static const char *DB_HOST     = "localhost";
static const char *DB_PORT     = "1521";
static const char *DB_USER     = "yassmine";
static const char *DB_PASSWORD = "smart123";

Connection::Connection()
{
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection::~Connection()
{
    if (db.isOpen())
        db.close();
}

Connection& Connection::createInstance()
{
    static Connection instance;
    return instance;
}

bool Connection::createConnection()
{
    if (db.isOpen())
        return true;

    m_lastError.clear();

    // 1) Essai avec le DSN
    db.setDatabaseName(DSN_NAME);
    db.setUserName(DB_USER);
    db.setPassword(DB_PASSWORD);
    if (db.open()) {
        qDebug() << "Connexion Oracle etablie (DSN)";
        return true;
    }
    m_lastError = db.lastError().text();

    // 2) Recherche automatique du pilote ODBC Oracle installe
    QStringList oracleDrivers;
#ifdef Q_OS_WIN
    QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\ODBC\\ODBCINST.INI\\ODBC Drivers",
                  QSettings::NativeFormat);
    const QStringList all = reg.childKeys();
    for (const QString &d : all)
        if (d.contains("Oracle", Qt::CaseInsensitive))
            oracleDrivers << d;
#endif

    if (oracleDrivers.isEmpty()) {
        m_lastError = QStringLiteral(
                          "Aucun pilote ODBC Oracle n'est installe sur cet ordinateur.\n"
                          "Installez Oracle Instant Client (Basic + ODBC, 64 bits) puis executez odbc_install.exe.\n\n")
                      + m_lastError;
        qDebug() << m_lastError;
        return false;
    }

    // 3) Connexion directe (sans DSN) : on teste plusieurs noms de service
    const QStringList services = { "XEPDB1", "XE", "ORCL", "ORCLPDB1" };
    db.setUserName(QString());
    db.setPassword(QString());

    for (const QString &drv : oracleDrivers) {
        for (const QString &svc : services) {
            const QString cs = QString("DRIVER={%1};DBQ=%2:%3/%4;UID=%5;PWD=%6;")
            .arg(drv, DB_HOST, DB_PORT, svc, DB_USER, DB_PASSWORD);
            db.setDatabaseName(cs);
            if (db.open()) {
                qDebug() << "Connexion Oracle etablie :" << drv << svc;
                m_lastError.clear();
                return true;
            }
            m_lastError = db.lastError().text();
            qDebug() << "Essai echoue :" << drv << svc << m_lastError;
        }
    }

    qDebug() << "Echec de la connexion :" << m_lastError;
    return false;
}