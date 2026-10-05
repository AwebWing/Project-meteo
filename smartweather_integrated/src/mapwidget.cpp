#include "mapwidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include "theme.h"
#include "zoneservice.h"

MapWidget::MapWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(320, 220);
    setCursor(Qt::PointingHandCursor);
}

void MapWidget::setZones(const QList<Zone> &zones) { m_zones = zones; update(); }
void MapWidget::setSelected(int idZone)            { m_selected = idZone; update(); }

QPointF MapWidget::project(const Zone &z) const
{
    double minLat = 1e9, maxLat = -1e9, minLon = 1e9, maxLon = -1e9;
    for (const Zone &o : m_zones) {
        minLat = std::min(minLat, o.latitude);  maxLat = std::max(maxLat, o.latitude);
        minLon = std::min(minLon, o.longitude); maxLon = std::max(maxLon, o.longitude);
    }
    double dLat = std::max(maxLat - minLat, 0.2);
    double dLon = std::max(maxLon - minLon, 0.2);
    minLat -= dLat * 0.15; maxLat += dLat * 0.15; dLat = maxLat - minLat;
    minLon -= dLon * 0.15; maxLon += dLon * 0.15; dLon = maxLon - minLon;

    const QRectF area = QRectF(rect()).adjusted(28, 34, -86, -34);
    const double x = area.left()   + (z.longitude - minLon) / dLon * area.width();
    const double y = area.bottom() - (z.latitude  - minLat) / dLat * area.height();
    return QPointF(x, y);
}

void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = QRectF(rect()).adjusted(1, 1, -1, -1);
    QPainterPath clip;
    clip.addRoundedRect(r, 12, 12);
    p.setClipPath(clip);

    QLinearGradient g(r.topLeft(), r.bottomRight());
    g.setColorAt(0, QColor(12, 44, 60));
    g.setColorAt(1, QColor(6, 22, 32));
    p.fillPath(clip, g);

    // grille
    p.setPen(QPen(QColor(255, 255, 255, 16), 1));
    for (int x = 0; x < width(); x += 36)  p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += 36) p.drawLine(0, y, width(), y);

    // marqueurs
    for (const Zone &z : m_zones) {
        const QPointF pt = project(z);
        const QColor c = Theme::levelColor(ZoneService::niveau(z.vulnerabilite));
        const bool inactive = z.statut != QLatin1String("Active");

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(c.red(), c.green(), c.blue(), inactive ? 25 : 55));
        p.drawEllipse(pt, 17, 17);
        p.setBrush(inactive ? QColor(110, 130, 140) : c);
        p.drawEllipse(pt, 8, 8);

        if (z.id == m_selected) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(Qt::white, 2));
            p.drawEllipse(pt, 14, 14);
        }
        p.setPen(Theme::textColor());
        p.drawText(QPointF(pt.x() + 15, pt.y() + 4), z.nom);
    }

    // titre + legende
    p.setPen(Theme::mutedColor());
    p.drawText(QPointF(14, 22), QStringLiteral("Carte des zones"));
    const QStringList levels = {QStringLiteral("Faible"), QStringLiteral("Moyen"), QStringLiteral("Élevé")};
    int lx = 14;
    for (const QString &l : levels) {
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::levelColor(l));
        p.drawEllipse(QPointF(lx + 5, height() - 16), 5, 5);
        p.setPen(Theme::mutedColor());
        p.drawText(QPointF(lx + 15, height() - 12), l);
        lx += 80;
    }

    p.setClipping(false);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0x17, 0x47, 0x5a), 1));
    p.drawRoundedRect(r, 12, 12);
}

void MapWidget::mousePressEvent(QMouseEvent *e)
{
    int best = -1;
    double bestDist = 20.0;   // rayon de clic en pixels
    for (const Zone &z : m_zones) {
        const QPointF d = project(z) - e->position();
        const double dist = std::hypot(d.x(), d.y());
        if (dist < bestDist) { bestDist = dist; best = z.id; }
    }
    if (best != -1) emit zoneClicked(best);
}
