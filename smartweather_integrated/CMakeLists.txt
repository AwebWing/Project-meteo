cmake_minimum_required(VERSION 3.16)
project(SmartWeather_Integrated VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Sql PrintSupport)

add_executable(SmartWeather_Integrated WIN32
    src/main.cpp
    src/theme.h
    src/uihelpers.h
    src/mainwindow.h
    src/mainwindow.cpp
    src/accueilpage.h
    src/accueilpage.cpp
    # Module Emploi (Yasmine Kouki)
    src/emploipage.h
    src/emploipage.cpp
    # Module Zones (Adem Nouioui)
    src/zone.h
    src/zoneservice.h
    src/zoneservice.cpp
    src/zonerepository.h
    src/zonerepository.cpp
    src/zonesmodule.h
    src/zonesmodule.cpp
    src/zonespage.h
    src/zonespage.cpp
    src/zonedialog.h
    src/zonedialog.cpp
    src/zonedetailsdialog.h
    src/zonedetailsdialog.cpp
    src/coveragepage.h
    src/coveragepage.cpp
    src/mapwidget.h
    src/mapwidget.cpp
    # Module Interventions (Bader Eddine)
    src/intervention.h
    src/interventionservice.h
    src/interventionservice.cpp
    src/interventiondialog.h
    src/interventiondialog.cpp
    src/interventionwidget.h
    src/interventionwidget.cpp
    src/barchartwidget.h
    src/barchartwidget.cpp
    src/database.h
    src/database.cpp
    # Module Équipements (Aweb Ghandour)
    src/equipment.h
    src/equipmentservice.h
    src/equipmentservice.cpp
    src/equipmentrepository.h
    src/equipmentrepository.cpp
    src/equipmentpage.h
    src/equipmentpage.cpp
    src/equipmentdialog.h
    src/equipmentdialog.cpp
    # Resources
    resources/resources.qrc
)

target_link_libraries(SmartWeather_Integrated PRIVATE Qt6::Widgets Qt6::Charts Qt6::Sql Qt6::PrintSupport)
