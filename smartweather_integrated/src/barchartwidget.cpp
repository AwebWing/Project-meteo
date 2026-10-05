#include "barchartwidget.h"

#include <QFontMetrics>
#include <QPainter>
#include <algorithm>

BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(200);
}

void BarChartWidget::setData(const Counts &data, const QString &title)
{
    m_data = data;
    m_title = title;
    setMinimumHeight(60 + int(m_data.size()) * 30);
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    p.setFont(titleFont);
    p.setPen(QColor("#162B3E"));
    p.drawText(QRect(12, 8, width() - 24, 26), Qt::AlignLeft | Qt::AlignVCenter, m_title);

    p.setFont(font());
    if (m_data.isEmpty()) {
        p.drawText(rect(), Qt::AlignCenter, tr("Aucune donnée pour cette période"));
        return;
    }

    int maxValue = 1;
    for (const auto &d : std::as_const(m_data))
        maxValue = std::max(maxValue, d.second);

    const QFontMetrics fm(font());
    const int top = 44, left = 180, right = 60, rowH = 30, barH = 22;
    const int avail = std::max(20, width() - left - right);

    for (int i = 0; i < m_data.size(); ++i) {
        const int y = top + i * rowH;
        const QRect label(10, y, left - 20, barH);
        p.setPen(QColor("#162B3E"));
        p.drawText(label, Qt::AlignRight | Qt::AlignVCenter,
                   fm.elidedText(m_data[i].first, Qt::ElideRight, label.width()));

        const int w = std::max(3, int(double(avail) * m_data[i].second / maxValue));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#347EA7"));
        p.drawRoundedRect(left, y, w, barH, 3, 3);

        p.setPen(QColor("#162B3E"));
        p.drawText(QRect(left + w + 6, y, right, barH), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(m_data[i].second));
    }
}
