cmake_minimum_required(VERSION 3.7)

project(dss_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH $ENV{Qt5_DIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(DdeControlCenter REQUIRED)
find_package(Qt6 COMPONENTS Core DBus Widgets Network REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)
find_package(Dtk6 COMPONENTS Widget REQUIRED)
find_package(DdeSessionShell REQUIRED)
find_package(PkgConfig REQUIRED)

aux_source_directory(../dss-network-plugin DSSPLUGINTHEMES)
aux_source_directory(../dss-network-plugin/notification DSSPLUGINNOTIFICATION)
aux_source_directory(../common-plugin COMMONPLUGINS)
aux_source_directory(../common-plugin/widgets COMMONPLUGINWIDGET)
aux_source_directory(../common-plugin/item COMMONPLUGINITEM)
aux_source_directory(. COMPILEFILES)

#set(COMPILEFILES ./*.h ./*.cpp)
file(GLOB_RECURSE COMMONPLUGINS_SRCS
    ../common-plugin/*
)
add_executable(${PROJECT_NAME}
    ${COMPILEFILES}
    ${NETWORKDIR}
    ${DSSPLUGINTHEMES}
    ${DSSPLUGINNOTIFICATION}
    ${COMMONPLUGINS_SRCS}
    ${REALIZEDIR})

target_include_directories(${PROJECT_NAME} PUBLIC
    ../dss-network-plugin
    ../common-plugin
    ../common-plugin/widgets
    ../common-plugin/item
    ../dss-network-plugin/interface
    ../src
    ${DdeControlCenter_INCLUDE_DIR}
    ${DDE_DOCK_INCLUDE_DIR}
    Dtk6::Widget
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
    KF6::NetworkManagerQt
    ${DDE-Network-Core_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DdeControlCenter_LIBRARIES}
    ${DDE_DOCK_LIBRARIES}
    Dtk6::Widget
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
    KF6::NetworkManagerQt
    dde-network-core6
)

add_custom_command(
    OUTPUT path.pb.cc path.pb.h
    COMMAND protoc
    ARGS --cpp_out . ${CMAKE_CURRENT_SOURCE_DIR}/path.proto -I ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS path.proto)
