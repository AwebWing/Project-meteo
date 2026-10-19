#pragma once
#include <QMainWindow>

class QLabel;
class QStackedWidget;
class ZoneRepository;

// Fenetre principale : barre laterale + bandeau meteo + pages.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QWidget *buildSidebar();
    QWidget *buildHeader();
    QWidget *placeholder(const QString &title, const QString &owner);
    void updateClock();

    ZoneRepository *m_repo;
    QStackedWidget *m_stack;
    QLabel *m_date, *m_time;
};
