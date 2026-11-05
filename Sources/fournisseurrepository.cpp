#include "fournisseurrepository.h"

FournisseurRepository::FournisseurRepository(QObject *parent) : QObject(parent)
{
    reload();   // la connexion Oracle doit déjà être établie (voir main.cpp)
}

bool FournisseurRepository::reload()
{
    QString err;
    const QList<Fournisseur> liste = Fournisseur::afficher(&err);
    if (!err.isEmpty()) {
        m_lastError = err;      // on garde l'ancienne liste en mémoire
        return false;
    }
    m_fournisseurs = liste;
    m_lastError.clear();
    return true;
}

std::optional<Fournisseur> FournisseurRepository::find(int id) const
{
    for (const Fournisseur &f : m_fournisseurs)
        if (f.id == id) return f;
    return std::nullopt;
}

int FournisseurRepository::add(Fournisseur f)
{
    if (!f.ajouter(&m_lastError)) return -1;
    reload();
    emit changed();
    return f.id;
}

bool FournisseurRepository::update(const Fournisseur &f)
{
    if (!f.modifier(&m_lastError)) return false;
    reload();
    emit changed();
    return true;
}

bool FournisseurRepository::remove(int id, QString *erreur)
{
    if (!Fournisseur::supprimer(id, &m_lastError)) {
        if (erreur) *erreur = m_lastError;
        return false;
    }
    reload();
    emit changed();
    return true;
}

QString FournisseurRepository::suggest(const QString &type) const
{
    if (type.isEmpty())
        return QStringLiteral("Choisissez d'abord un type de fournisseur.");

    // Meilleure note d'abord, puis délai de livraison le plus court
    const Fournisseur *best = nullptr;
    for (const Fournisseur &f : m_fournisseurs) {
        if (f.type != type || !f.contratValide()) continue;
        if (!best || f.note > best->note || (f.note == best->note && f.delai < best->delai))
            best = &f;
    }
    if (best)
        return QStringLiteral("Meilleur fournisseur pour « %1 » : %2 (évaluation %3/5, livraison en %4 jours)")
            .arg(type, best->nom).arg(best->note).arg(best->delai);
    return QStringLiteral("Aucun fournisseur actif avec un contrat valide pour « %1 ».").arg(type);
}
