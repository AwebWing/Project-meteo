#include "gmeteo.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Gmeteo w;
    w.show();
    return QApplication::exec();
}
