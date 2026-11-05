#include "mainwindow.h"
#include "theme.h"
#include "connection.h"

#include <QApplication>
#include <QDebug>
#include <QFont>
#include <QMessageBox>
#include <QSqlQuery>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SmartWeather"));
    app.setOrganizationName(QStringLiteral("SmartWeather"));
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(Theme::styleSheet());

    // --- Connexion Oracle (Singleton) : AVANT de creer la fenetre principale,
    //     car les modules lisent la base des leur construction.
    Connection &c = Connection::createInstance();
    if (!c.createConnection()) {
        QMessageBox::critical(nullptr, QStringLiteral("Erreur de base de données"),
            QStringLiteral("Connexion à la base Oracle impossible.\n\n%1\n\n"
                           "Vérifiez la source ODBC « smartweatherDSN », l'utilisateur et le mot de passe.")
                .arg(c.lastError()));
    } else {
        qDebug() << ">>> DATABASE CONNECTION SUCCESSFUL <<<";
        QSqlQuery q("SELECT user FROM dual");
        if (q.next()) {
            qDebug() << ">>> CURRENT ORACLE USER:" << q.value(0).toString() << "<<<";
        }
    }

    MainWindow w;
    w.show();

    return app.exec();
}
