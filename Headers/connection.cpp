#include "connection.h"

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
    db.setDatabaseName("smartweatherDSN");
    db.setUserName("meteo");
    db.setPassword("meteo5");

    if (db.open()) {
        qDebug() << "Connexion établie";
        return true;
    }

    qDebug() << "Échec de la connexion :" << db.lastError().text();
    return false;
}