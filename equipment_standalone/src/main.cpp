#include <QApplication>
#include <QFont>
#include "mainwindow.h"
#include "theme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(Theme::styleSheet());

    MainWindow w;
    w.show();
    return app.exec();
}
