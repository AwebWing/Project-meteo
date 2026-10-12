#pragma once
#include "interventionservice.h"
#include <QWidget>

// Histogramme horizontal simple (aucune dépendance à Qt Charts).
class BarChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    void setData(const Counts &data, const QString &title);
    QSize sizeHint() const override { return {600, 300}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Counts m_data;
    QString m_title;
};
