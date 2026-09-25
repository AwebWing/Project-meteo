#include "gmeteo.h"
#include "ui_gmeteo.h"

Gmeteo::Gmeteo(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Gmeteo)
{
    ui->setupUi(this);
}

Gmeteo::~Gmeteo()
{
    delete ui;
}
