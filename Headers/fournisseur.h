#pragma once
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// Types de fournisseurs (ordre stable = ordre d'affichage)
inline const QStringList FOURNISSEUR_TYPES = {
    QStringLiteral("Capteurs et stations"),
    QStringLiteral("Maintenance"),
    QStringLiteral("Énergie"),
    QStringLiteral("Réseau et télécom"),
    QStringLiteral("Informatique")
};

// Statuts possibles
inline const QStringList FOURNISSEUR_STATUTS = {
    QStringLiteral("Actif"),
    QStringLiteral("Suspendu"),
    QStringLiteral("Archivé")
};

// Gouvernorats de Tunisie
inline const QStringList GOUVERNORATS = {
    QStringLiteral("Ariana"),    QStringLiteral("Béja"),      QStringLiteral("Ben Arous"),
    QStringLiteral("Bizerte"),   QStringLiteral("Gabès"),     QStringLiteral("Gafsa"),
    QStringLiteral("Jendouba"),  QStringLiteral("Kairouan"),  QStringLiteral("Kasserine"),
    QStringLiteral("Kébili"),    QStringLiteral("Kef"),       QStringLiteral("Mahdia"),
    QStringLiteral("Manouba"),   QStringLiteral("Médenine"),  QStringLiteral("Monastir"),
    QStringLiteral("Nabeul"),    QStringLiteral("Sfax"),      QStringLiteral("Sidi Bouzid"),
    QStringLiteral("Siliana"),   QStringLiteral("Sousse"),    QStringLiteral("Tataouine"),
    QStringLiteral("Tozeur"),    QStringLiteral("Tunis"),     QStringLiteral("Zaghouan")
};

// Un contrat est "à renouveler" quand il se termine dans moins de N jours
constexpr int FOURNISSEUR_JOURS_RENOUVELLEMENT = 30;

// Icônes par type
inline QString fournisseurTypeIcon(const QString &type)
{
    if (type == QStringLiteral("Capteurs et stations")) return QStringLiteral("📡");
    if (type == QStringLiteral("Maintenance"))          return QStringLiteral("🔧");
    if (type == QStringLiteral("Énergie"))              return QStringLiteral("⚡");
    if (type == QStringLiteral("Réseau et télécom"))    return QStringLiteral("🌐");
    if (type == QStringLiteral("Informatique"))         return QStringLiteral("💻");
    return QStringLiteral("📦");
}

// ---------------------------------------------------------------
//  Entité FOURNISSEUR  (table FOURNISSEUR de la base Oracle)
//  FOURNISSEUR(id_fournisseur, nom, type, contact, email, telephone,
//              adresse, gouvernorat, note, delai, date_debut, date_fin, statut)
//
//  Architecture modèle-vue : TOUTES les requêtes SQL sont ici
//  (Sources/fournisseur.cpp). Aucune requête dans la page ni dans le
//  repository. Toutes les requêtes sont PRÉPARÉES (prepare + bindValue).
// ---------------------------------------------------------------
struct Fournisseur {
    int     id = 0;                      // 0 = nouveau fournisseur
    QString nom;
    QString type;
    QString contact;
    QString email;
    QString telephone;
    QString adresse;
    QString gouvernorat;
    int     note  = 3;                   // évaluation de 1 à 5
    int     delai = 7;                   // délai de livraison en jours
    QDate   debut = QDate::currentDate();
    QDate   fin   = QDate::currentDate().addYears(1);
    QString statut = QStringLiteral("Actif");   // Actif / Suspendu / Archivé

    // ---------------- Règles métier simples ----------------
    int  joursRestants() const { return QDate::currentDate().daysTo(fin); }
    bool contratValide() const {
        return statut == QStringLiteral("Actif") && fin.isValid() && fin >= QDate::currentDate();
    }
    bool aRenouveler() const {
        return statut != QStringLiteral("Archivé") && fin.isValid()
               && joursRestants() <= FOURNISSEUR_JOURS_RENOUVELLEMENT;
    }

    // ---------------- CRUD (Oracle) ----------------
    // Chaque méthode retourne true si la requête a réussi.
    // En cas d'échec, *erreur reçoit un message lisible.

    // Create : insère le fournisseur, renseigne this->id.
    bool ajouter(QString *erreur = nullptr);
    // Update : met à jour la ligne.
    bool modifier(QString *erreur = nullptr) const;
    // Delete : supprime le fournisseur.
    static bool supprimer(int id, QString *erreur = nullptr);
    // Read : tous les fournisseurs, triés par id.
    static QList<Fournisseur> afficher(QString *erreur = nullptr);
};
