#include "zonedialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include "theme.h"
#include "zoneservice.h"

ZoneDialog::ZoneDialog(const QList<Zone> &existantes, const Zone *zone, QWidget *parent)
    : QDialog(parent), m_existantes(existantes)
{
    if (zone) m_zone = *zone;
    setWindowTitle(zone ? QStringLiteral("Modifier la zone") : QStringLiteral("Nouvelle zone"));
    setMinimumWidth(480);

    m_nom = new QLineEdit(m_zone.nom);
    m_code = new QLineEdit(m_zone.code);
    m_code->setPlaceholderText(QStringLiteral("ex : ZN-07"));

    m_lat = new QDoubleSpinBox; m_lat->setRange(-90, 90);   m_lat->setDecimals(5); m_lat->setValue(m_zone.latitude);
    m_lon = new QDoubleSpinBox; m_lon->setRange(-180, 180); m_lon->setDecimals(5); m_lon->setValue(m_zone.longitude);

    m_statut = new QComboBox;
    m_statut->addItems({QStringLiteral("Active"), QStringLiteral("Inactive")});
    m_statut->setCurrentText(m_zone.statut);

    m_vuln = new QDoubleSpinBox; m_vuln->setRange(0, 100); m_vuln->setDecimals(1); m_vuln->setValue(m_zone.vulnerabilite);
    m_niveauLabel = new QLabel;

    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->addRow(QStringLiteral("Nom *"), m_nom);
    form->addRow(QStringLiteral("Code *"), m_code);
    form->addRow(QStringLiteral("Latitude"), m_lat);
    form->addRow(QStringLiteral("Longitude"), m_lon);
    form->addRow(QStringLiteral("Statut"), m_statut);
    form->addRow(QStringLiteral("Vulnérabilité (0-100)"), m_vuln);
    form->addRow(QString(), m_niveauLabel);

    // --- Calculateur de score (metier 1) ---
    auto *box = new QGroupBox(QStringLiteral("Calculateur de score de vulnérabilité"));
    auto *bf = new QFormLayout(box);
    m_inond = new QSpinBox; m_inond->setRange(0, 100); m_inond->setValue(50);
    m_dens  = new QSpinBox; m_dens->setRange(0, 100);  m_dens->setValue(50);
    m_incid = new QSpinBox; m_incid->setRange(0, 100); m_incid->setValue(50);
    auto *calc = new QPushButton(QStringLiteral("Calculer le score"));
    bf->addRow(QStringLiteral("Exposition aux inondations (50 %)"), m_inond);
    bf->addRow(QStringLiteral("Densité de population (30 %)"), m_dens);
    bf->addRow(QStringLiteral("Historique d'incidents (20 %)"), m_incid);
    bf->addRow(QString(), calc);

    m_error = new QLabel;
    m_error->setStyleSheet("color:#ff8a8b;");
    m_error->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Enregistrer"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName("primary");
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Annuler"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);
    root->addLayout(form);
    root->addWidget(box);
    root->addWidget(m_error);
    root->addWidget(buttons);

    connect(calc, &QPushButton::clicked, this, &ZoneDialog::calculerScore);
    connect(m_vuln, &QDoubleSpinBox::valueChanged, this, &ZoneDialog::updateNiveau);
    connect(buttons, &QDialogButtonBox::accepted, this, &ZoneDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    updateNiveau();
}

void ZoneDialog::updateNiveau()
{
    const QString n = ZoneService::niveau(m_vuln->value());
    m_niveauLabel->setText(QStringLiteral("Niveau : <b style='color:%1'>%2</b>")
                               .arg(Theme::levelColor(n).name(), n));
}

void ZoneDialog::calculerScore()
{
    m_vuln->setValue(ZoneService::scoreVulnerabilite(m_inond->value(), m_dens->value(), m_incid->value()));
}

void ZoneDialog::accept()
{
    Zone z = m_zone;   // garde id, nbStations, interventionsActives
    z.nom = m_nom->text().trimmed();
    z.code = m_code->text().trimmed().toUpper();
    z.latitude = m_lat->value();
    z.longitude = m_lon->value();
    z.statut = m_statut->currentText();
    z.vulnerabilite = m_vuln->value();

    const QStringList erreurs = ZoneService::valider(z, m_existantes);
    if (!erreurs.isEmpty()) {
        m_error->setText(QStringLiteral("• ") + erreurs.join(QStringLiteral("\n• ")));
        return;
    }

    // Metier 4 : anomalies geographiques -> avertissement, l'utilisateur decide
    const QStringList anomalies = ZoneService::controleGeographique(z);
    if (!anomalies.isEmpty()) {
        const auto rep = QMessageBox::question(this, QStringLiteral("Anomalie géographique"),
            anomalies.join('\n') + QStringLiteral("\n\nEnregistrer quand même ?"));
        if (rep != QMessageBox::Yes) return;
    }

    m_zone = z;
    QDialog::accept();
}
