#pragma once
#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

// Petit histogramme dessiné à la main (aucune dépendance à Qt Charts).
class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(QWidget *parent = nullptr);
    void setData(const QString &title, const QList<QPair<QString, int>> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString m_title;
    QList<QPair<QString, int>> m_data;
};
