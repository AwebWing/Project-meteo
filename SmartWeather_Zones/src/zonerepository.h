#pragma once
#include <QList>
#include <QObject>
#include <optional>
#include "zone.h"

// ============================================================
//  ZoneRepository : acces aux donnees des zones.
//  Pour l'instant les donnees sont EN MEMOIRE (avec des exemples),
//  ce qui permet de lancer l'application sans base de donnees.
//  PLUS TARD : on remplacera l'interieur de ces fonctions par des requetes
//  QSqlQuery vers MySQL. L'interface (les signatures) ne change pas,
//  donc les pages n'auront rien a modifier.
// ============================================================
class ZoneRepository : public QObject
{
    Q_OBJECT
public:
    explicit ZoneRepository(QObject *parent = nullptr);

    QList<Zone>           all() const { return m_zones; }
    std::optional<Zone>   find(int id) const;
    int                   add(Zone z);                              // retourne le nouvel id
    bool                  update(const Zone &z);                    // historise les changements
    bool                  remove(int id, QString *erreur = nullptr);
    QList<ZoneHistory>    history(int idZone) const;

signals:
    void changed();   // emis a chaque modification -> les pages se rafraichissent

private:
    void loadSamples();
    void log(int idZone, const QString &champ, const QString &ancien, const QString &nouveau,
             const QDateTime &date = QDateTime::currentDateTime());

    QList<Zone>        m_zones;
    QList<ZoneHistory> m_history;
    int                m_nextId = 1;
};
