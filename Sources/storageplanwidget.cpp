#include "storageplanwidget.h"
#include "equipmentservice.h"
#include "theme.h"
#include <QBrush>
#include <QFont>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QPen>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>

namespace {

constexpr qreal TILE_W = 72, TILE_H = 82, GAP = 10, PAD = 14, HEADER_H = 36;
constexpr int   PAR_LIGNE = 4;                         // tuiles par ligne dans une étagère
constexpr qreal RACK_W = PAD * 2 + PAR_LIGNE * TILE_W + (PAR_LIGNE - 1) * GAP;
constexpr qreal RACK_GAP = 18;

QColor statutColor(const QString &statut) { return Theme::levelColor(statut); }

QString nomCourt(const QString &nom)
{
    return nom.size() > 11 ? nom.left(10) + QStringLiteral("…") : nom;
}

// ---------------------------------------------------------------
//  Tuile d'un équipement : déplaçable à la souris.
// ---------------------------------------------------------------
class EquipmentTile : public QGraphicsRectItem
{
public:
    EquipmentTile(const Equipment &eq, bool selectionne, StoragePlanWidget *plan)
        : QGraphicsRectItem(0, 0, TILE_W, TILE_H), m_eq(eq), m_plan(plan), m_sel(selectionne)
    {
        setFlag(ItemIsMovable);
        setFlag(ItemIsSelectable, false);
        setCursor(Qt::OpenHandCursor);
        setAcceptHoverEvents(true);
        setToolTip(QStringLiteral("%1 (%2)\n%3 — %4\nFournisseur : %5\nGlisser vers une autre étagère pour le déplacer")
                       .arg(eq.nom, eq.code, eq.categorie, eq.statut, eq.fournisseurNom));
    }

    int equipmentId() const { return m_eq.id; }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        p->setRenderHint(QPainter::Antialiasing);
        const QColor col = statutColor(m_eq.statut);
        p->setBrush(m_hover || m_sel ? QColor(0x14, 0x3D, 0x52) : QColor(0x0D, 0x2A, 0x3A));
        p->setPen(QPen(col, m_sel ? 3 : 2));
        p->drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);

        QFont f = p->font();
        f.setPointSize(20);
        p->setFont(f);
        p->setPen(Theme::textColor());
        p->drawText(QRectF(0, 6, TILE_W, 38), Qt::AlignCenter, categoryIcon(m_eq.categorie));

        f.setPointSize(8);
        p->setFont(f);
        p->drawText(QRectF(2, 46, TILE_W - 4, 16), Qt::AlignCenter, nomCourt(m_eq.nom));
        p->setPen(Theme::mutedColor());
        p->drawText(QRectF(2, 62, TILE_W - 4, 16), Qt::AlignCenter, m_eq.code);
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *) override { m_hover = true;  update(); }
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override { m_hover = false; update(); }

    void mousePressEvent(QGraphicsSceneMouseEvent *e) override
    {
        m_start = e->scenePos();
        setZValue(100);                       // au-dessus des étagères pendant le glisser
        setCursor(Qt::ClosedHandCursor);
        QGraphicsRectItem::mousePressEvent(e);
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e) override
    {
        QGraphicsRectItem::mouseMoveEvent(e);
        m_plan->tileDragging(e->scenePos());
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override
    {
        QGraphicsRectItem::mouseReleaseEvent(e);
        setCursor(Qt::OpenHandCursor);
        const bool clic = (e->scenePos() - m_start).manhattanLength() < 5;
        if (clic) {
            setPos(m_homePos);
            m_plan->tileClicked(m_eq.id);
        } else {
            m_plan->tileDropped(m_eq.id, e->scenePos());
        }
    }

public:
    void setHome(const QPointF &p) { m_homePos = p; setPos(p); }

private:
    Equipment m_eq;
    StoragePlanWidget *m_plan;
    bool m_sel;
    bool m_hover = false;
    QPointF m_start, m_homePos;
};

} // namespace

// ============================================================
StoragePlanWidget::StoragePlanWidget(QWidget *parent)
    : QGraphicsView(parent), m_scene(new QGraphicsScene(this)), m_pulse(new QTimer(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setBackgroundBrush(QColor(0x07, 0x15, 0x1F));
    setFrameShape(QFrame::NoFrame);
    setDragMode(QGraphicsView::NoDrag);

    // Les équipements en retard de maintenance clignotent doucement (badge ⚠).
    m_pulse->setInterval(700);
    connect(m_pulse, &QTimer::timeout, this, [this] {
        m_pulseOn = !m_pulseOn;
        for (auto *b : std::as_const(m_alertBadges)) b->setOpacity(m_pulseOn ? 1.0 : 0.25);
    });
    m_pulse->start();
}

void StoragePlanWidget::setEquipments(const QList<Equipment> &list)
{
    m_equipments = list;
    m_enRetard.clear();
    for (const auto &e : EquipmentService::alertesMaintenance(list)) m_enRetard.insert(e.id);
    rebuild();
}

bool StoragePlanWidget::rackExists(const QString &nom) const
{
    for (const auto &e : m_equipments)
        if (e.localisation.compare(nom, Qt::CaseInsensitive) == 0) return true;
    for (const auto &r : m_extraRacks)
        if (r.compare(nom, Qt::CaseInsensitive) == 0) return true;
    return false;
}

bool StoragePlanWidget::addRack(const QString &nom)
{
    const QString n = nom.trimmed();
    if (n.isEmpty() || rackExists(n)) return false;
    m_extraRacks << n;
    rebuild();
    return true;
}

void StoragePlanWidget::setSelected(int id) { m_selected = id; rebuild(); }

int StoragePlanWidget::colonnes() const
{
    const int w = viewport()->width() - 20;
    return std::max(1, int(w / (RACK_W + RACK_GAP)));
}

void StoragePlanWidget::resizeEvent(QResizeEvent *e)
{
    QGraphicsView::resizeEvent(e);
    if (colonnes() != m_colonnes) rebuild();       // on ne reconstruit que si le nombre de colonnes change
}

void StoragePlanWidget::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {    // Ctrl + molette = zoom
        const qreal f = e->angleDelta().y() > 0 ? 1.12 : 1 / 1.12;
        scale(f, f);
        e->accept();
    } else {
        QGraphicsView::wheelEvent(e);
    }
}

void StoragePlanWidget::rebuild()
{
    m_scene->clear();
    m_racks.clear();
    m_alertBadges.clear();
    m_colonnes = colonnes();

    // 1) Étagères = localisations distinctes (+ étagères vides créées à la main)
    QStringList noms;
    for (const auto &e : std::as_const(m_equipments))
        if (!noms.contains(e.localisation, Qt::CaseInsensitive)) noms << e.localisation;
    m_extraRacks.erase(std::remove_if(m_extraRacks.begin(), m_extraRacks.end(),
        [&](const QString &r) { return noms.contains(r, Qt::CaseInsensitive); }), m_extraRacks.end());
    noms << m_extraRacks;
    std::sort(noms.begin(), noms.end(), [](const QString &a, const QString &b) {
        return a.compare(b, Qt::CaseInsensitive) < 0; });

    // 2) Disposition en grille
    qreal x = 0, y = 0, hLigne = 0;
    int col = 0;
    for (const QString &nom : std::as_const(noms)) {
        QList<Equipment> contenu;
        for (const auto &e : std::as_const(m_equipments))
            if (e.localisation.compare(nom, Qt::CaseInsensitive) == 0) contenu << e;

        const int lignes = std::max(1, int((contenu.size() + PAR_LIGNE - 1) / PAR_LIGNE));
        const qreal h = HEADER_H + PAD + lignes * TILE_H + (lignes - 1) * GAP + PAD;

        if (col == m_colonnes) { col = 0; x = 0; y += hLigne + RACK_GAP; hLigne = 0; }

        Rack r;
        r.nom = nom;
        r.rect = QRectF(x, y, RACK_W, h);
        r.item = m_scene->addRect(r.rect, QPen(QColor(0x1B, 0x4A, 0x5F), 1.5), QBrush(QColor(0x0A, 0x1F, 0x2D)));
        r.item->setZValue(0);
        m_racks << r;

        auto *titre = m_scene->addSimpleText(QStringLiteral("🗄  ") + nom);
        QFont f = titre->font(); f.setBold(true); f.setPointSize(10);
        titre->setFont(f);
        titre->setBrush(Theme::accentColor());
        titre->setPos(x + PAD, y + 8);
        titre->setZValue(1);

        auto *compteur = m_scene->addSimpleText(QString::number(contenu.size()));
        compteur->setBrush(Theme::mutedColor());
        compteur->setPos(x + RACK_W - PAD - compteur->boundingRect().width(), y + 9);
        compteur->setZValue(1);

        if (contenu.isEmpty()) {
            auto *vide = m_scene->addSimpleText(QStringLiteral("Étagère vide — déposez un équipement ici"));
            vide->setBrush(Theme::mutedColor());
            vide->setPos(x + PAD, y + HEADER_H + PAD + 22);
            vide->setZValue(1);
        }

        for (int i = 0; i < contenu.size(); ++i) {
            auto *tile = new EquipmentTile(contenu[i], contenu[i].id == m_selected, this);
            const qreal tx = x + PAD + (i % PAR_LIGNE) * (TILE_W + GAP);
            const qreal ty = y + HEADER_H + PAD + (i / PAR_LIGNE) * (TILE_H + GAP);
            m_scene->addItem(tile);
            tile->setHome(QPointF(tx, ty));
            tile->setZValue(2);

            if (m_enRetard.contains(contenu[i].id)) {
                auto *badge = m_scene->addSimpleText(QStringLiteral("⚠"));
                badge->setBrush(QColor(0xE4, 0x97, 0x40));
                badge->setPos(tx + TILE_W - 18, ty + 2);
                badge->setZValue(3);
                badge->setToolTip(QStringLiteral("Maintenance en retard (> 180 jours)"));
                m_alertBadges << badge;
            }
        }

        hLigne = std::max(hLigne, h);
        x += RACK_W + RACK_GAP;
        ++col;
    }
    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-10, -10, 10, 10));
}

int StoragePlanWidget::rackIndexAt(const QPointF &p) const
{
    for (int i = 0; i < m_racks.size(); ++i)
        if (m_racks[i].rect.contains(p)) return i;
    return -1;
}

void StoragePlanWidget::highlightRack(int index)
{
    for (int i = 0; i < m_racks.size(); ++i) {
        const bool on = (i == index);
        m_racks[i].item->setPen(QPen(on ? Theme::accentColor() : QColor(0x1B, 0x4A, 0x5F), on ? 3 : 1.5));
        m_racks[i].item->setBrush(on ? QColor(0x0E, 0x35, 0x44) : QColor(0x0A, 0x1F, 0x2D));
    }
}

void StoragePlanWidget::tileDragging(const QPointF &scenePos)
{
    highlightRack(rackIndexAt(scenePos));
}

void StoragePlanWidget::tileDropped(int id, const QPointF &scenePos)
{
    const int idx = rackIndexAt(scenePos);
    QString cible;
    if (idx >= 0) cible = m_racks[idx].nom;

    QString actuelle;
    for (const auto &e : std::as_const(m_equipments)) if (e.id == id) { actuelle = e.localisation; break; }

    // On ne reconstruit pas dans le gestionnaire d'événement de la tuile elle-même
    // (elle serait supprimée pendant qu'elle s'exécute) : on attend la boucle suivante.
    QTimer::singleShot(0, this, [this, id, cible, actuelle] {
        if (!cible.isEmpty() && cible.compare(actuelle, Qt::CaseInsensitive) != 0)
            emit equipmentMoved(id, cible);       // la page met à jour Oracle puis rafraîchit
        rebuild();                                // remet tout en place (succès ou échec)
    });
}

void StoragePlanWidget::tileClicked(int id)
{
    m_selected = id;
    emit equipmentSelected(id);
    QTimer::singleShot(0, this, [this] { rebuild(); });
}
