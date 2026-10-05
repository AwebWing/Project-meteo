#include "equipmentrepository.h"
#include <algorithm>

EquipmentRepository::EquipmentRepository(QObject *parent) : QObject(parent)
{
    loadSamples();
}

void EquipmentRepository::loadSamples()
{
    auto mk = [this](const QString &nom, const QString &code,
                     const QString &cat, const QString &statut,
                     const QString &loc, QDate date) {
        Equipment e;
        e.id = m_nextId++;
        e.nom = nom; e.code = code; e.categorie = cat;
        e.statut = statut; e.localisation = loc;
        e.derniereMaintenance = date;
        m_equipments << e;
    };

    // Capteurs (6)
    mk("Thermomètre connecté",      "EQ-001", "Capteurs",               "Disponible",     "ST-04",    QDate(2025, 5, 10));
    mk("Station météo",             "EQ-002", "Capteurs",               "Disponible",     "ST-07",    QDate(2025, 5,  8));
    mk("Capteur UV",                "EQ-003", "Capteurs",               "Disponible",     "ST-01",    QDate(2025, 4, 22));
    mk("Baromètre digital",         "EQ-004", "Capteurs",               "Hors service",   "ST-03",    QDate(2025, 3, 15));
    mk("Anémomètre",                "EQ-005", "Capteurs",               "Disponible",     "ST-09",    QDate(2025, 5, 12));
    mk("Pluviomètre",               "EQ-006", "Capteurs",               "Disponible",     "ST-05",    QDate(2025, 5,  3));

    // Véhicules (6)
    mk("Véhicule V-01",             "EQ-007", "Véhicules",              "Disponible",     "Tunis",    QDate(2025, 4, 30));
    mk("Véhicule V-02",             "EQ-008", "Véhicules",              "Disponible",     "Sfax",     QDate(2025, 5,  5));
    mk("Véhicule V-03",             "EQ-009", "Véhicules",              "En maintenance", "Sousse",   QDate(2025, 5,  1));
    mk("Moto tout-terrain",         "EQ-010", "Véhicules",              "Disponible",     "Nabeul",   QDate(2025, 4, 18));
    mk("Camionnette terrain",       "EQ-011", "Véhicules",              "Disponible",     "Bizerte",  QDate(2025, 5,  7));
    mk("Drone de surveillance",     "EQ-012", "Véhicules",              "Disponible",     "Ariana",   QDate(2025, 5,  9));

    // Matériel de terrain (5)
    mk("GPS portable",              "EQ-013", "Matériel de terrain",    "Disponible",     "ST-12",    QDate(2025, 5,  7));
    mk("Tente de terrain",          "EQ-014", "Matériel de terrain",    "En maintenance", "Manouba",  QDate(2025, 5,  5));
    mk("Lampe UV terrain",          "EQ-015", "Matériel de terrain",    "Disponible",     "ST-02",    QDate(2025, 4, 28));
    mk("Trousse premiers secours",  "EQ-016", "Matériel de terrain",    "Disponible",     "ST-06",    QDate(2025, 5, 11));
    mk("Radio longue portée",       "EQ-017", "Matériel de terrain",    "Disponible",     "ST-08",    QDate(2025, 5,  6));

    // Équipements de sécurité (4)
    mk("Gilet de sécurité",         "EQ-018", "Équipements de sécurité","Disponible",     "Dépôt-1",  QDate(2025, 5,  2));
    mk("Casque de protection",      "EQ-019", "Équipements de sécurité","Disponible",     "Dépôt-2",  QDate(2025, 5,  4));
    mk("Harnais de sécurité",       "EQ-020", "Équipements de sécurité","Disponible",     "Dépôt-1",  QDate(2025, 5,  1));
    mk("Extincteur portable",       "EQ-021", "Équipements de sécurité","Disponible",     "Dépôt-3",  QDate(2025, 4, 20));

    // Historique d'exemple
    const QDateTime base = QDateTime(QDate(2025, 4,  1), QTime(9, 0));
    log(3,  "Statut", "En maintenance", "Disponible",     base);
    log(9,  "Statut", "Disponible",     "En maintenance", base.addDays(10));
    log(4,  "Statut", "Disponible",     "Hors service",   base.addDays(15));
    log(14, "Statut", "Disponible",     "En maintenance", base.addDays(20));
}

// ----------------------------------------------------------------

std::optional<Equipment> EquipmentRepository::find(int id) const
{
    for (const Equipment &e : m_equipments)
        if (e.id == id) return e;
    return std::nullopt;
}

int EquipmentRepository::add(Equipment eq)
{
    eq.id = m_nextId++;
    m_equipments << eq;
    log(eq.id, QStringLiteral("Création"), QString(), eq.nom);
    emit changed();
    return eq.id;
}

bool EquipmentRepository::update(const Equipment &eq)
{
    for (Equipment &old : m_equipments) {
        if (old.id != eq.id) continue;

        if (old.nom          != eq.nom)          log(eq.id, "Nom",          old.nom,          eq.nom);
        if (old.code         != eq.code)         log(eq.id, "Code",         old.code,         eq.code);
        if (old.statut       != eq.statut)       log(eq.id, "Statut",       old.statut,       eq.statut);
        if (old.categorie    != eq.categorie)    log(eq.id, "Catégorie",    old.categorie,    eq.categorie);
        if (old.localisation != eq.localisation) log(eq.id, "Localisation", old.localisation, eq.localisation);

        old = eq;
        emit changed();
        return true;
    }
    return false;
}

bool EquipmentRepository::remove(int id, QString *erreur)
{
    for (int i = 0; i < m_equipments.size(); ++i) {
        if (m_equipments[i].id != id) continue;
        m_equipments.removeAt(i);
        emit changed();
        return true;
    }
    if (erreur) *erreur = QStringLiteral("Équipement introuvable.");
    return false;
}

QList<EquipmentHistory> EquipmentRepository::history(int idEquipment) const
{
    QList<EquipmentHistory> out;
    for (const EquipmentHistory &h : m_history)
        if (h.idEquipment == idEquipment) out << h;
    std::sort(out.begin(), out.end(), [](const EquipmentHistory &a, const EquipmentHistory &b) {
        return a.date > b.date;
    });
    return out;
}

void EquipmentRepository::log(int idEquipment,
                               const QString &champ,
                               const QString &ancien,
                               const QString &nouveau,
                               const QDateTime &date)
{
    EquipmentHistory h;
    h.idEquipment = idEquipment;
    h.date   = date;
    h.champ  = champ;
    h.ancien = ancien;
    h.nouveau= nouveau;
    m_history << h;
}
