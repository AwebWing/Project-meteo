#pragma once
#include "intervention.h"
#include <QDialog>

class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QPlainTextEdit;

// Formulaire de création / modification. Les règles de validation sont celles du service
// (aucune règle dupliquée dans l'interface).
class InterventionDialog : public QDialog {
    Q_OBJECT
public:
    InterventionDialog(const Intervention &initial, bool isNew, QWidget *parent = nullptr);
    Intervention result() const { return m_result; }

protected:
    void accept() override;

private:
    bool m_new;
    Intervention m_old;
    Intervention m_result;

    QComboBox *m_type, *m_priorite, *m_statut, *m_zone, *m_emploi;
    QDateTimeEdit *m_debut, *m_fin;
    QLineEdit *m_responsable;
    QPlainTextEdit *m_description, *m_compteRendu;
};
