cmake_minimum_required(VERSION 3.16)
project(SmartWeather_Equipment VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

add_executable(SmartWeather_Equipment WIN32
    src/main.cpp
    src/theme.h
    src/uihelpers.h
    src/equipment.h
    src/equipmentservice.h
    src/equipmentservice.cpp
    src/equipmentrepository.h
    src/equipmentrepository.cpp
    src/equipmentpage.h
    src/equipmentpage.cpp
    src/equipmentdialog.h
    src/equipmentdialog.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    resources/resources.qrc
)

target_link_libraries(SmartWeather_Equipment PRIVATE Qt6::Widgets Qt6::Charts)
