#pragma once
#include <QDialog>
#include "intervention.h"

QT_BEGIN_NAMESPACE
namespace Ui { class InterventionDialog; }
QT_END_NAMESPACE

// Formulaire d'ajout / modification d'une intervention (interventiondialog.ui).
class InterventionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InterventionDialog(int idIntervention = 0, QWidget *parent = nullptr);
    ~InterventionDialog() override;

    Intervention result() const { return m_result; }

private slots:
    void onSave();

private:
    Ui::InterventionDialog *ui;
    int m_id;
    Intervention m_result;
};
