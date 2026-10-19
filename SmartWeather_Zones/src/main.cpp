#include <QApplication>
#include <QFont>
#include <QMessageBox>
#include "mainwindow.h"
#include "theme.h"
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(Theme::styleSheet());

    Connection &c = Connection::createInstance();

    if (c.createConnection()) {
        QMessageBox::information(nullptr, QObject::tr("Base de données"),
                                 QObject::tr("Connexion réussie à la base Oracle."),
                                 QMessageBox::Ok);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("Base de données"),
                              QObject::tr("Échec de la connexion.\n\n%1").arg(c.lastErrorText()),
                              QMessageBox::Cancel);
    }

    MainWindow w;
    w.show();
    return app.exec();
}