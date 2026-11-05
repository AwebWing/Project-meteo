#pragma once
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QString>

// Contrôle de saisie partagé par tous les formulaires (consigne de l'atelier :
// contrôle de saisie obligatoire dans le code C++).
namespace InputFilters {

// Un nom commence par une lettre, puis lettres / espaces / apostrophe / tiret.
// Aucun chiffre n'est accepté.
inline const QString NOM_REGEX = QStringLiteral("^\\p{L}[\\p{L} '’\\-]*$");

inline bool nomValide(const QString &nom)
{
    return QRegularExpression(NOM_REGEX).match(nom.trimmed()).hasMatch();
}

inline const QString NOM_MESSAGE =
    QStringLiteral("doit commencer par une lettre et ne contenir aucun chiffre.");

// Filtre de saisie : les chiffres (et symboles) sont refusés dès la frappe.
inline void appliquerFiltreNom(QLineEdit *edit, int longueurMax = 100)
{
    QString motif = NOM_REGEX;
    motif.remove(0, 1);                 // retire '^'
    motif.chop(1);                      // retire '$'
    edit->setValidator(new QRegularExpressionValidator(QRegularExpression(motif), edit));
    edit->setMaxLength(longueurMax);
}

} // namespace InputFilters
