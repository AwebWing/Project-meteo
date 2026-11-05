#pragma once
#include <QSqlDatabase>

// Petits utilitaires partagés par les entités Oracle (Zone, Equipment, Fournisseur).
namespace DbHelpers {

// Transaction RAII : rollback automatique si commit() n'est pas appelé
// (par exemple quand une requête échoue au milieu d'un ajout + historique).
// Si le pilote ne gère pas les transactions, on continue sans.
class Transaction
{
public:
    Transaction() : m_db(QSqlDatabase::database()), m_active(m_db.isOpen() && m_db.transaction()) {}
    ~Transaction() { if (m_active) m_db.rollback(); }

    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;

    bool commit()
    {
        if (!m_active) return true;
        m_active = false;
        return m_db.commit();
    }

private:
    QSqlDatabase m_db;
    bool m_active;
};

} // namespace DbHelpers
