#pragma once
#include <QString>

// Connexion SQLite + schéma. Les tables ZONE, AGENT, STATION, RELEVE, EMPLOI et
// EQUIPEMENT sont de simples tables "support" minimales (propriété des autres
// membres de l'équipe) : elles permettent de développer et tester les
// interventions seules. Lors de l'intégration, remplacer par le schéma commun.
class Database {
public:
    // path = ":memory:" pour les tests
    static bool open(const QString &path, QString *error = nullptr);
    static void close();

private:
    static bool createSchema(QString *error);
    static bool seedIfEmpty(QString *error);
};
