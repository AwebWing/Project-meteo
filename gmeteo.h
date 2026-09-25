#ifndef GMETEO_H
#define GMETEO_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class Gmeteo;
}
QT_END_NAMESPACE

class Gmeteo : public QMainWindow
{
    Q_OBJECT

public:
    explicit Gmeteo(QWidget *parent = nullptr);
    ~Gmeteo() override;

private:
    Ui::Gmeteo *ui;
};
#endif // GMETEO_H
