#include "gmeteo.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    gmeteo w;
    w.show();
    return a.exec();
}