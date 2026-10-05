#include "emploipage.h"
#include "theme.h"
#include "uihelpers.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
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
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QVBoxLayout>

#include <QChart>
#include <QChartView>
#include <QPieSeries>
#include <QPieSlice>

// Badge delegate for agent status
class AgentStatutDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
        const QString statut = idx.data().toString();
        QColor bg, fg;
        if (statut == QLatin1String("En service")) {
            bg = QColor(0x3F, 0xC9, 0x8F, 50); fg = QColor(0x3F, 0xC9, 0x8F);
        } else if (statut == QLatin1String("Disponible")) {
            bg = QColor(0x19, 0xC6, 0xB7, 50); fg = QColor(0x19, 0xC6, 0xB7);
        } else if (statut == QLatin1String("En congé")) {
            bg = QColor(0xE4, 0x97, 0x40, 50); fg = QColor(0xE4, 0x97, 0x40);
        } else {
            bg = QColor(0xE0, 0x5A, 0x5B, 50); fg = QColor(0xE0, 0x5A, 0x5B);
        }

        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        if (opt.state & QStyle::State_Selected)
            p->fillRect(opt.rect, QColor(0x12, 0x58, 0x6a, 120));

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
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

EmploiPage::EmploiPage(QWidget *parent) : QWidget(parent)
{
    initSampleData();

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(14);

    // Title
    auto *titleRow = new QHBoxLayout;
    auto *pageTitle = new QLabel(QStringLiteral("📅  Gestion d'emploi du temps & Personnel"));
    pageTitle->setObjectName("title");
    auto *pageSub = new QLabel(QStringLiteral("Planning des équipes et affectation des agents sur le terrain"));
    pageSub->setObjectName("muted");
    titleRow->addWidget(pageTitle);
    titleRow->addWidget(pageSub, 1);
    root->addLayout(titleRow);

    // KPI Cards
    auto *kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(12);
    kpiRow->addWidget(makeKpiCard("👥", "Total Agents", m_kpiTotal));
    kpiRow->addWidget(makeKpiCard("⚡", "En service", m_kpiService));
    kpiRow->addWidget(makeKpiCard("✅", "Disponibles", m_kpiDispo));
    kpiRow->addWidget(makeKpiCard("🏖", "En congé", m_kpiConge));
    root->addLayout(kpiRow);

    // Main 3-column layout
    auto *cols = new QHBoxLayout;
    cols->setSpacing(12);

    // LEFT: Role filter sidebar
    auto *rolePanel = new QFrame;
    rolePanel->setObjectName("catPanel");
    rolePanel->setFixedWidth(210);
    auto *roleV = new QVBoxLayout(rolePanel);
    roleV->setContentsMargins(12, 14, 12, 14);
    roleV->setSpacing(6);

    auto *roleTitle = new QLabel(QStringLiteral("Rôles & Postes"));
    roleTitle->setObjectName("cardTitle");
    roleV->addWidget(roleTitle);

    m_roleGroup = new QButtonGroup(this);
    m_roleGroup->setExclusive(true);

    const QStringList roles = {"Tous les rôles", "Technicien Météo", "Météorologue", "Superviseur", "Agent de terrain"};
    for (int i = 0; i < roles.size(); ++i) {
        auto *btn = new QPushButton((i == 0 ? "🗂  " : "👤  ") + roles[i]);
        btn->setObjectName("catBtn");
        btn->setCheckable(true);
        if (i == 0) btn->setChecked(true);
        m_roleGroup->addButton(btn, i);
        roleV->addWidget(btn);
    }
    roleV->addStretch(1);
    connect(m_roleGroup, &QButtonGroup::idClicked, this, [this](int) { applyFilters(); });

    cols->addWidget(rolePanel);

    // CENTER: Table & Filters
    auto *centerCard = new QFrame;
    centerCard->setObjectName("card");
    auto *centerV = new QVBoxLayout(centerCard);
    centerV->setContentsMargins(14, 12, 14, 12);
    centerV->setSpacing(10);

    auto *filterRow = new QHBoxLayout;
    m_search = new QLineEdit;
    m_search->setPlaceholderText("🔍  Rechercher agent, zone, rôle...");
    filterRow->addWidget(m_search, 1);

    m_statutFilter = new QComboBox;
    m_statutFilter->addItem(QStringLiteral("Tous les statuts"), QString());
    m_statutFilter->addItems({"En service", "Disponible", "En congé", "Formation"});
    m_statutFilter->setFixedWidth(180);
    filterRow->addWidget(m_statutFilter);
    centerV->addLayout(filterRow);

    m_model = new QStandardItemModel(0, 5, this);
    m_model->setHorizontalHeaderLabels({"Nom & Prénom", "Rôle", "Statut", "Zone Affectée", "Quart de travail"});

    m_table = new QTableView;
    m_table->setModel(m_model);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->resizeSection(2, 140);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setItemDelegateForColumn(2, new AgentStatutDelegate(this));
    m_table->setMinimumHeight(300);
    centerV->addWidget(m_table, 1);

    // Action buttons
    auto *actRow = new QHBoxLayout;
    auto *btnAdd = new QPushButton(QStringLiteral("➕  Nouvel agent"));
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

    // RIGHT: Donut Chart & Details
    auto *rightV = new QVBoxLayout;
    rightV->setSpacing(12);

    auto *chartCard = new QFrame;
    chartCard->setObjectName("card");
    chartCard->setFixedWidth(240);
    auto *chartV = new QVBoxLayout(chartCard);
    chartV->setContentsMargins(14, 12, 14, 12);
    chartV->setSpacing(8);

    auto *chartTitle = new QLabel(QStringLiteral("Disponibilité des équipes"));
    chartTitle->setObjectName("cardTitle");
    chartV->addWidget(chartTitle);

    m_pieSeries = new QPieSeries;
    m_pieSeries->setHoleSize(0.52);
    m_pieSeries->setPieSize(0.82);

    auto *chart = new QChart;
    chart->addSeries(m_pieSeries);
    chart->setBackgroundBrush(Qt::transparent);
    chart->setMargins({0, 0, 0, 0});
    chart->legend()->hide();

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(Qt::transparent);
    m_chartView->setFixedHeight(180);

    m_chartCenter = new QLabel(m_chartView);
    m_chartCenter->setAlignment(Qt::AlignCenter);
    m_chartCenter->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_chartCenter->setStyleSheet("color: white; font-size: 16px; font-weight: 700; background: transparent;");
    m_chartCenter->setGeometry(0, 0, 240, 180);

    chartV->addWidget(m_chartView);
    rightV->addWidget(chartCard);

    // Details panel
    QVBoxLayout *detailL = nullptr;
    auto *detailCard = makeCard(detailL);
    detailCard->setFixedWidth(240);
    auto *dHdr = new QLabel(QStringLiteral("Fiche Agent"));
    dHdr->setObjectName("cardTitle");
    detailL->addWidget(dHdr);
    detailL->addWidget(makeSeparator());

    m_detailTitle = new QLabel(QStringLiteral("—"));
    m_detailTitle->setStyleSheet("font-weight: 600; color: #e6f1f5;");
    detailL->addWidget(m_detailTitle);

    m_detailBody = new QLabel;
    m_detailBody->setWordWrap(true);
    m_detailBody->setObjectName("muted");
    detailL->addWidget(m_detailBody, 1);

    rightV->addWidget(detailCard, 1);
    cols->addLayout(rightV);
    root->addLayout(cols, 1);

    // Connections
    connect(m_search, &QLineEdit::textChanged, this, &EmploiPage::applyFilters);
    connect(m_statutFilter, &QComboBox::currentIndexChanged, this, &EmploiPage::applyFilters);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        const int id = currentId();
        const bool hasSel = (id >= 0);
        m_btnEdit->setEnabled(hasSel);
        m_btnDelete->setEnabled(hasSel);

        if (!hasSel) {
            m_detailTitle->setText(QStringLiteral("—"));
            m_detailBody->clear();
            return;
        }
        for (const auto &a : m_agents) {
            if (a.id == id) {
                m_detailTitle->setText(a.prenom + " " + a.nom);
                m_detailBody->setText(QString("<b>Rôle :</b> %1<br>"
                                              "<b>Statut :</b> %2<br>"
                                              "<b>Zone :</b> %3<br>"
                                              "<b>Quart :</b> %4")
                                      .arg(a.role, a.statut, a.zoneAffectee, a.quartTravail));
                break;
            }
        }
    });

    connect(btnAdd, &QPushButton::clicked, this, &EmploiPage::onAddAgent);
    connect(m_btnEdit, &QPushButton::clicked, this, &EmploiPage::onEditAgent);
    connect(m_btnDelete, &QPushButton::clicked, this, &EmploiPage::onDeleteAgent);

    refresh();
}

void EmploiPage::initSampleData() {
    m_agents = {
        {m_nextId++, "Ben Salah", "Ahmed", "Technicien Météo", "En service", "Tunis Nord", "Matin (06h-14h)"},
        {m_nextId++, "Trabelsi", "Sana", "Météorologue", "Disponible", "Sfax Côte", "Après-midi (14h-22h)"},
        {m_nextId++, "Jlassi", "Karim", "Agent de terrain", "En service", "Tunis Nord", "Matin (06h-14h)"},
        {m_nextId++, "Gharbi", "Meriam", "Superviseur", "Disponible", "Kairouan Plaine", "Matin (06h-14h)"},
        {m_nextId++, "Kouki", "Yasmine", "Météorologue", "En service", "Gabès Industriel", "Après-midi (14h-22h)"},
        {m_nextId++, "Nouioui", "Adem", "Technicien Météo", "En congé", "Tunis Nord", "Nuit (22h-06h)"},
        {m_nextId++, "Ghandour", "Aweb", "Agent de terrain", "En service", "Sfax Côte", "Matin (06h-14h)"},
        {m_nextId++, "Eddine", "Bader", "Superviseur", "Formation", "Kairouan Plaine", "Après-midi (14h-22h)"}
    };
}

void EmploiPage::refresh() {
    int total = m_agents.size();
    int service = 0, dispo = 0, conge = 0;
    for (const auto &a : m_agents) {
        if (a.statut == "En service") service++;
        else if (a.statut == "Disponible") dispo++;
        else if (a.statut == "En congé") conge++;
    }

    m_kpiTotal->setText(QString::number(total));
    m_kpiService->setText(QString::number(service));
    m_kpiDispo->setText(QString::number(dispo));
    m_kpiConge->setText(QString::number(conge));

    m_pieSeries->clear();
    auto addSlice = [&](int val, const QColor &col) {
        if (val <= 0) return;
        auto *sl = m_pieSeries->append(QString::number(val), val);
        sl->setColor(col);
        sl->setBorderColor(Qt::transparent);
    };
    addSlice(service, QColor(0x3F, 0xC9, 0x8F));
    addSlice(dispo, QColor(0x19, 0xC6, 0xB7));
    addSlice(conge, QColor(0xE4, 0x97, 0x40));

    int pct = total > 0 ? (service * 100 / total) : 0;
    m_chartCenter->setText(QString("%1%\nEn service").arg(pct));

    applyFilters();
}

void EmploiPage::applyFilters() {
    const QString search = m_search->text().trimmed().toLower();
    const QString statut = m_statutFilter->currentText();
    const int roleId = m_roleGroup->checkedId();
    const QStringList roles = {"", "Technicien Météo", "Météorologue", "Superviseur", "Agent de terrain"};
    const QString selectedRole = (roleId > 0 && roleId < roles.size()) ? roles[roleId] : QString();

    m_model->removeRows(0, m_model->rowCount());

    for (const auto &a : m_agents) {
        if (!selectedRole.isEmpty() && a.role != selectedRole) continue;
        if (m_statutFilter->currentIndex() > 0 && a.statut != statut) continue;
        if (!search.isEmpty() && !a.nom.toLower().contains(search) &&
            !a.prenom.toLower().contains(search) &&
            !a.zoneAffectee.toLower().contains(search) &&
            !a.role.toLower().contains(search)) continue;

        QList<QStandardItem*> row;
        auto makeIt = [&](const QString &txt) {
            auto *it = new QStandardItem(txt);
            it->setData(a.id, Qt::UserRole);
            it->setEditable(false);
            return it;
        };
        row << makeIt(a.prenom + " " + a.nom)
            << makeIt(a.role)
            << makeIt(a.statut)
            << makeIt(a.zoneAffectee)
            << makeIt(a.quartTravail);
        m_model->appendRow(row);
    }
}

int EmploiPage::currentId() const {
    const auto sel = m_table->selectionModel()->selectedRows();
    if (sel.isEmpty()) return -1;
    return sel.first().data(Qt::UserRole).toInt();
}

void EmploiPage::onAddAgent() {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Ajouter un agent"));
    dlg.resize(380, 260);

    auto *fl = new QFormLayout(&dlg);
    auto *nomE = new QLineEdit;
    auto *prenomE = new QLineEdit;
    auto *roleC = new QComboBox;
    roleC->addItems({"Technicien Météo", "Météorologue", "Superviseur", "Agent de terrain"});
    auto *statutC = new QComboBox;
    statutC->addItems({"En service", "Disponible", "En congé", "Formation"});
    auto *zoneE = new QLineEdit;
    zoneE->setText("Tunis Nord");
    auto *quartC = new QComboBox;
    quartC->addItems({"Matin (06h-14h)", "Après-midi (14h-22h)", "Nuit (22h-06h)"});

    fl->addRow("Nom :", nomE);
    fl->addRow("Prénom :", prenomE);
    fl->addRow("Rôle :", roleC);
    fl->addRow("Statut :", statutC);
    fl->addRow("Zone affectée :", zoneE);
    fl->addRow("Quart :", quartC);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    fl->addRow(box);

    if (dlg.exec() == QDialog::Accepted && !nomE->text().trimmed().isEmpty()) {
        m_agents.append({m_nextId++, nomE->text().trimmed(), prenomE->text().trimmed(),
                        roleC->currentText(), statutC->currentText(),
                        zoneE->text().trimmed(), quartC->currentText()});
        refresh();
    }
}

void EmploiPage::onEditAgent() {
    int id = currentId();
    if (id < 0) return;

    int idx = -1;
    for (int i = 0; i < m_agents.size(); ++i) {
        if (m_agents[i].id == id) { idx = i; break; }
    }
    if (idx < 0) return;

    AgentEmploi &a = m_agents[idx];

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Modifier l'agent"));
    dlg.resize(380, 260);

    auto *fl = new QFormLayout(&dlg);
    auto *nomE = new QLineEdit(a.nom);
    auto *prenomE = new QLineEdit(a.prenom);
    auto *roleC = new QComboBox;
    roleC->addItems({"Technicien Météo", "Météorologue", "Superviseur", "Agent de terrain"});
    roleC->setCurrentText(a.role);
    auto *statutC = new QComboBox;
    statutC->addItems({"En service", "Disponible", "En congé", "Formation"});
    statutC->setCurrentText(a.statut);
    auto *zoneE = new QLineEdit(a.zoneAffectee);
    auto *quartC = new QComboBox;
    quartC->addItems({"Matin (06h-14h)", "Après-midi (14h-22h)", "Nuit (22h-06h)"});
    quartC->setCurrentText(a.quartTravail);

    fl->addRow("Nom :", nomE);
    fl->addRow("Prénom :", prenomE);
    fl->addRow("Rôle :", roleC);
    fl->addRow("Statut :", statutC);
    fl->addRow("Zone affectée :", zoneE);
    fl->addRow("Quart :", quartC);

    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    fl->addRow(box);

    if (dlg.exec() == QDialog::Accepted) {
        a.nom = nomE->text().trimmed();
        a.prenom = prenomE->text().trimmed();
        a.role = roleC->currentText();
        a.statut = statutC->currentText();
        a.zoneAffectee = zoneE->text().trimmed();
        a.quartTravail = quartC->currentText();
        refresh();
    }
}

void EmploiPage::onDeleteAgent() {
    int id = currentId();
    if (id < 0) return;

    if (QMessageBox::question(this, "Supprimer agent", "Voulez-vous vraiment supprimer cet agent ?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        for (int i = 0; i < m_agents.size(); ++i) {
            if (m_agents[i].id == id) { m_agents.removeAt(i); break; }
        }
        refresh();
    }
}
