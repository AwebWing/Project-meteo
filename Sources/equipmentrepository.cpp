#include "equipmentrepository.h"

EquipmentRepository::EquipmentRepository(QObject *parent) : QObject(parent)
{
    reload();   // la connexion Oracle doit déjà être établie (voir main.cpp)
}

bool EquipmentRepository::reload()
{
    QString err;
    const QList<Equipment> equipments = Equipment::afficher(&err);
    if (!err.isEmpty()) {
        m_lastError = err;
        return false;
    }
    m_equipments = equipments;
    m_lastError.clear();
    return true;
}

std::optional<Equipment> EquipmentRepository::find(int id) const
{
    for (const Equipment &e : m_equipments)
        if (e.id == id) return e;
    return std::nullopt;
}

int EquipmentRepository::add(Equipment eq)
{
    if (!eq.ajouter(&m_lastError)) return -1;
    reload();
    emit changed();
    return eq.id;
}

bool EquipmentRepository::update(const Equipment &eq)
{
    if (!eq.modifier(&m_lastError)) return false;
    reload();
    emit changed();
    return true;
}

bool EquipmentRepository::remove(int id, QString *erreur)
{
    if (!Equipment::supprimer(id, &m_lastError)) {
        if (erreur) *erreur = m_lastError;
        return false;
    }
    reload();
    emit changed();
    return true;
}

QList<EquipmentHistory> EquipmentRepository::history(int idEquipment) const
{
    return EquipmentHistory::pourEquipement(idEquipment);
}
