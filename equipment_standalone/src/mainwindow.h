#pragma once
#include <QMainWindow>

class EquipmentRepository;
class EquipmentPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    EquipmentRepository *m_repo{nullptr};
    EquipmentPage *m_page{nullptr};
};
