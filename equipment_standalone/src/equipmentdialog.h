#pragma once
#include <QDialog>
#include "equipment.h"

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QTextEdit;
class EquipmentRepository;

// ---------------------------------------------------------------
//  Formulaire Ajouter / Modifier un équipement
// ---------------------------------------------------------------
class EquipmentDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EquipmentDialog(EquipmentRepository *repo,
                              const Equipment     &eq     = Equipment{},
                              QWidget             *parent = nullptr);

    Equipment result() const { return m_eq; }

private:
    void accept() override;

    EquipmentRepository *m_repo;
    Equipment            m_eq;

    QLineEdit *m_nom;
    QLineEdit *m_code;
    QComboBox *m_categorie;
    QComboBox *m_statut;
    QLineEdit *m_localisation;
    QDateEdit *m_date;
    QTextEdit *m_description;
    QLabel    *m_errorLabel;
};
