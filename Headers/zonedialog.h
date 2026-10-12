#pragma once
#include <QDialog>
#include <QList>
#include "zone.h"

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QLabel;

// Formulaire Ajouter / Modifier une zone (avec validations + calculateur de score).
class ZoneDialog : public QDialog
{
    Q_OBJECT
public:
    // zone == nullptr  -> mode "Nouvelle zone"
    ZoneDialog(const QList<Zone> &existantes, const Zone *zone, QWidget *parent = nullptr);
    Zone zone() const { return m_zone; }

protected:
    void accept() override;

private:
    void updateNiveau();
    void calculerScore();

    Zone m_zone;
    QList<Zone> m_existantes;

    QLineEdit *m_nom, *m_code;
    QDoubleSpinBox *m_lat, *m_lon, *m_vuln;
    QComboBox *m_statut;
    QSpinBox *m_inond, *m_dens, *m_incid;
    QLabel *m_niveauLabel, *m_error;
};
