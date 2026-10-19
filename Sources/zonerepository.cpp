#include "zonerepository.h"

ZoneRepository::ZoneRepository(QObject *parent) : QObject(parent)
{
    reload();   // la connexion Oracle doit deja etre etablie (voir main.cpp)
}

bool ZoneRepository::reload()
{
    QString err;
    const QList<Zone> zones = Zone::afficher(&err);
    if (!err.isEmpty()) {
        m_lastError = err;
        return false;
    }
    m_zones = zones;
    m_lastError.clear();
    return true;
}

std::optional<Zone> ZoneRepository::find(int id) const
{
    for (const Zone &z : m_zones)
        if (z.id == id) return z;
    return std::nullopt;
}

int ZoneRepository::add(Zone z)
{
    if (!z.ajouter(&m_lastError)) return -1;
    reload();
    emit changed();
    return z.id;
}

bool ZoneRepository::update(const Zone &z)
{
    if (!z.modifier(&m_lastError)) return false;
    reload();
    emit changed();
    return true;
}

bool ZoneRepository::remove(int id, QString *erreur)
{
    if (const auto z = find(id); z && z->nbStations > 0) {
        m_lastError = QStringLiteral("Cette zone possède %1 station(s) rattachée(s) : elle ne peut pas être supprimée.")
                          .arg(z->nbStations);
        if (erreur) *erreur = m_lastError;
        return false;
    }
    if (!Zone::supprimer(id, &m_lastError)) {
        if (erreur) *erreur = m_lastError;
        return false;
    }
    reload();
    emit changed();
    return true;
}

QList<ZoneHistory> ZoneRepository::history(int idZone) const
{
    return ZoneHistory::pourZone(idZone);
}
