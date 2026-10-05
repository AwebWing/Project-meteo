#include "database.h"
#include "mainwindow.h"
#include "theme.h"

#include <QApplication>
#include <QDir>
#include <QFont>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SmartWeather"));
    app.setOrganizationName(QStringLiteral("SmartWeather"));
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(Theme::styleSheet());

    // Connect DB for Interventions & core modules
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);

    QString dbErr;
    if (!Database::open(dir + QStringLiteral("/smartweather.db"), &dbErr)) {
        QMessageBox::warning(nullptr, QStringLiteral("Base de données"),
                             QStringLiteral("Connexion SQLite de secours en mode dégradé :\n%1").arg(dbErr));
    }

    MainWindow w;
    w.show();

    const int rc = app.exec();
    Database::close();
    return rc;
}
