#pragma once
#include <QString>

namespace Database {
// Ouvre (et crée si besoin) la base SQLite, le schéma et un jeu de données de démonstration.
bool open(const QString &path, QString *error = nullptr);
void close();
}
