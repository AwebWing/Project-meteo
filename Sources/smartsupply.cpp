#include "smartsupply.h"
#include "fournisseurrepository.h"
#include "theme.h"
#include "uihelpers.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include "inputfilters.h"
#include <QShowEvent>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

namespace {

// ------------------------------------------------------------
//  Helpers d'affichage
// ------------------------------------------------------------
QColor statutColor(const QString &s)
{
    if (s == QStringLiteral("Actif"))    return QColor(0x3F, 0xC9, 0x8F);   // vert
    if (s == QStringLiteral("Suspendu")) return QColor(0xE4, 0x97, 0x40);   // orange
    return Theme::mutedColor();                                             // Archivé
}

const QColor ALERT_RED(0xE0, 0x5A, 0x5B);
const QColor STAR_GOLD(0xF5, 0xC5, 0x42);

QString etoiles(int n)
{
    n = qBound(0, n, 5);
    return QString(QStringLiteral("★")).repeated(n) + QString(QStringLiteral("☆")).repeated(5 - n);
}

QString fmtDate(const QDate &d) { return d.toString(QStringLiteral("dd/MM/yyyy")); }

// Texte de la colonne "Contrat" ; *alerte = true si expiré ou à renouveler
QString etatContrat(const Fournisseur &f, bool *alerte)
{
    *alerte = false;
    if (f.statut == QStringLiteral("Archivé")) return fmtDate(f.fin);
    const int j = f.joursRestants();
    if (j < 0) { *alerte = true; return QStringLiteral("Expiré"); }
    if (j <= FOURNISSEUR_JOURS_RENOUVELLEMENT) {
        *alerte = true;
        return QStringLiteral("À renouveler (%1 j)").arg(j);
    }
    return fmtDate(f.fin);
}

QString esc(const QString &s) { return s.toHtmlEscaped(); }

// ------------------------------------------------------------
//  Delegate : affiche le statut comme un badge coloré arrondi
//  (nom unique : EquipmentPage a déjà son propre StatutDelegate)
// ------------------------------------------------------------
class FournisseurStatutDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        const QString statut = idx.data().toString();
        const QColor fg = statutColor(statut);
        const QColor bg(fg.red(), fg.green(), fg.blue(), 50);

        p->save();
        p->setRenderHint(QPainter::Antialiasing);

        if (opt.state & QStyle::State_Selected)
            p->fillRect(opt.rect, QColor(0x12, 0x58, 0x6a, 120));

        const QRect r = opt.rect.adjusted(8, 6, -8, -6);
        p->setBrush(bg);
        p->setPen(QPen(fg, 1));
        p->drawRoundedRect(r, 9, 9);

        p->setPen(fg);
        QFont f = opt.font;
        f.setWeight(QFont::DemiBold);
        p->setFont(f);
        p->drawText(r, Qt::AlignCenter, statut);
        p->restore();
    }
};

// Indices des colonnes
constexpr int COL_NOM     = 0;
constexpr int COL_CONTACT = 1;
constexpr int COL_NOTE    = 2;
constexpr int COL_DELAI   = 3;
constexpr int COL_CONTRAT = 4;
constexpr int COL_STATUT  = 5;
constexpr int COL_COUNT   = 6;

// Rôle utilisé pour trier (texte affiché ≠ valeur de tri : "10 j", dates...)
constexpr int SORT_ROLE = Qt::UserRole + 1;

} // namespace

// =====================================================================
//  FournisseurDialog
// =====================================================================
FournisseurDialog::FournisseurDialog(FournisseurRepository *repo, const Fournisseur &f, QWidget *parent)
    : QDialog(parent), m_repo(repo), m_f(f)
{
    const bool isNew = (f.id == 0);
    setWindowTitle(isNew ? QStringLiteral("Nouveau fournisseur") : QStringLiteral("Modifier le fournisseur"));
    setMinimumWidth(500);
    setModal(true);

    auto *root = new QVBoxLayout(this);
    root->setSpacing(14);
    root->setContentsMargins(20, 20, 20, 20);

    auto *titleLbl = new QLabel(isNew ? QStringLiteral("➕  Nouveau fournisseur")
                                      : QStringLiteral("✏️  Modifier le fournisseur"));
    titleLbl->setObjectName(QStringLiteral("cardTitle"));
    root->addWidget(titleLbl);

    auto *form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_nom = new QLineEdit(f.nom);
    InputFilters::appliquerFiltreNom(m_nom);
    m_nom->setPlaceholderText(QStringLiteral("Ex : Meteo Sensors TN"));
    form->addRow(QStringLiteral("Nom *"), m_nom);

    m_type = new QComboBox;
    m_type->addItems(FOURNISSEUR_TYPES);
    if (!f.type.isEmpty()) m_type->setCurrentText(f.type);
    form->addRow(QStringLiteral("Type *"), m_type);

    m_contact = new QLineEdit(f.contact);
    form->addRow(QStringLiteral("Contact"), m_contact);

    m_email = new QLineEdit(f.email);
    m_email->setPlaceholderText(QStringLiteral("nom@societe.tn"));
    form->addRow(QStringLiteral("E-mail"), m_email);

    m_telephone = new QLineEdit(f.telephone);
    m_telephone->setPlaceholderText(QStringLiteral("71000000"));
    form->addRow(QStringLiteral("Téléphone"), m_telephone);

    m_adresse = new QLineEdit(f.adresse);
    form->addRow(QStringLiteral("Adresse"), m_adresse);

    m_gouv = new QComboBox;
    m_gouv->addItems(GOUVERNORATS);
    if (!f.gouvernorat.isEmpty()) m_gouv->setCurrentText(f.gouvernorat);
    form->addRow(QStringLiteral("Gouvernorat"), m_gouv);

    m_note = new QSpinBox;
    m_note->setRange(1, 5);
    m_note->setValue(f.note);
    form->addRow(QStringLiteral("Évaluation (1 à 5)"), m_note);

    m_delai = new QSpinBox;
    m_delai->setRange(1, 365);
    m_delai->setSuffix(QStringLiteral(" jours"));
    m_delai->setValue(f.delai);
    form->addRow(QStringLiteral("Délai de livraison"), m_delai);

    m_debut = new QDateEdit(f.debut.isValid() ? f.debut : QDate::currentDate());
    m_debut->setCalendarPopup(true);
    m_debut->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    form->addRow(QStringLiteral("Début du contrat"), m_debut);

    m_fin = new QDateEdit(f.fin.isValid() ? f.fin : QDate::currentDate().addYears(1));
    m_fin->setCalendarPopup(true);
    m_fin->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    form->addRow(QStringLiteral("Fin du contrat"), m_fin);

    m_statut = new QComboBox;
    m_statut->addItems(FOURNISSEUR_STATUTS);
    m_statut->setCurrentText(f.statut);
    form->addRow(QStringLiteral("Statut *"), m_statut);

    root->addLayout(form);

    m_errorLabel = new QLabel;
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #ff8a8b; font-size: 12px;"));
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primary"));
    box->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Enregistrer"));
    box->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Annuler"));
    connect(box, &QDialogButtonBox::accepted, this, &FournisseurDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(box);
}

void FournisseurDialog::accept()
{
    m_f.nom         = m_nom->text().trimmed();
    m_f.type        = m_type->currentText();
    m_f.contact     = m_contact->text().trimmed();
    m_f.email       = m_email->text().trimmed();
    m_f.telephone   = m_telephone->text().trimmed();
    m_f.adresse     = m_adresse->text().trimmed();
    m_f.gouvernorat = m_gouv->currentText();
    m_f.note        = m_note->value();
    m_f.delai       = m_delai->value();
    m_f.debut       = m_debut->date();
    m_f.fin         = m_fin->date();
    m_f.statut      = m_statut->currentText();

    QStringList errs;
    if (m_f.nom.isEmpty())
        errs << QStringLiteral("Le nom du fournisseur est obligatoire.");
    else if (!InputFilters::nomValide(m_f.nom))
        errs << QStringLiteral("Le nom du fournisseur ") + InputFilters::NOM_MESSAGE;
    if (!m_f.email.isEmpty() && !m_f.email.contains(QLatin1Char('@')))
        errs << QStringLiteral("L'adresse e-mail doit contenir un @.");
    if (!m_f.telephone.isEmpty() &&
        !QRegularExpression(QStringLiteral("^[0-9 +]{8,15}$")).match(m_f.telephone).hasMatch())
        errs << QStringLiteral("Le téléphone doit contenir 8 à 15 chiffres.");
    if (m_f.fin < m_f.debut)
        errs << QStringLiteral("La fin du contrat doit être après son début.");

    if (!errs.isEmpty()) {
        m_errorLabel->setText(QStringLiteral("⚠  ") + errs.join(QStringLiteral("\n⚠  ")));
        m_errorLabel->show();
        return;
    }
    QDialog::accept();
}

// =====================================================================
//  SmartSupply — page Fournisseurs
// =====================================================================
SmartSupply::SmartSupply(FournisseurRepository *repo, QWidget *parent)
    : QWidget(parent), m_repo(repo)
{
    // ── Root layout ─────────────────────────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // ── Titre ───────────────────────────────────────────────────
    auto *titleRow = new QHBoxLayout;
    auto *pageTitle = new QLabel(QStringLiteral("🏢  Fournisseurs"));
    pageTitle->setObjectName(QStringLiteral("title"));
    auto *pageSub = new QLabel(QStringLiteral("Gestion des fournisseurs, de leurs contrats et de leur évaluation"));
    pageSub->setObjectName(QStringLiteral("muted"));
    titleRow->addWidget(pageTitle);
    titleRow->addWidget(pageSub, 1);
    root->addLayout(titleRow);

    // ── Bandeau d'erreur (visible seulement si la lecture Oracle échoue) ──
    m_errBanner = new QLabel;
    m_errBanner->setWordWrap(true);
    m_errBanner->setStyleSheet(QStringLiteral(
        "color: #ff8a8b; background: #3a1d26; border: 1px solid #cc4e4f; "
        "border-radius: 8px; padding: 8px 12px;"));
    m_errBanner->hide();
    root->addWidget(m_errBanner);

    // ── KPI row ─────────────────────────────────────────────────
    auto *kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(12);
    kpiRow->addWidget(makeKpiCard(QStringLiteral("🏢"), QStringLiteral("Fournisseurs actifs"),     m_kpiActifs));
    kpiRow->addWidget(makeKpiCard(QStringLiteral("⭐"), QStringLiteral("Note moyenne"),             m_kpiNote));
    kpiRow->addWidget(makeKpiCard(QStringLiteral("🔔"), QStringLiteral("Contrats à renouveler"),   m_kpiRenouv));
    kpiRow->addWidget(makeKpiCard(QStringLiteral("🚚"), QStringLiteral("Délai moyen de livraison"), m_kpiDelai));
    root->addLayout(kpiRow);

    // ── Corps : types | tableau | détail ────────────────────────
    auto *body = new QHBoxLayout;
    body->setSpacing(12);

    // LEFT : panneau types
    auto *catPanel = new QFrame;
    catPanel->setObjectName(QStringLiteral("catPanel"));
    catPanel->setFixedWidth(250);
    auto *catV = new QVBoxLayout(catPanel);
    catV->setContentsMargins(12, 14, 12, 14);
    catV->setSpacing(6);

    auto *catTitle = new QLabel(QStringLiteral("Types de fournisseurs"));
    catTitle->setObjectName(QStringLiteral("cardTitle"));
    catV->addWidget(catTitle);
    catV->addSpacing(4);

    m_typeGroup = new QButtonGroup(this);
    m_typeGroup->setExclusive(true);

    auto *btnAll = new QPushButton;
    btnAll->setObjectName(QStringLiteral("catBtn"));
    btnAll->setCheckable(true);
    btnAll->setChecked(true);
    m_typeGroup->addButton(btnAll, -1);
    catV->addWidget(btnAll);

    for (int i = 0; i < FOURNISSEUR_TYPES.size(); ++i) {
        auto *btn = new QPushButton;
        btn->setObjectName(QStringLiteral("catBtn"));
        btn->setCheckable(true);
        m_typeGroup->addButton(btn, i);
        catV->addWidget(btn);
    }
    catV->addStretch(1);
    body->addWidget(catPanel);

    // CENTER : tableau + actions
    auto *centerCard = new QFrame;
    centerCard->setObjectName(QStringLiteral("card"));
    auto *centerV = new QVBoxLayout(centerCard);
    centerV->setContentsMargins(14, 12, 14, 12);
    centerV->setSpacing(10);

    auto *filterRow = new QHBoxLayout;
    filterRow->setSpacing(8);
    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("🔍  Rechercher par nom, contact, e-mail, gouvernorat…"));
    filterRow->addWidget(m_search, 1);

    m_statutFilter = new QComboBox;
    m_statutFilter->addItem(QStringLiteral("Tous les statuts"), QString());
    for (const QString &s : FOURNISSEUR_STATUTS)
        m_statutFilter->addItem(s, s);
    m_statutFilter->setFixedWidth(180);
    filterRow->addWidget(m_statutFilter);
    centerV->addLayout(filterRow);

    m_model = new QStandardItemModel(0, COL_COUNT, this);
    m_model->setHorizontalHeaderLabels({QStringLiteral("Fournisseur"), QStringLiteral("Contact"),
                                        QStringLiteral("Évaluation"),  QStringLiteral("Délai"),
                                        QStringLiteral("Contrat"),     QStringLiteral("Statut")});
    m_model->setSortRole(SORT_ROLE);

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(COL_NOM,     QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(COL_CONTACT, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_NOTE,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_DELAI,   QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_CONTRAT, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_STATUT,  QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(COL_STATUT, 130);
    m_table->setItemDelegateForColumn(COL_STATUT, new FournisseurStatutDelegate(this));
    m_table->setMinimumHeight(300);
    m_table->sortByColumn(COL_NOM, Qt::AscendingOrder);
    centerV->addWidget(m_table, 1);

    auto *actRow = new QHBoxLayout;
    actRow->setSpacing(8);
    auto *btnAdd = new QPushButton(QStringLiteral("➕  Nouveau fournisseur"));
    btnAdd->setObjectName(QStringLiteral("primary"));
    actRow->addWidget(btnAdd);
    actRow->addStretch(1);

    auto *btnSuggest = new QPushButton(QStringLiteral("⚡  Meilleur fournisseur"));
    actRow->addWidget(btnSuggest);

    m_btnEdit = new QPushButton(QStringLiteral("✏  Modifier"));
    m_btnEdit->setEnabled(false);
    actRow->addWidget(m_btnEdit);

    m_btnDelete = new QPushButton(QStringLiteral("🗑  Supprimer"));
    m_btnDelete->setObjectName(QStringLiteral("danger"));
    m_btnDelete->setEnabled(false);
    actRow->addWidget(m_btnDelete);
    centerV->addLayout(actRow);

    m_sugRes = new QLabel;
    m_sugRes->setObjectName(QStringLiteral("muted"));
    m_sugRes->setWordWrap(true);
    centerV->addWidget(m_sugRes);

    body->addWidget(centerCard, 1);

    // RIGHT : détail
    QVBoxLayout *detailLayout = nullptr;
    auto *detailCard = makeCard(detailLayout);
    detailCard->setFixedWidth(280);
    auto *detailHdr = new QLabel(QStringLiteral("Détail"));
    detailHdr->setObjectName(QStringLiteral("cardTitle"));
    detailLayout->addWidget(detailHdr);
    detailLayout->addWidget(makeSeparator());

    m_detailTitle = new QLabel(QStringLiteral("—"));
    m_detailTitle->setWordWrap(true);
    m_detailTitle->setStyleSheet(QStringLiteral("font-weight: 600; color: #e6f1f5;"));
    detailLayout->addWidget(m_detailTitle);

    m_detailBody = new QLabel;
    m_detailBody->setWordWrap(true);
    m_detailBody->setObjectName(QStringLiteral("muted"));
    m_detailBody->setAlignment(Qt::AlignTop);
    m_detailBody->setTextFormat(Qt::RichText);
    detailLayout->addWidget(m_detailBody);
    detailLayout->addStretch(1);

    body->addWidget(detailCard);
    root->addLayout(body, 1);

    // ── Connexions ──────────────────────────────────────────────
    connect(m_repo,         &FournisseurRepository::changed, this, &SmartSupply::refresh);
    connect(m_search,       &QLineEdit::textChanged,         this, &SmartSupply::applyFilters);
    connect(m_statutFilter, &QComboBox::currentIndexChanged, this, &SmartSupply::applyFilters);
    connect(m_typeGroup,    &QButtonGroup::idClicked,        this, [this](int) {
        m_sugRes->clear();
        applyFilters();
    });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SmartSupply::updateDetail);
    connect(btnAdd,     &QPushButton::clicked, this, &SmartSupply::onAdd);
    connect(btnSuggest, &QPushButton::clicked, this, &SmartSupply::onSuggest);
    connect(m_btnEdit,  &QPushButton::clicked, this, &SmartSupply::onEdit);
    connect(m_btnDelete,&QPushButton::clicked, this, &SmartSupply::onDelete);
    connect(m_table, &QTableView::doubleClicked, this, [this](const QModelIndex &) { onEdit(); });

    refresh();
}

void SmartSupply::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    // Relit Oracle à chaque fois qu'on ouvre l'onglet : on voit aussi les lignes
    // ajoutées depuis SQL Developer (à condition qu'elles soient COMMITées).
    m_repo->reload();
    refresh();
}

// ---------------------------------------------------------------
void SmartSupply::refresh()
{
    const QList<Fournisseur> all = m_repo->all();

    // Bandeau d'erreur
    if (m_repo->lastError().isEmpty()) {
        m_errBanner->hide();
    } else {
        m_errBanner->setText(QStringLiteral("⚠  Lecture des fournisseurs impossible :\n") + m_repo->lastError());
        m_errBanner->show();
    }

    // KPIs (moyennes calculées sur les fournisseurs non archivés)
    int actifs = 0, nonArch = 0, renouv = 0;
    double sumNote = 0, sumDelai = 0;
    for (const Fournisseur &f : all) {
        if (f.statut == QStringLiteral("Actif")) ++actifs;
        if (f.statut != QStringLiteral("Archivé")) {
            ++nonArch;
            sumNote  += f.note;
            sumDelai += f.delai;
        }
        if (f.aRenouveler()) ++renouv;
    }
    m_kpiActifs->setText(QStringLiteral("%1 / %2").arg(actifs).arg(all.size()));
    m_kpiNote->setText(nonArch > 0 ? QString::number(sumNote / nonArch, 'f', 1) + QStringLiteral(" / 5")
                                   : QStringLiteral("—"));
    m_kpiRenouv->setText(QString::number(renouv));
    m_kpiDelai->setText(nonArch > 0 ? QString::number(qRound(sumDelai / nonArch)) + QStringLiteral(" j")
                                    : QStringLiteral("—"));

    // Compteurs par type
    for (auto *btn : m_typeGroup->buttons()) {
        const int bid = m_typeGroup->id(btn);
        if (bid < 0) {
            btn->setText(QStringLiteral("🗂   Tous les types   (%1)").arg(all.size()));
            continue;
        }
        const QString &t = FOURNISSEUR_TYPES[bid];
        int nb = 0;
        for (const Fournisseur &f : all) if (f.type == t) ++nb;
        btn->setText(fournisseurTypeIcon(t) + QStringLiteral("   ") + t
                     + QStringLiteral("   (%1)").arg(nb));
    }

    applyFilters();
}

void SmartSupply::applyFilters()
{
    const QString search = m_search->text().trimmed().toLower();
    const QString statut = m_statutFilter->currentData().toString();
    const int     typeId = m_typeGroup->checkedId();
    const QString type   = (typeId >= 0) ? FOURNISSEUR_TYPES[typeId] : QString();

    const int prevId = currentId();
    m_model->removeRows(0, m_model->rowCount());

    for (const Fournisseur &f : m_repo->all()) {
        if (!type.isEmpty()   && f.type   != type)   continue;
        if (!statut.isEmpty() && f.statut != statut) continue;
        if (!search.isEmpty()
            && !f.nom.toLower().contains(search)
            && !f.contact.toLower().contains(search)
            && !f.email.toLower().contains(search)
            && !f.gouvernorat.toLower().contains(search))
            continue;

        bool alerte = false;
        const QString contrat = etatContrat(f, &alerte);

        auto makeItem = [&](const QString &txt, const QVariant &sortKey) {
            auto *it = new QStandardItem(txt);
            it->setData(f.id, Qt::UserRole);
            it->setData(sortKey, SORT_ROLE);
            it->setEditable(false);
            return it;
        };

        auto *iNom     = makeItem(f.nom,     f.nom.toLower());
        auto *iContact = makeItem(f.contact, f.contact.toLower());
        auto *iNote    = makeItem(etoiles(f.note), f.note);
        iNote->setForeground(STAR_GOLD);
        auto *iDelai   = makeItem(QStringLiteral("%1 j").arg(f.delai), f.delai);
        auto *iContrat = makeItem(contrat, f.fin);
        if (alerte) iContrat->setForeground(ALERT_RED);
        auto *iStatut  = makeItem(f.statut, f.statut);

        m_model->appendRow({iNom, iContact, iNote, iDelai, iContrat, iStatut});
    }

    // QStandardItemModel ne se retrie pas tout seul après appendRow
    m_model->sort(m_table->horizontalHeader()->sortIndicatorSection(),
                  m_table->horizontalHeader()->sortIndicatorOrder());

    if (prevId >= 0) selectById(prevId);
    updateDetail();
}

void SmartSupply::updateDetail()
{
    const int id = currentId();
    const bool hasSelection = (id >= 0);
    m_btnEdit->setEnabled(hasSelection);
    m_btnDelete->setEnabled(hasSelection);

    if (!hasSelection) {
        m_detailTitle->setText(QStringLiteral("—"));
        m_detailBody->clear();
        return;
    }

    const auto opt = m_repo->find(id);
    if (!opt) return;
    const Fournisseur &f = *opt;

    m_detailTitle->setText(f.nom);

    bool alerte = false;
    const QString etat = etatContrat(f, &alerte);
    const QString contrat = fmtDate(f.debut) + QStringLiteral(" → ") + fmtDate(f.fin)
                            + (alerte ? QStringLiteral("<br><span style='color:%1'>%2</span>")
                                            .arg(ALERT_RED.name(), esc(etat))
                                      : QString());
    auto val = [](const QString &s) { return s.isEmpty() ? QStringLiteral("—") : esc(s); };
    auto ligne = [](const QString &label, const QString &html) {
        return QStringLiteral("<b>") + label + QStringLiteral(" :</b> ") + html + QStringLiteral("<br>");
    };

    QString body;
    body += ligne(QStringLiteral("Type"),    esc(f.type));
    body += ligne(QStringLiteral("Statut"),
                  QStringLiteral("<span style='color:") + statutColor(f.statut).name() + QStringLiteral("'>")
                  + esc(f.statut) + QStringLiteral("</span>"));
    body += ligne(QStringLiteral("Contact"), val(f.contact));
    body += ligne(QStringLiteral("E-mail"),  val(f.email));
    body += ligne(QStringLiteral("Tél."),    val(f.telephone));
    body += ligne(QStringLiteral("Adresse"), val(f.adresse) + QStringLiteral(" (") + val(f.gouvernorat) + QStringLiteral(")"));
    body += ligne(QStringLiteral("Évaluation"),
                  QStringLiteral("<span style='color:") + STAR_GOLD.name() + QStringLiteral("'>")
                  + etoiles(f.note) + QStringLiteral("</span> ") + QString::number(f.note) + QStringLiteral("/5"));
    body += ligne(QStringLiteral("Livraison"), QString::number(f.delai) + QStringLiteral(" jours"));
    body += ligne(QStringLiteral("Contrat"), contrat);
    m_detailBody->setText(body);

}

// ---------------------------------------------------------------
void SmartSupply::onAdd()
{
    Fournisseur base;
    const int typeId = m_typeGroup->checkedId();
    if (typeId >= 0) base.type = FOURNISSEUR_TYPES[typeId];

    FournisseurDialog dlg(m_repo, base, this);
    if (dlg.exec() != QDialog::Accepted) return;

    const int newId = m_repo->add(dlg.result());
    if (newId < 0) {
        QMessageBox::critical(this, QStringLiteral("Enregistrement impossible"), m_repo->lastError());
        return;
    }
    selectById(newId);
}

void SmartSupply::onEdit()
{
    const int id = currentId();
    if (id < 0) return;
    const auto opt = m_repo->find(id);
    if (!opt) return;

    FournisseurDialog dlg(m_repo, *opt, this);
    if (dlg.exec() != QDialog::Accepted) return;

    if (!m_repo->update(dlg.result())) {
        QMessageBox::critical(this, QStringLiteral("Enregistrement impossible"), m_repo->lastError());
        return;
    }
    selectById(id);
}

void SmartSupply::onDelete()
{
    const int id = currentId();
    if (id < 0) return;
    const auto opt = m_repo->find(id);
    if (!opt) return;

    const int ret = QMessageBox::warning(
        this,
        QStringLiteral("Supprimer le fournisseur"),
        QStringLiteral("Supprimer « %1 » ?\nCette action est irréversible.").arg(opt->nom),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (ret != QMessageBox::Yes) return;

    QString err;
    if (!m_repo->remove(id, &err))
        QMessageBox::critical(this, QStringLiteral("Suppression impossible"), err);
}

void SmartSupply::onSuggest()
{
    const int typeId = m_typeGroup->checkedId();
    m_sugRes->setText(m_repo->suggest(typeId >= 0 ? FOURNISSEUR_TYPES[typeId] : QString()));
}

// ---------------------------------------------------------------
int SmartSupply::currentId() const
{
    const QModelIndexList sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return -1;
    return sel.first().data(Qt::UserRole).toInt();
}

void SmartSupply::selectById(int id)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (m_model->item(row, 0)->data(Qt::UserRole).toInt() == id) {
            m_table->selectRow(row);
            m_table->scrollTo(m_model->index(row, 0));
            return;
        }
    }
}
