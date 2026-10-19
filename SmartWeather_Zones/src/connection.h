#pragma once
#include <QSqlDatabase>
#include <QString>

class Connection
{
public:
    static Connection& createInstance();
    bool createConnection();
    bool isOpen() const { return db.isOpen(); }
    QString lastErrorText() const { return m_lastError; }

private:
    Connection();
    ~Connection();
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    QSqlDatabase db;
    QString m_lastError;
};