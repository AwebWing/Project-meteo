#pragma once
#include <QList>
#include <QObject>
#include <optional>
#include "equipment.h"

// ============================================================
//  EquipmentRepository : couche entre les pages (vue) et l'entité
//  Equipment (modèle Oracle).
//  - Aucune requête SQL ici : tout passe par Equipment::ajouter /
//    modifier / supprimer / afficher (Sources/equipment.cpp).
//  - Garde en mémoire la dernière liste lue et se recharge après
//    chaque ajout / modification / suppression.
//  - Émet changed() après chaque modification : les pages se
//    rafraîchissent automatiquement.
// ============================================================
class EquipmentRepository : public QObject
{
    Q_OBJECT
public:
    explicit EquipmentRepository(QObject *parent = nullptr);

    QList<Equipment>         all()    const { return m_equipments; }
    std::optional<Equipment> find(int id)   const;
    int                      add(Equipment eq);           // retourne le nouvel id, ou -1
    bool                     update(const Equipment &eq); // historise les changements
    bool                     remove(int id, QString *erreur = nullptr);
    QList<EquipmentHistory>  history(int idEquipment) const;
    bool                     reload();                    // relit la table EQUIPEMENT

    QString lastError() const { return m_lastError; }

signals:
    void changed();  // émis à chaque modification → les vues se rafraîchissent

private:
    QList<Equipment> m_equipments;
    QString          m_lastError;
};
