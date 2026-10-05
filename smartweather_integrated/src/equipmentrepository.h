#pragma once
#include <QList>
#include <QObject>
#include <optional>
#include "equipment.h"

// ============================================================
//  EquipmentRepository : couche données (en mémoire).
//  Pour passer à MySQL : seuls les corps des fonctions changent ;
//  les signatures — et donc les pages UI — restent identiques.
// ============================================================
class EquipmentRepository : public QObject
{
    Q_OBJECT
public:
    explicit EquipmentRepository(QObject *parent = nullptr);

    QList<Equipment>         all()    const { return m_equipments; }
    std::optional<Equipment> find(int id)   const;
    int                      add(Equipment eq);           // retourne le nouvel id
    bool                     update(const Equipment &eq); // historise les changements
    bool                     remove(int id, QString *erreur = nullptr);
    QList<EquipmentHistory>  history(int idEquipment) const;

signals:
    void changed();  // émis à chaque modification → les vues se rafraîchissent

private:
    void loadSamples();
    void log(int idEquipment,
             const QString &champ,
             const QString &ancien,
             const QString &nouveau,
             const QDateTime &date = QDateTime::currentDateTime());

    QList<Equipment>        m_equipments;
    QList<EquipmentHistory> m_history;
    int                     m_nextId = 1;
};
