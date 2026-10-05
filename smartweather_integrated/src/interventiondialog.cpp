#include "interventiondialog.h"
#include "ui_interventiondialog.h"

#include "intervention.h"
#include "interventionservice.h"

#include <QDateTime>
#include <QMessageBox>
#include <QPushButton>

InterventionDialog::InterventionDialog(int idIntervention, QWidget *parent)
    : QDialog(parent), ui(new Ui::InterventionDialog), m_id(idIntervention)
{
    ui->setupUi(this);

    ui->comboType->addItems(InterventionService::types());
    ui->comboPriorite->addItems(InterventionService::priorites());
    ui->comboStatut->addItems(InterventionService::statuts());
    for (const auto &z : InterventionService::zones()) ui->comboZone->addItem(z.nom, z.id);
    ui->comboEmploi->addItem("— aucun —", 0);
    for (const auto &e : InterventionService::emplois()) ui->comboEmploi->addItem(e.label, e.id);

    QDateTime now = QDateTime::currentDateTime();
    now.setTime(QTime(now.time().hour(), 0));
    ui->dtDebut->setDateTime(now);
    ui->dtFin->setDateTime(now.addSecs(2 * 3600));
    ui->dtFin->setEnabled(false);
    connect(ui->checkFin, &QCheckBox::toggled, ui->dtFin, &QWidget::setEnabled);

    ui->buttonBox->button(QDialogButtonBox::Save)->setText("Enregistrer");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Annuler");
    ui->buttonBox->button(QDialogButtonBox::Save)->setProperty("role", "primary");
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &InterventionDialog::onSave);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    if (m_id > 0) {
        Intervention i;
        auto opt = InterventionService::get(m_id);
        if (opt) {
            i = *opt;
            setWindowTitle(QString("Modifier l'intervention #%1").arg(m_id));
            ui->comboType->setCurrentText(i.type);
            ui->comboPriorite->setCurrentText(i.priorite);
            ui->comboStatut->setCurrentText(i.statut);
            ui->comboZone->setCurrentIndex(qMax(0, ui->comboZone->findData(i.idZone)));
            ui->comboEmploi->setCurrentIndex(qMax(0, ui->comboEmploi->findData(i.idEmploi)));
            ui->dtDebut->setDateTime(i.debut);
            ui->checkFin->setChecked(i.fin.isValid());
            if (i.fin.isValid()) ui->dtFin->setDateTime(i.fin);
            ui->editResponsable->setText(i.responsable);
            ui->editDescription->setPlainText(i.description);
            ui->editCompteRendu->setPlainText(i.compteRendu);
        }
    } else {
        setWindowTitle("Nouvelle intervention");
    }
}

InterventionDialog::~InterventionDialog() { delete ui; }

void InterventionDialog::onSave()
{
    Intervention i;
    i.id = m_id;
    i.type = ui->comboType->currentText();
    i.priorite = ui->comboPriorite->currentText();
    i.statut = ui->comboStatut->currentText();
    i.idZone = ui->comboZone->currentData().toInt();
    i.idEmploi = ui->comboEmploi->currentData().toInt();
    i.debut = ui->dtDebut->dateTime();
    if (ui->checkFin->isChecked()) i.fin = ui->dtFin->dateTime();
    i.responsable = ui->editResponsable->text();
    i.description = ui->editDescription->toPlainText();
    i.compteRendu = ui->editCompteRendu->toPlainText();

    m_result = i;

    QString err;
    const bool ok = (m_id > 0) ? InterventionService::update(i, QStringLiteral(""), &err) : InterventionService::add(i, &err);
    if (!ok) {
        QMessageBox::warning(this, "Validation", err);
        return;   // on reste dans le formulaire : l'utilisateur ne perd pas sa saisie
    }
    accept();
}
