QT       += core gui widgets charts sql

TARGET   = SmartWeather
TEMPLATE = app

CONFIG   += c++17

# Folder structure according to ESPRIT guidelines
INCLUDEPATH += Headers

# Source files
SOURCES += \
    Sources/connection.cpp \
    Sources/main.cpp \
    Sources/zone.cpp \
    Sources/mainwindow.cpp \
    Sources/accueilpage.cpp \
    Sources/zonespage.cpp \
    Sources/zonesmodule.cpp \
    Sources/zoneservice.cpp \
    Sources/zonerepository.cpp \
    Sources/zonedialog.cpp \
    Sources/zonedetailsdialog.cpp \
    Sources/interventionswindow.cpp \
    Sources/interventionservice.cpp \
    Sources/interventionwidget.cpp \
    Sources/interventiondialog.cpp \
    Sources/emploipage.cpp \
    Sources/equipmentpage.cpp \
    Sources/equipmentdialog.cpp \
    Sources/equipmentservice.cpp \
    Sources/equipment.cpp \
    Sources/equipmentrepository.cpp \
    Sources/database.cpp \
    Sources/chartwidget.cpp \
    Sources/barchartwidget.cpp \
    Sources/mapwidget.cpp \
    Sources/coveragepage.cpp \
    Sources/intervention.cpp

# Header files
HEADERS += \
    Headers/connection.h \
    Headers/mainwindow.h \
    Headers/accueilpage.h \
    Headers/zonespage.h \
    Headers/zonesmodule.h \
    Headers/zoneservice.h \
    Headers/zonerepository.h \
    Headers/zonedialog.h \
    Headers/zonedetailsdialog.h \
    Headers/interventionswindow.h \
    Headers/interventionservice.h \
    Headers/interventionwidget.h \
    Headers/interventiondialog.h \
    Headers/emploipage.h \
    Headers/equipmentpage.h \
    Headers/equipmentdialog.h \
    Headers/equipmentservice.h \
    Headers/equipmentrepository.h \
    Headers/database.h \
    Headers/chartwidget.h \
    Headers/barchartwidget.h \
    Headers/mapwidget.h \
    Headers/coveragepage.h \
    Headers/intervention.h \
    Headers/equipment.h \
    Headers/theme.h \
    Headers/uihelpers.h \
    Headers/zone.h

# UI files
FORMS += \
    Forms/mainwindow.ui \
    Forms/interventionswindow.ui \
    Forms/interventiondialog.ui

# Resources
RESOURCES += resources/resources.qrc
