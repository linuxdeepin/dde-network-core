cmake_minimum_required(VERSION 3.7)

set(PLUGIN_NAME "dock-hotspot-plugin")

project(${PLUGIN_NAME})
include(GNUInstallDirs)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SRCS "*.h" "*.cpp" "../common-plugin/widgets/*" "../common-plugin/plugin_global.g")

find_package(Qt5 COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(DtkCore REQUIRED)
find_package(DtkWidget REQUIRED)

find_package(DdeDock REQUIRED)
find_package(KF5NetworkManagerQt REQUIRED)

pkg_check_modules(DDE-Dock REQUIRED dde-dock)

file(GLOB TS_FILES "translations/*.ts")
qt5_add_translation(QM_FILES ${TS_FILES})
add_custom_target(HotSpotlanguage ALL DEPENDS ${QM_FILES})

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS})

get_target_property(KF5_QT_INCLUDE_DIRS KF5::NetworkManagerQt INTERFACE_INCLUDE_DIRECTORIES)

set(TRY_KF5_LIBRARIES
IMPORTED_LOCATION_DEBIAN
IMPORTED_LOCATION_NOCONFIG
)

if (NOT KF5_QT_LIBRARIES)
    find_library(KF5_QT_LIBRARIES libKF5NetworkManagerQt.so)
endif()

target_include_directories(${PLUGIN_NAME} PUBLIC
    ${NETWORKINTERFACE}
    ${DtkWidget_INCLUDE_DIRS}
    ${Qt5DBus_INCLUDE_DIRS}
    ${DDE_DOCK_INCLUDE_DIR}
    ${Qt5Network_INCLUDE_DIRS}
    ${KF5_QT_INCLUDE_DIRS}
    ${NETINTERFACEINCLUDE}
    "../common-plugin"
    "../src"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${DtkWidget_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Svg_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${KF5_QT_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${DDE-Network-Core_LIBRARIES}
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/plugins/quick-trays)
# 安装 .qm 文件
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PLUGIN_NAME}/translations)
