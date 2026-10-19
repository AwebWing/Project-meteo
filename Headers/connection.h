#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QString>

// Patron Singleton : une seule connexion Oracle (ODBC) pour toute l'application.
// Les requetes utilisent ensuite la connexion par defaut (QSqlQuery q;).
class Connection {
public:
    static Connection& createInstance();
    bool createConnection();
    QString lastError() const { return m_lastError; }

private:
    Connection();
    ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    QSqlDatabase db;
    QString m_lastError;
};

#endif // CONNECTION_H
