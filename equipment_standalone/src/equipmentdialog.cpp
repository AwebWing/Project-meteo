#include "equipmentdialog.h"
#include "equipmentrepository.h"
#include "equipmentservice.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

EquipmentDialog::EquipmentDialog(EquipmentRepository *repo,
                                  const Equipment     &eq,
                                  QWidget             *parent)
    : QDialog(parent), m_repo(repo), m_eq(eq)
{
    const bool isNew = (eq.id == 0);
    setWindowTitle(isNew ? tr("Nouvel équipement") : tr("Modifier l'équipement"));
    setMinimumWidth(480);
    setModal(true);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(20, 20, 20, 20);

    // --- Titre ---
    auto *titleLbl = new QLabel(isNew ? QStringLiteral("➕  Nouvel équipement")
                                      : QStringLiteral("✏️  Modifier l'équipement"));
    titleLbl->setObjectName("cardTitle");
    root->addWidget(titleLbl);

    // --- Formulaire ---
    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_nom = new QLineEdit(eq.nom);
    m_nom->setPlaceholderText("Ex : Thermomètre connecté");
    form->addRow(QStringLiteral("Nom *"), m_nom);

    m_code = new QLineEdit(eq.code);
    m_code->setPlaceholderText("Ex : EQ-022");
    form->addRow(QStringLiteral("Code"), m_code);

    m_categorie = new QComboBox;
    for (const QString &c : EQUIPMENT_CATEGORIES) m_categorie->addItem(c);
    if (!eq.categorie.isEmpty()) m_categorie->setCurrentText(eq.categorie);
    form->addRow(QStringLiteral("Catégorie *"), m_categorie);

    m_statut = new QComboBox;
    for (const QString &s : EQUIPMENT_STATUTS) m_statut->addItem(s);
    m_statut->setCurrentText(eq.statut);
    form->addRow(QStringLiteral("Statut *"), m_statut);

    m_localisation = new QLineEdit(eq.localisation);
    m_localisation->setPlaceholderText("Ex : ST-04, Tunis, Dépôt-1...");
    form->addRow(QStringLiteral("Localisation *"), m_localisation);

    m_date = new QDateEdit(eq.derniereMaintenance.isValid()
                            ? eq.derniereMaintenance
                            : QDate::currentDate());
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    form->addRow(QStringLiteral("Dernière maintenance"), m_date);

    m_description = new QTextEdit(eq.description);
    m_description->setPlaceholderText("Description optionnelle…");
    m_description->setFixedHeight(72);
    form->addRow(QStringLiteral("Description"), m_description);

    root->addLayout(form);

    // --- Erreurs ---
    m_errorLabel = new QLabel;
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet("color: #ff8a8b; font-size: 12px;");
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    // --- Boutons ---
    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Ok)->setObjectName("primary");
    box->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Enregistrer"));
    box->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Annuler"));
    connect(box, &QDialogButtonBox::accepted, this, &EquipmentDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(box);
}

void EquipmentDialog::accept()
{
    m_eq.nom           = m_nom->text().trimmed();
    m_eq.code          = m_code->text().trimmed();
    m_eq.categorie     = m_categorie->currentText();
    m_eq.statut        = m_statut->currentText();
    m_eq.localisation  = m_localisation->text().trimmed();
    m_eq.derniereMaintenance = m_date->date();
    m_eq.description   = m_description->toPlainText().trimmed();

    const QStringList errs = EquipmentService::valider(m_eq, m_repo->all());
    if (!errs.isEmpty()) {
        m_errorLabel->setText("⚠  " + errs.join(QStringLiteral("\n⚠  ")));
        m_errorLabel->show();
        return;
    }
    QDialog::accept();
}
