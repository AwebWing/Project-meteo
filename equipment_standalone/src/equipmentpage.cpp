#include "equipmentpage.h"
#include "equipmentdialog.h"
#include "equipmentrepository.h"
#include "equipmentservice.h"
#include "theme.h"
#include "uihelpers.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStackedLayout>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

#include <QChart>
#include <QChartView>
#include <QPieSeries>
#include <QPieSlice>

// ================================================================
//  Delegate : affiche le statut comme un badge coloré arrondi
// ================================================================
class StatutDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        const QString statut = idx.data().toString();

        QColor bg, fg;
        if (statut == QLatin1String("Disponible")) {
            bg = QColor(0x3F, 0xC9, 0x8F, 50);
            fg = QColor(0x3F, 0xC9, 0x8F);
        } else if (statut == QLatin1String("En maintenance")) {
            bg = QColor(0xE4, 0x97, 0x40, 50);
            fg = QColor(0xE4, 0x97, 0x40);
        } else {
            bg = QColor(0xE0, 0x5A, 0x5B, 50);
            fg = QColor(0xE0, 0x5A, 0x5B);
        }

        p->save();
        p->setRenderHint(QPainter::Antialiasing);

        // Draw selection background if selected
        if (opt.state & QStyle::State_Selected)
            p->fillRect(opt.rect, QColor(0x12, 0x58, 0x6a, 120));

        // Badge pill
        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        p->setBrush(bg);
        p->setPen(QPen(fg, 1));
        p->drawRoundedRect(r, 9, 9);

        // Text
        p->setPen(fg);
        QFont f = opt.font;
        f.setWeight(QFont::DemiBold);
        p->setFont(f);
        p->drawText(r, Qt::AlignCenter, statut);
        p->restore();
    }
};

// ================================================================
//  Column indices
// ================================================================
static constexpr int COL_CODE  = 0;
static constexpr int COL_NOM   = 1;
static constexpr int COL_CAT   = 2;
static constexpr int COL_STAT  = 3;
static constexpr int COL_LOC   = 4;
static constexpr int COL_DATE  = 5;
static constexpr int COL_COUNT = 6;

// ================================================================
//  EquipmentPage
// ================================================================
EquipmentPage::EquipmentPage(EquipmentRepository *repo, QWidget *parent)
    : QWidget(parent), m_repo(repo)
{
    // ── Root layout ─────────────────────────────────────────────
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // ── Page title ───────────────────────────────────────────────
    auto *titleRow = new QHBoxLayout;
    auto *pageTitle = new QLabel(QStringLiteral("🧰  Équipements"));
    pageTitle->setObjectName("title");
    auto *pageSub = new QLabel(QStringLiteral("Gestion du matériel et des ressources"));
    pageSub->setObjectName("muted");
    titleRow->addWidget(pageTitle);
    titleRow->addWidget(pageSub, 1);
    root->addLayout(titleRow);

    // ── KPI row ──────────────────────────────────────────────────
    auto *kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(12);
    kpiRow->addWidget(makeKpiCard("📦", "Total équipements", m_kpiTotal));
    kpiRow->addWidget(makeKpiCard("✅", "Disponibles",       m_kpiDispo));
    kpiRow->addWidget(makeKpiCard("🔧", "En maintenance",    m_kpiMaint));
    kpiRow->addWidget(makeKpiCard("❌", "Hors service",      m_kpiHs));
    root->addLayout(kpiRow);

    // ── Main 3-column row ─────────────────────────────────────────
    auto *cols = new QHBoxLayout;
    cols->setSpacing(12);

    // ═══════════════════════════════════════════════════════════
    //  LEFT: Category panel
    // ═══════════════════════════════════════════════════════════
    auto *catPanel = new QFrame;
    catPanel->setObjectName("catPanel");
    catPanel->setFixedWidth(210);
    auto *catV = new QVBoxLayout(catPanel);
    catV->setContentsMargins(12, 14, 12, 14);
    catV->setSpacing(6);

    auto *catTitle = new QLabel(QStringLiteral("Catégories"));
    catTitle->setObjectName("cardTitle");
    catV->addWidget(catTitle);

    auto *catSearch = new QLineEdit;
    catSearch->setPlaceholderText("🔍  Rechercher…");
    catV->addWidget(catSearch);
    catV->addSpacing(4);

    m_catGroup = new QButtonGroup(this);
    m_catGroup->setExclusive(true);

    // "Toutes" button (id = -1 means no filter)
    auto *btnAll = new QPushButton(QStringLiteral("🗂   Toutes les catégories"));
    btnAll->setObjectName("catBtn");
    btnAll->setCheckable(true);
    btnAll->setChecked(true);
    m_catGroup->addButton(btnAll, -1);
    catV->addWidget(btnAll);

    for (int i = 0; i < EQUIPMENT_CATEGORIES.size(); ++i) {
        const QString &cat = EQUIPMENT_CATEGORIES[i];
        auto *btn = new QPushButton(categoryIcon(cat) + QStringLiteral("   ") + cat);
        btn->setObjectName("catBtn");
        btn->setCheckable(true);
        m_catGroup->addButton(btn, i);
        catV->addWidget(btn);
    }
    catV->addStretch(1);

    // Filter category search
    connect(catSearch, &QLineEdit::textChanged, this, [this, catSearch]() {
        const QString txt = catSearch->text().trimmed().toLower();
        // re-filter when user types in category search
        Q_UNUSED(txt)
        applyFilters();
    });
    connect(m_catGroup, &QButtonGroup::idClicked, this, [this](int) { applyFilters(); });

    cols->addWidget(catPanel);

    // ═══════════════════════════════════════════════════════════
    //  CENTER: Search + Table + Actions
    // ═══════════════════════════════════════════════════════════
    auto *centerCard = new QFrame;
    centerCard->setObjectName("card");
    auto *centerV = new QVBoxLayout(centerCard);
    centerV->setContentsMargins(14, 12, 14, 12);
    centerV->setSpacing(10);

    // --- Search + statut filter row ---
    auto *filterRow = new QHBoxLayout;
    filterRow->setSpacing(8);

    m_search = new QLineEdit;
    m_search->setPlaceholderText("🔍  Rechercher par nom, code, localisation…");
    filterRow->addWidget(m_search, 1);

    m_statutFilter = new QComboBox;
    m_statutFilter->addItem(QStringLiteral("Tous les statuts"), QString());
    for (const QString &s : EQUIPMENT_STATUTS)
        m_statutFilter->addItem(s, s);
    m_statutFilter->setFixedWidth(180);
    filterRow->addWidget(m_statutFilter);

    centerV->addLayout(filterRow);

    // --- Table ---
    m_model = new QStandardItemModel(0, COL_COUNT, this);
    m_model->setHorizontalHeaderLabels({
        "Code", "Nom", "Catégorie", "Statut", "Localisation", "Dernière maintenance"
    });

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(COL_NOM,  QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(COL_CAT,  QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_STAT, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(COL_STAT, 150);
    m_table->horizontalHeader()->setSectionResizeMode(COL_CODE, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_LOC,  QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_DATE, QHeaderView::ResizeToContents);
    m_table->setItemDelegateForColumn(COL_STAT, new StatutDelegate(this));
    m_table->setWordWrap(false);
    m_table->setMinimumHeight(300);

    centerV->addWidget(m_table, 1);

    // --- Action buttons row ---
    auto *actRow = new QHBoxLayout;
    actRow->setSpacing(8);

    auto *btnAdd = new QPushButton(QStringLiteral("➕  Nouvel équipement"));
    btnAdd->setObjectName("primary");
    actRow->addWidget(btnAdd);

    actRow->addStretch(1);

    m_btnEdit = new QPushButton(QStringLiteral("✏  Modifier"));
    m_btnEdit->setEnabled(false);
    actRow->addWidget(m_btnEdit);

    m_btnDelete = new QPushButton(QStringLiteral("🗑  Supprimer"));
    m_btnDelete->setObjectName("danger");
    m_btnDelete->setEnabled(false);
    actRow->addWidget(m_btnDelete);

    centerV->addLayout(actRow);
    cols->addWidget(centerCard, 1);

    // ═══════════════════════════════════════════════════════════
    //  RIGHT: Donut chart + detail panel
    // ═══════════════════════════════════════════════════════════
    auto *rightV = new QVBoxLayout;
    rightV->setSpacing(12);

    // --- Donut chart card ---
    auto *chartCard = new QFrame;
    chartCard->setObjectName("card");
    chartCard->setFixedWidth(240);
    auto *chartCardV = new QVBoxLayout(chartCard);
    chartCardV->setContentsMargins(14, 12, 14, 12);
    chartCardV->setSpacing(8);

    auto *chartTitle = new QLabel(QStringLiteral("État du parc"));
    chartTitle->setObjectName("cardTitle");
    chartCardV->addWidget(chartTitle);

    // Chart + overlay
    m_pieSeries = new QPieSeries;
    m_pieSeries->setHoleSize(0.52);
    m_pieSeries->setPieSize(0.82);

    auto *chart = new QChart;
    chart->addSeries(m_pieSeries);
    chart->setBackgroundBrush(Qt::transparent);
    chart->setPlotAreaBackgroundBrush(Qt::transparent);
    chart->setBackgroundRoundness(0);
    chart->setMargins({0, 0, 0, 0});
    chart->legend()->hide();

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);
    m_chartView->setStyleSheet("background: transparent; border: none;");
    m_chartView->setFixedHeight(190);

    // Overlay center label
    m_chartCenter = new QLabel(m_chartView);
    m_chartCenter->setAlignment(Qt::AlignCenter);
    m_chartCenter->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_chartCenter->setStyleSheet("background: transparent;");
    m_chartCenter->setGeometry(0, 0, 240, 190);

    chartCardV->addWidget(m_chartView);

    // Legend rows
    auto mkLegRow = [&](QLabel *&lbl, const QColor &dot, const QString &txt) {
        auto *row = new QHBoxLayout;
        auto *dotLbl = new QLabel;
        dotLbl->setFixedSize(12, 12);
        dotLbl->setStyleSheet(
            QString("background: %1; border-radius: 6px;").arg(dot.name()));
        lbl = new QLabel(txt);
        lbl->setObjectName("muted");
        row->addWidget(dotLbl);
        row->addWidget(lbl, 1);
        chartCardV->addLayout(row);
    };

    mkLegRow(m_legendDispo, QColor(0x3F, 0xC9, 0x8F), "Disponibles");
    mkLegRow(m_legendMaint, QColor(0xE4, 0x97, 0x40), "En maintenance");
    mkLegRow(m_legendHs,   QColor(0xE0, 0x5A, 0x5B), "Hors service");

    rightV->addWidget(chartCard);

    // --- Detail panel ---
    QVBoxLayout *detailLayout = nullptr;
    auto *detailCard = makeCard(detailLayout);
    detailCard->setFixedWidth(240);

    auto *detailHdr = new QLabel(QStringLiteral("Détail"));
    detailHdr->setObjectName("cardTitle");
    detailLayout->addWidget(detailHdr);
    detailLayout->addWidget(makeSeparator());

    m_detailTitle = new QLabel(QStringLiteral("—"));
    m_detailTitle->setWordWrap(true);
    m_detailTitle->setStyleSheet("font-weight: 600; color: #e6f1f5;");
    detailLayout->addWidget(m_detailTitle);

    m_detailBody = new QLabel;
    m_detailBody->setWordWrap(true);
    m_detailBody->setObjectName("muted");
    m_detailBody->setAlignment(Qt::AlignTop);
    detailLayout->addWidget(m_detailBody, 1);

    rightV->addWidget(detailCard, 1);
    cols->addLayout(rightV);

    root->addLayout(cols, 1);

    // ── Connections ─────────────────────────────────────────────
    connect(m_repo, &EquipmentRepository::changed, this, &EquipmentPage::refresh);
    connect(m_search,       &QLineEdit::textChanged,
            this, &EquipmentPage::applyFilters);
    connect(m_statutFilter, &QComboBox::currentIndexChanged,
            this, &EquipmentPage::applyFilters);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &EquipmentPage::updateDetail);
    connect(btnAdd,    &QPushButton::clicked, this, &EquipmentPage::onAdd);
    connect(m_btnEdit, &QPushButton::clicked, this, &EquipmentPage::onEdit);
    connect(m_btnDelete,&QPushButton::clicked, this, &EquipmentPage::onDelete);

    // Double-click opens edit dialog
    connect(m_table, &QTableView::doubleClicked,
            this, [this](const QModelIndex &) { onEdit(); });

    refresh();
}

// ================================================================
//  refresh() — reconstruit le modèle depuis le repo
// ================================================================
void EquipmentPage::refresh()
{
    const auto stats = EquipmentService::statistiques(m_repo->all());

    // Update KPIs
    m_kpiTotal->setText(QString::number(stats.total));
    m_kpiDispo->setText(QString::number(stats.disponibles));
    m_kpiMaint->setText(QString::number(stats.enMaintenance));
    m_kpiHs->setText(QString::number(stats.horsService));

    // Update donut chart
    m_pieSeries->clear();
    auto addSlice = [&](int val, const QColor &col) {
        if (val <= 0) return;
        auto *sl = m_pieSeries->append(QString::number(val), val);
        sl->setColor(col);
        sl->setBorderColor(Qt::transparent);
    };
    addSlice(stats.disponibles,   QColor(0x3F, 0xC9, 0x8F));
    addSlice(stats.enMaintenance, QColor(0xE4, 0x97, 0x40));
    addSlice(stats.horsService,   QColor(0xE0, 0x5A, 0x5B));

    m_chartCenter->setText(
        QString("<div style='text-align:center;'>"
                "<span style='font-size:24px; font-weight:800; color:#3fc98f;'>%1%</span><br>"
                "<span style='font-size:11px; font-weight:600; color:#8fb3c2;'>Disponibles</span>"
                "</div>").arg(static_cast<int>(stats.pctDisponibles)));
    m_chartCenter->setTextFormat(Qt::RichText);

    // Update chart legend
    m_legendDispo->setText(QString("Disponibles    <b style='color:#e6f1f5'>%1</b>").arg(stats.disponibles));
    m_legendMaint->setText(QString("En maintenance <b style='color:#e6f1f5'>%1</b>").arg(stats.enMaintenance));
    m_legendHs->setText(   QString("Hors service   <b style='color:#e6f1f5'>%1</b>").arg(stats.horsService));
    m_legendDispo->setTextFormat(Qt::RichText);
    m_legendMaint->setTextFormat(Qt::RichText);
    m_legendHs->setTextFormat(Qt::RichText);

    // Update category buttons with counts
    const auto counts = EquipmentService::categoryCounts(m_repo->all());
    const auto buttons = m_catGroup->buttons();
    for (auto *btn : buttons) {
        const int bid = m_catGroup->id(btn);
        if (bid < 0) continue;  // "Toutes"
        const QString &cat = EQUIPMENT_CATEGORIES[bid];
        btn->setText(categoryIcon(cat) + QStringLiteral("   ") + cat
                     + QStringLiteral("   (") + QString::number(counts.value(cat)) + ")");
    }

    applyFilters();
}

// ================================================================
//  applyFilters() — filtre la vue du tableau
// ================================================================
void EquipmentPage::applyFilters()
{
    const QString search    = m_search->text().trimmed().toLower();
    const QString statut    = m_statutFilter->currentData().toString();
    const int     catId     = m_catGroup->checkedId();
    const QString categorie = (catId >= 0) ? EQUIPMENT_CATEGORIES[catId] : QString();

    // Save current selection
    const int prevId = currentId();

    m_model->removeRows(0, m_model->rowCount());

    const QList<Equipment> all = m_repo->all();
    for (const Equipment &e : all) {
        if (!categorie.isEmpty() && e.categorie != categorie) continue;
        if (!statut.isEmpty()    && e.statut    != statut)    continue;
        if (!search.isEmpty()) {
            if (!e.nom.toLower().contains(search) &&
                !e.code.toLower().contains(search) &&
                !e.localisation.toLower().contains(search))
                continue;
        }

        QList<QStandardItem *> row;
        auto makeItem = [&](const QString &txt) {
            auto *it = new QStandardItem(txt);
            it->setData(e.id, Qt::UserRole);
            it->setEditable(false);
            return it;
        };
        row << makeItem(e.code)
            << makeItem(e.nom)
            << makeItem(e.categorie)
            << makeItem(e.statut)
            << makeItem(e.localisation)
            << makeItem(e.derniereMaintenance.toString("dd/MM/yyyy"));

        m_model->appendRow(row);
    }

    // Restore selection
    if (prevId >= 0) selectById(prevId);
    updateDetail();
}

// ================================================================
//  updateDetail() — met à jour le panneau de détail
// ================================================================
void EquipmentPage::updateDetail()
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
    const Equipment &e = *opt;

    m_detailTitle->setText(e.nom);

    const QString body =
        QString("<b>Code :</b> %1<br>"
                "<b>Catégorie :</b> %2<br>"
                "<b>Statut :</b> %3<br>"
                "<b>Localisation :</b> %4<br>"
                "<b>Maintenance :</b> %5<br>%6")
        .arg(e.code.isEmpty() ? "—" : e.code)
        .arg(e.categorie)
        .arg(e.statut)
        .arg(e.localisation)
        .arg(e.derniereMaintenance.toString("dd/MM/yyyy"))
        .arg(e.description.isEmpty() ? QString() :
             "<br><b>Description :</b><br>" + e.description);

    m_detailBody->setTextFormat(Qt::RichText);
    m_detailBody->setText(body);
}

// ================================================================
//  CRUD actions
// ================================================================
void EquipmentPage::onAdd()
{
    EquipmentDialog dlg(m_repo, Equipment{}, this);
    if (dlg.exec() != QDialog::Accepted) return;
    const int newId = m_repo->add(dlg.result());
    selectById(newId);
}

void EquipmentPage::onEdit()
{
    const int id = currentId();
    if (id < 0) return;
    const auto opt = m_repo->find(id);
    if (!opt) return;

    EquipmentDialog dlg(m_repo, *opt, this);
    if (dlg.exec() != QDialog::Accepted) return;
    m_repo->update(dlg.result());
    selectById(id);
}

void EquipmentPage::onDelete()
{
    const int id = currentId();
    if (id < 0) return;
    const auto opt = m_repo->find(id);
    if (!opt) return;

    const int ret = QMessageBox::warning(
        this,
        QStringLiteral("Supprimer l'équipement"),
        QStringLiteral("Supprimer « %1 » ?\nCette action est irréversible.").arg(opt->nom),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (ret != QMessageBox::Yes) return;

    QString err;
    if (!m_repo->remove(id, &err))
        QMessageBox::critical(this, QStringLiteral("Erreur"), err);
}

// ================================================================
//  Helpers
// ================================================================
int EquipmentPage::currentId() const
{
    const QModelIndexList sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return -1;
    return sel.first().data(Qt::UserRole).toInt();
}

void EquipmentPage::selectById(int id)
{
    for (int row = 0; row < m_model->rowCount(); ++row) {
        if (m_model->item(row, 0)->data(Qt::UserRole).toInt() == id) {
            m_table->selectRow(row);
            m_table->scrollTo(m_model->index(row, 0));
            return;
        }
    }
}
