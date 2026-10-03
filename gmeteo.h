#ifndef GMETEO_H
#define GMETEO_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class gmeteo; }
QT_END_NAMESPACE

class gmeteo : public QMainWindow
{
    Q_OBJECT

public:
    gmeteo(QWidget *parent = nullptr);
    ~gmeteo();

private slots:
    void on_btnLogin_clicked();
    void on_btnNavStations_clicked();
    void on_btnBackFromStations_clicked();
    void on_btnLogout_clicked();

private:
    Ui::gmeteo *ui;
};

#endif // GMETEO_H
