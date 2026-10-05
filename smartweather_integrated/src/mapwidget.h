#pragma once
#include <QList>
#include <QWidget>
#include "zone.h"

// Mini-carte dessinee a la main (pas de tuiles / pas d'internet) :
// chaque zone est un marqueur place selon sa latitude / longitude,
// colore selon son niveau de vulnerabilite. Clic sur un marqueur = selection.
class MapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = nullptr);
    void setZones(const QList<Zone> &zones);
    void setSelected(int idZone);

signals:
    void zoneClicked(int idZone);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QPointF project(const Zone &z) const;

    QList<Zone> m_zones;
    int m_selected = -1;
};
