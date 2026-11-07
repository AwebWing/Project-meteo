QT       += core gui widgets charts sql

TARGET   = SmartWeather
TEMPLATE = app

CONFIG   += c++17

# Folder structure according to ESPRIT guidelines
INCLUDEPATH += Headers

# Source files
SOURCES += \
    Sources/accueilpage.cpp \
    Sources/barchartwidget.cpp \
    Sources/connection.cpp \
    Sources/coveragepage.cpp \
    Sources/emploipage.cpp \
    Sources/equipment.cpp \
    Sources/equipmentdialog.cpp \
    Sources/equipmentpage.cpp \
    Sources/equipmentrepository.cpp \
    Sources/equipmentservice.cpp \
    Sources/fournisseur.cpp \
    Sources/fournisseurrepository.cpp \
    Sources/interventiondialog.cpp \
    Sources/interventionservice.cpp \
    Sources/interventionwidget.cpp \
    Sources/main.cpp \
    Sources/mainwindow.cpp \
    Sources/mapwidget.cpp \
    Sources/smartsupply.cpp \
    Sources/storageplanwidget.cpp \
    Sources/zone.cpp \
    Sources/zonedetailsdialog.cpp \
    Sources/zonedialog.cpp \
    Sources/zonerepository.cpp \
    Sources/zoneservice.cpp \
    Sources/zonesmodule.cpp \
    Sources/zonespage.cpp

# Header files
HEADERS += \
    Headers/accueilpage.h \
    Headers/barchartwidget.h \
    Headers/connection.h \
    Headers/coveragepage.h \
    Headers/emploipage.h \
    Headers/equipment.h \
    Headers/equipmentdialog.h \
    Headers/equipmentpage.h \
    Headers/equipmentrepository.h \
    Headers/equipmentservice.h \
    Headers/fournisseur.h \
    Headers/fournisseurrepository.h \
    Headers/intervention.h \
    Headers/inputfilters.h \
    Headers/interventiondialog.h \
    Headers/interventionservice.h \
    Headers/interventionwidget.h \
    Headers/mainwindow.h \
    Headers/mapwidget.h \
    Headers/smartsupply.h \
    Headers/storageplanwidget.h \
    Headers/theme.h \
    Headers/uihelpers.h \
    Headers/zone.h \
    Headers/zonedetailsdialog.h \
    Headers/zonedialog.h \
    Headers/zonerepository.h \
    Headers/zoneservice.h \
    Headers/zonesmodule.h \
    Headers/zonespage.h

# UI files
FORMS += \
    Forms/interventiondialog.ui \
    Forms/mainwindow.ui

# Resources
RESOURCES += resources/resources.qrc
