#include "mainwindow.h"
#include "equipmentpage.h"
#include "equipmentrepository.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("SmartWeather - Gestion des Équipements"));
    resize(1300, 850);
    setMinimumSize(1100, 700);

    m_repo = new EquipmentRepository(this);
    m_page = new EquipmentPage(m_repo, this);

    setCentralWidget(m_page);
}
