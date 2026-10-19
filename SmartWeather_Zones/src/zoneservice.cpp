#include "zoneservice.h"
#include <algorithm>
#include <cmath>

namespace ZoneService {

static double round1(double v) { return std::round(v * 10.0) / 10.0; }

// ---------- Metier 1 ----------
double scoreVulnerabilite(double inondation, double densite, double incidents)
{
    // Poids configurables : 50 % inondation, 30 % densite, 20 % incidents
    const double s = 0.5 * inondation + 0.3 * densite + 0.2 * incidents;
    return round1(std::clamp(s, 0.0, 100.0));
}

QString niveau(double v)
{
    if (v >= 70.0) return QStringLiteral("Élevé");
    if (v >= 40.0) return QStringLiteral("Moyen");
    return QStringLiteral("Faible");
}

// ---------- Metier 2 ----------
double scorePriorite(const Zone &z)
{
    const double activite = std::min(100.0, z.interventionsActives * 25.0);
    return round1(0.7 * z.vulnerabilite + 0.3 * activite);
}

QList<Zone> classementPriorite(QList<Zone> zones)
{
    std::sort(zones.begin(), zones.end(), [](const Zone &a, const Zone &b) {
        return scorePriorite(a) > scorePriorite(b);
    });
    return zones;
}

// ---------- Metier 3 ----------
int stationsAttendues(const Zone &z)
{
    if (z.vulnerabilite >= 70.0) return 3;
    if (z.vulnerabilite >= 40.0) return 2;
    return 1;
}

bool sousCouverture(const Zone &z)
{
    return z.statut == QLatin1String("Active") && z.nbStations < stationsAttendues(z);
}

double couvertureReseau(const QList<Zone> &zones)
{
    int attendues = 0, couvertes = 0;
    for (const Zone &z : zones) {
        if (z.statut != QLatin1String("Active")) continue;
        const int a = stationsAttendues(z);
        attendues += a;
        couvertes += std::min(z.nbStations, a);
    }
    return attendues == 0 ? 0.0 : round1(100.0 * couvertes / attendues);
}

// ---------- Metier 4 ----------
bool coordonneesValides(const Zone &z)
{
    return z.latitude >= -90.0 && z.latitude <= 90.0
        && z.longitude >= -180.0 && z.longitude <= 180.0;
}

QStringList controleGeographique(const Zone &z)
{
    QStringList anomalies;
    if (!coordonneesValides(z)) {
        anomalies << QStringLiteral("Coordonnées hors de la plage géographique valide.");
        return anomalies;
    }
    if (z.latitude == 0.0 && z.longitude == 0.0)
        anomalies << QStringLiteral("Coordonnées (0, 0) : probablement non renseignées.");
    else if (z.latitude < 30.2 || z.latitude > 37.6 || z.longitude < 7.5 || z.longitude > 11.6)
        anomalies << QStringLiteral("Coordonnées en dehors de la Tunisie : à vérifier.");
    return anomalies;
}

// ---------- Validations CRUD ----------
QStringList valider(const Zone &z, const QList<Zone> &existantes)
{
    QStringList erreurs;
    if (z.nom.trimmed().isEmpty())
        erreurs << QStringLiteral("Le nom de la zone est obligatoire.");
    if (z.code.trimmed().isEmpty())
        erreurs << QStringLiteral("Le code de la zone est obligatoire.");
    else {
        for (const Zone &e : existantes) {
            if (e.id != z.id && e.code.compare(z.code, Qt::CaseInsensitive) == 0) {
                erreurs << QStringLiteral("Ce code existe déjà (%1) : le code doit être unique.").arg(e.nom);
                break;
            }
        }
    }
    if (z.vulnerabilite < 0.0 || z.vulnerabilite > 100.0)
        erreurs << QStringLiteral("La vulnérabilité doit être comprise entre 0 et 100.");
    if (!coordonneesValides(z))
        erreurs << QStringLiteral("Les coordonnées doivent respecter une plage géographique valide.");
    if (z.statut != QLatin1String("Active") && z.statut != QLatin1String("Inactive"))
        erreurs << QStringLiteral("Le statut doit être Active ou Inactive.");
    return erreurs;
}

} // namespace ZoneService
