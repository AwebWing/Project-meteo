#pragma once
#include <QList>
#include <QObject>
#include <optional>
#include "fournisseur.h"

// ============================================================
//  FournisseurRepository : couche entre la page (vue) et l'entité
//  Fournisseur (modèle Oracle).
//  - Aucune requête SQL ici : tout passe par Fournisseur::ajouter /
//    modifier / supprimer / afficher (Sources/fournisseur.cpp).
//  - Garde en mémoire la dernière liste lue et se recharge après
//    chaque ajout / modification / suppression.
//  - Émet changed() après chaque modification : la page se
//    rafraîchit automatiquement.
//  - lastError() permet à la page d'afficher pourquoi la lecture
//    a échoué (table absente, mauvaise colonne, pas de connexion...).
// ============================================================
class FournisseurRepository : public QObject
{
    Q_OBJECT
public:
    explicit FournisseurRepository(QObject *parent = nullptr);

    QList<Fournisseur>         all() const { return m_fournisseurs; }
    std::optional<Fournisseur> find(int id) const;
    int                        add(Fournisseur f);            // retourne le nouvel id, ou -1
    bool                       update(const Fournisseur &f);
    bool                       remove(int id, QString *erreur = nullptr);
    bool                       reload();                      // relit la table FOURNISSEUR

    // Métier : meilleur fournisseur actif avec contrat valide pour un type
    QString suggest(const QString &type) const;

    QString lastError() const { return m_lastError; }

signals:
    void changed();  // émis à chaque modification → la vue se rafraîchit

private:
    QList<Fournisseur> m_fournisseurs;
    QString            m_lastError;
};
