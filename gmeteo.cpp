#include "gmeteo.h"
#include "ui_gmeteo.h"

gmeteo::gmeteo(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::gmeteo)
{
    ui->setupUi(this);

    // Start on Login Page (Index 0)
    ui->stackedWidget->setCurrentIndex(0);
}

gmeteo::~gmeteo()
{
    delete ui;
}

// 1. Click Login -> Go to Main Menu (Index 1)
void gmeteo::on_btnLogin_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

// 2. Click Stations Button -> Go to Stations Page (Index 2)
void gmeteo::on_btnNavStations_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

// 3. Click "Retour au Menu" -> Back to Main Menu (Index 1)
void gmeteo::on_btnBackFromStations_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

// 4. Click Logout -> Back to Login (Index 0)
void gmeteo::on_btnLogout_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}