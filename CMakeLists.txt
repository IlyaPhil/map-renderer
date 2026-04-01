cmake_minimum_required(VERSION 3.14)
project(MapRenderer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSYS2 MinGW64
list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")

set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(GDAL REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MapWidget.cpp
    src/S57Loader.cpp
)

add_executable(MapRenderer ${SOURCES})

target_link_libraries(MapRenderer
    Qt5::Widgets
    GDAL::GDAL
)