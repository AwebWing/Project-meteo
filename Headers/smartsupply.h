#ifndef SMARTSUPPLY_H
#define SMARTSUPPLY_H

#include <QDialog>
#include <QWidget>
#include "fournisseur.h"

class QButtonGroup;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QSpinBox;
class QStandardItemModel;
class QTableView;
class FournisseurRepository;

// ---------------------------------------------------------------
//  Formulaire Ajouter / Modifier (même principe que EquipmentDialog :
//  le dialogue ne touche pas à la base, il renvoie result() à la page)
// ---------------------------------------------------------------
class FournisseurDialog : public QDialog
{
    Q_OBJECT
public:
    FournisseurDialog(FournisseurRepository *repo, const Fournisseur &f, QWidget *parent = nullptr);
    Fournisseur result() const { return m_f; }
    void accept() override;

private:
    FournisseurRepository *m_repo;
    Fournisseur m_f;

    QLineEdit *m_nom, *m_contact, *m_email, *m_telephone, *m_adresse;
    QComboBox *m_type, *m_gouv, *m_statut;
    QSpinBox  *m_note, *m_delai;
    QDateEdit *m_debut, *m_fin;
    QLabel    *m_errorLabel;
};

// ---------------------------------------------------------------
//  SmartSupply : page "Fournisseurs"
//    - KPI row (actifs / note moyenne / contrats à renouveler / délai moyen)
//    - Panneau types (gauche)
//    - Tableau CRUD avec recherche + filtre statut (centre)
//    - Panneau détail (droite)
// ---------------------------------------------------------------
class SmartSupply : public QWidget
{
    Q_OBJECT
public:
    explicit SmartSupply(FournisseurRepository *repo, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *e) override;   // relit la base à chaque affichage de l'onglet

private slots:
    void refresh();
    void applyFilters();
    void updateDetail();
    void onAdd();
    void onEdit();
    void onDelete();
    void onSuggest();

private:
    int  currentId() const;   // id de la ligne sélectionnée (-1 si aucune)
    void selectById(int id);

    FournisseurRepository *m_repo;

    QLabel *m_errBanner;

    // KPIs
    QLabel *m_kpiActifs, *m_kpiNote, *m_kpiRenouv, *m_kpiDelai;

    // Types
    QButtonGroup *m_typeGroup;

    // Filtres / recherche
    QLineEdit *m_search;
    QComboBox *m_statutFilter;

    // Tableau
    QTableView         *m_table;
    QStandardItemModel *m_model;

    // Actions
    QPushButton *m_btnEdit, *m_btnDelete;
    QLabel      *m_sugRes;

    // Panneau détail
    QLabel *m_detailTitle, *m_detailBody;
};

#endif // SMARTSUPPLY_H
