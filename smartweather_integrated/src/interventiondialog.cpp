#include "interventiondialog.h"
#include "interventionservice.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

InterventionDialog::InterventionDialog(const Intervention &initial, bool isNew, QWidget *parent)
    : QDialog(parent), m_new(isNew), m_old(initial), m_result(initial)
{
    setWindowTitle(isNew ? tr("Nouvelle intervention")
                         : tr("Modifier l'intervention #%1").arg(initial.id));
    setMinimumWidth(520);

    m_type = new QComboBox;
    m_type->addItems(InterventionService::types());
    m_priorite = new QComboBox;
    m_priorite->addItems(InterventionService::priorites());
    m_statut = new QComboBox;
    m_statut->addItems(isNew ? QStringList{Statut::Planifiee}
                             : InterventionService::transitionsFrom(initial.statut));
    m_statut->setEnabled(!isNew && m_statut->count() > 1);

    m_zone = new QComboBox;
    for (const ZoneItem &z : InterventionService::zones())
        m_zone->addItem(z.active ? z.nom : tr("%1 (inactive)").arg(z.nom), z.id);

    m_emploi = new QComboBox;
    m_emploi->addItem(tr("— Aucun —"), 0);
    for (const EmploiItem &e : InterventionService::emplois())
        m_emploi->addItem(e.label, e.id);

    QDateTime d = QDateTime::currentDateTime();
    d.setTime(QTime(d.time().hour(), 0));
    d = d.addSecs(3600);
    const QDateTime debut = initial.debut.isValid() ? initial.debut : d;
    const QDateTime fin = initial.fin.isValid() ? initial.fin : d.addSecs(4 * 3600);

    m_debut = new QDateTimeEdit(debut);
    m_fin = new QDateTimeEdit(fin);
    for (auto *e : {m_debut, m_fin}) {
        e->setCalendarPopup(true);
        e->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm"));
    }

    m_responsable = new QLineEdit(initial.responsable);
    m_responsable->setPlaceholderText(tr("Nom du responsable (ou lier un emploi)"));
    m_description = new QPlainTextEdit(initial.description);
    m_description->setFixedHeight(70);
    m_compteRendu = new QPlainTextEdit(initial.compteRendu);
    m_compteRendu->setFixedHeight(70);
    m_compteRendu->setPlaceholderText(tr("Obligatoire pour terminer l'intervention (20 caractères min.)"));

    // Valeurs initiales
    if (!isNew) {
        m_type->setCurrentText(initial.type);
        m_priorite->setCurrentText(initial.priorite);
        m_statut->setCurrentText(initial.statut);
        const int zi = m_zone->findData(initial.idZone);
        if (zi >= 0) m_zone->setCurrentIndex(zi);
        const int ei = m_emploi->findData(initial.idEmploi);
        if (ei >= 0) m_emploi->setCurrentIndex(ei);
    } else {
        m_priorite->setCurrentText(Priorite::Moyenne);
    }

    auto *form = new QFormLayout;
    form->addRow(tr("Type *"), m_type);
    form->addRow(tr("Zone *"), m_zone);
    form->addRow(tr("Priorité *"), m_priorite);
    form->addRow(tr("Statut *"), m_statut);
    form->addRow(tr("Début *"), m_debut);
    form->addRow(tr("Fin *"), m_fin);
    form->addRow(tr("Emploi lié"), m_emploi);
    form->addRow(tr("Responsable"), m_responsable);
    form->addRow(tr("Description"), m_description);
    form->addRow(tr("Compte-rendu"), m_compteRendu);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Enregistrer"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Annuler"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addWidget(buttons);
}

void InterventionDialog::accept()
{
    Intervention r = m_old;
    r.type = m_type->currentText();
    r.priorite = m_priorite->currentText();
    r.statut = m_statut->currentText();
    r.debut = m_debut->dateTime();
    r.fin = m_fin->dateTime();
    r.idZone = m_zone->currentData().toInt();
    r.idEmploi = m_emploi->currentData().toInt();
    r.responsable = m_responsable->text().trimmed();
    r.description = m_description->toPlainText().trimmed();
    r.compteRendu = m_compteRendu->toPlainText().trimmed();

    const QStringList errors = InterventionService::validate(r, m_new ? nullptr : &m_old);
    if (!errors.isEmpty()) {
        QMessageBox::warning(this, tr("Données invalides"), errors.join(QLatin1Char('\n')));
        return;   // le formulaire reste ouvert
    }
    m_result = r;
    QDialog::accept();
}
