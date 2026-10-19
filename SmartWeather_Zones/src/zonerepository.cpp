#include <algorithm>
#include "zonerepository.h"

ZoneRepository::ZoneRepository(QObject *parent) : QObject(parent)
{
    loadSamples();
}

void ZoneRepository::loadSamples()
{
    auto mk = [this](const QString &nom, const QString &code, double vuln, double lat, double lon,
                     int stations, int interventions) {
        Zone z;
        z.id = m_nextId++;
        z.nom = nom; z.code = code; z.vulnerabilite = vuln;
        z.latitude = lat; z.longitude = lon;
        z.nbStations = stations; z.interventionsActives = interventions;
        m_zones << z;
    };
    mk(QStringLiteral("Tunis Centre"), "ZN-01", 62, 36.8065, 10.1815, 2, 2);
    mk(QStringLiteral("Ariana"),       "ZN-02", 48, 36.8665, 10.1647, 2, 1);
    mk(QStringLiteral("Ben Arous"),    "ZN-03", 71, 36.7531, 10.2189, 2, 3);
    mk(QStringLiteral("Manouba"),      "ZN-04", 35, 36.8101, 10.0863, 2, 0);
    mk(QStringLiteral("Bizerte"),      "ZN-05", 55, 37.2744,  9.8739, 1, 1);
    mk(QStringLiteral("Nabeul"),       "ZN-06", 40, 36.4513, 10.7357, 2, 0);

    const QDateTime base = QDateTime(QDate(2026, 9, 1), QTime(9, 0));
    log(3, "Vulnérabilité", "65", "71", base);
    log(3, "Statut", "Inactive", "Active", base.addDays(3));
    log(5, "Vulnérabilité", "50", "55", base.addDays(6));
}

std::optional<Zone> ZoneRepository::find(int id) const
{
    for (const Zone &z : m_zones)
        if (z.id == id) return z;
    return std::nullopt;
}

int ZoneRepository::add(Zone z)
{
    z.id = m_nextId++;
    m_zones << z;
    log(z.id, QStringLiteral("Création"), QString(), z.nom);
    emit changed();
    return z.id;
}

bool ZoneRepository::update(const Zone &z)
{
    for (Zone &old : m_zones) {
        if (old.id != z.id) continue;
        // Metier 5 : on historise les champs importants
        if (old.nom != z.nom)                 log(z.id, "Nom", old.nom, z.nom);
        if (old.code != z.code)               log(z.id, "Code", old.code, z.code);
        if (old.statut != z.statut)           log(z.id, "Statut", old.statut, z.statut);
        if (!qFuzzyCompare(old.vulnerabilite + 1.0, z.vulnerabilite + 1.0))
            log(z.id, QStringLiteral("Vulnérabilité"), QString::number(old.vulnerabilite), QString::number(z.vulnerabilite));
        if (old.latitude != z.latitude || old.longitude != z.longitude)
            log(z.id, "Coordonnées",
                QStringLiteral("%1 ; %2").arg(old.latitude).arg(old.longitude),
                QStringLiteral("%1 ; %2").arg(z.latitude).arg(z.longitude));
        old = z;
        emit changed();
        return true;
    }
    return false;
}

bool ZoneRepository::remove(int id, QString *erreur)
{
    for (int i = 0; i < m_zones.size(); ++i) {
        if (m_zones[i].id != id) continue;
        if (m_zones[i].nbStations > 0) {
            if (erreur)
                *erreur = QStringLiteral("Cette zone possède %1 station(s) rattachée(s) : elle ne peut pas être supprimée.")
                              .arg(m_zones[i].nbStations);
            return false;
        }
        m_zones.removeAt(i);
        emit changed();
        return true;
    }
    if (erreur) *erreur = QStringLiteral("Zone introuvable.");
    return false;
}

QList<ZoneHistory> ZoneRepository::history(int idZone) const
{
    QList<ZoneHistory> out;
    for (const ZoneHistory &h : m_history)
        if (h.idZone == idZone) out << h;
    std::sort(out.begin(), out.end(), [](const ZoneHistory &a, const ZoneHistory &b) { return a.date > b.date; });
    return out;
}

void ZoneRepository::log(int idZone, const QString &champ, const QString &ancien, const QString &nouveau,
                         const QDateTime &date)
{
    ZoneHistory h;
    h.idZone = idZone; h.date = date; h.champ = champ; h.ancien = ancien; h.nouveau = nouveau;
    m_history << h;
}
