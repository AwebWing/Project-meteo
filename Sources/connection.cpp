#include "connection.h"

#include <QDebug>
#include <QSqlError>

Connection::Connection() {
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection::~Connection() {
    if (db.isOpen()) {
        db.close();
    }
}

Connection& Connection::createInstance() {
    static Connection instance;
    return instance;
}

bool Connection::createConnection() {
    db.setDatabaseName("smartweatherDSN");   // source de donnees ODBC
    db.setUserName("meteo");
    db.setPassword("meteo5");

    if (db.open()) {
        m_lastError.clear();
        qDebug() << "Connexion établie";
        return true;
    }

    m_lastError = db.lastError().text();
    qDebug() << "Échec de la connexion :" << m_lastError;
    return false;
}
