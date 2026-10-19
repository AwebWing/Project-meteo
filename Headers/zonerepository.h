#pragma once
#include <QList>
#include <QObject>
#include <optional>
#include "zone.h"

// ============================================================
//  ZoneRepository : couche entre les pages (vue) et l'entite Zone (modele).
//  - Aucune requete SQL ici : tout passe par Zone::ajouter/modifier/supprimer/afficher
//    (Sources/zone.cpp, requetes preparees sur la base Oracle).
//  - Garde en memoire la derniere liste lue (all()/find() sont appeles tres souvent
//    par les pages) et se recharge apres chaque ajout / modification / suppression.
//  - Emet changed() apres chaque modification : les pages se rafraichissent.
// ============================================================
class ZoneRepository : public QObject
{
    Q_OBJECT
public:
    explicit ZoneRepository(QObject *parent = nullptr);

    QList<Zone>           all() const { return m_zones; }
    std::optional<Zone>   find(int id) const;
    int                   add(Zone z);                              // nouvel id, ou -1 si echec
    bool                  update(const Zone &z);                    // historise les changements
    bool                  remove(int id, QString *erreur = nullptr);
    QList<ZoneHistory>    history(int idZone) const;
    bool                  reload();                                 // relit la table ZONE

    QString lastError() const { return m_lastError; }               // dernier message d'erreur

signals:
    void changed();   // emis a chaque modification -> les pages se rafraichissent

private:
    QList<Zone> m_zones;
    QString     m_lastError;
};
