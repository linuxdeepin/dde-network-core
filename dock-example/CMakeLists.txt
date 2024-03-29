cmake_minimum_required(VERSION 3.7)

project(dock_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH $ENV{Qt5_DIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core DBus Network REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(KF5NetworkManagerQt REQUIRED)
find_package(DtkCore REQUIRED)
find_package(DtkWidget REQUIRED)

set(DTKCORE_INCLUDE_DIR DTKCORE_INCLUDE_DIRS)

find_package(DdeDock REQUIRED)
find_package(PkgConfig REQUIRED)

aux_source_directory(../dock-network-plugin DOCKPLUGINDIR)
aux_source_directory(../common-plugin/* COMMONPLUGINS)

aux_source_directory(. COMPILEFILES)
file(GLOB_RECURSE QRCFILES "../common-plugin/network.qrc"
    "../common-plugin/resources/common/*.svg"
    "../common-plugin/resources/wired/*.svg"
    "../common-plugin/resources/wireless/*.svg"
    "../common-plugin/resources/wireless6/*.svg")

get_target_property(KF5_QT_INCLUDE_DIRS KF5::NetworkManagerQt INTERFACE_INCLUDE_DIRECTORIES)

file(GLOB_RECURSE COMMONPLUGINS_SRCS
    ../common-plugin/*
    ../dss-network-plugin/dockpopupwindow.h
    ../dss-network-plugin/dockpopupwindow.cpp
)
add_executable(${PROJECT_NAME}
    ${COMPILEFILES}
    ${DOCKPLUGINDIR}
    ${COMMONPLUGINS_SRCS}
    ${QRCFILES})

get_target_property(KF5_QT_INCLUDE_DIRS KF5::NetworkManagerQt INTERFACE_INCLUDE_DIRECTORIES)

foreach(LETTER ${TRY_KF5_LIBRARIES})
    if (NOT KF5_QT_LIBRARIES)
        get_target_property(KF5_QT_LIBRARIES KF5::NetworkManagerQt ${LETTER})
    endif()
endforeach()

if (NOT KF5_QT_LIBRARIES)
    find_library(KF5_QT_LIBRARIES libKF5NetworkManagerQt.so)
endif()

set(TRY_KF5_LIBRARIES
IMPORTED_LOCATION_DEBIAN
IMPORTED_LOCATION_NOCONFIG
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ../dock-network-plugin
    ../common-plugin
    ../common-plugin/widgets
    ../common-plugin/item
    ../common-plugin/resources/common/
    ../common-plugin/resources/wired/
    ../common-plugin/resources/wireless/
    ../dss-network-plugin
    ${DdeControlCenter_INCLUDE_DIR}
    ${DDE_DOCK_INCLUDE_DIR}
    ${DtkWidget_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5DBus_INCLUDE_DIRS}
    ${Qt5Widget_INCLUDE_DIRS}
    ${QGSettings_INCLUDE_DIRS}
    #${LibNM_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${KF5_QT_INCLUDE_DIRS}
    ${DDE-Network-Core_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DDE_DOCK_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    #${LibNM_LIBRARIES}
    ${Qt5Svg_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${QGSettings_LIBRARIES}
    ${KF5_QT_LIBRARIES}
    ${DDE-Network-Core_LIBRARIES}
)

add_custom_command(
    OUTPUT path.pb.cc path.pb.h
    COMMAND protoc
    ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/path.proto -I ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS path.proto)
