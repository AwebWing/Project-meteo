#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SmartWeather"));
    app.setOrganizationName(QStringLiteral("SmartWeather"));
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(Theme::styleSheet());

    MainWindow w;
    w.show();

    return app.exec();
}
