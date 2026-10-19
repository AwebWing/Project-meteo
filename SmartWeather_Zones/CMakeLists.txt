cmake_minimum_required(VERSION 3.16)
project(SmartWeather_Zones VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Sql)

add_executable(SmartWeather_Zones WIN32
    src/main.cpp
    src/connection.h
    src/connection.cpp
    src/theme.h
    src/uihelpers.h
    src/zone.h
    src/zoneservice.h
    src/zoneservice.cpp
    src/zonerepository.h
    src/zonerepository.cpp
    src/mapwidget.h
    src/mapwidget.cpp
    src/zonedialog.h
    src/zonedialog.cpp
    src/zonedetailsdialog.h
    src/zonedetailsdialog.cpp
    src/zonespage.h
    src/zonespage.cpp
    src/coveragepage.h
    src/coveragepage.cpp
    src/zonesmodule.h
    src/zonesmodule.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    resources/resources.qrc
)

target_link_libraries(SmartWeather_Zones PRIVATE Qt6::Widgets Qt6::Charts Qt6::Sql)