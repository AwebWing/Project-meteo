#include "chartwidget.h"

#include <QColor>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

ChartWidget::ChartWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(230);
}

void ChartWidget::setData(const QString &title, const QList<QPair<QString, int>> &data)
{
    m_title = title;
    m_data = data;
    update();
}

void ChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Carte sombre
    p.setPen(QColor("#162B3E"));
    p.setBrush(QColor("#0B1218"));
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);

    const QRectF r = QRectF(rect()).adjusted(16, 12, -16, -12);
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor("#E6EDF3"));
    p.drawText(QRectF(r.left(), r.top(), r.width(), 24), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    QFont f = font();
    f.setBold(false);
    f.setPointSize(f.pointSize() - 1);
    p.setFont(f);

    if (m_data.isEmpty()) {
        p.setPen(QColor("#9fb3c8"));
        p.drawText(r, Qt::AlignCenter, "Aucune donnée");
        return;
    }

    int maxV = 1;
    for (const auto &d : m_data) maxV = std::max(maxV, d.second);

    const QRectF plot(r.left() + 6, r.top() + 40, r.width() - 12, r.height() - 40 - 24);
    const int n = m_data.size();
    const qreal slot = plot.width() / n;
    const qreal bw = std::min<qreal>(slot * 0.55, 64);
    static const QColor palette[] = {QColor("#347EA7"), QColor("#409A71"), QColor("#E49740"),
                                     QColor("#CC4E4F"), QColor("#C6A555"), QColor("#7AA2F7")};

    // ligne de base
    p.setPen(QColor("#1d3a52"));
    p.drawLine(QPointF(plot.left(), plot.bottom()), QPointF(plot.right(), plot.bottom()));

    const QFontMetrics fm(f);
    for (int i = 0; i < n; ++i) {
        const qreal cx = plot.left() + slot * i + slot / 2;
        const qreal h = plot.height() * m_data[i].second / maxV;
        const QRectF bar(cx - bw / 2, plot.bottom() - h, bw, h);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i % 6]);
        if (m_data[i].second > 0) p.drawRoundedRect(bar, 4, 4);

        p.setPen(QColor("#E6EDF3"));
        p.drawText(QRectF(cx - slot / 2, bar.top() - 18, slot, 16), Qt::AlignCenter, QString::number(m_data[i].second));

        p.setPen(QColor("#9fb3c8"));
        p.drawText(QRectF(cx - slot / 2, plot.bottom() + 4, slot, 18), Qt::AlignCenter,
                   fm.elidedText(m_data[i].first, Qt::ElideRight, int(slot) - 4));
    }
}
