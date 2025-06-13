# SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

set(PLUGIN_NAME "dock-network-plugin")

set(QT_NS Qt6)
set(DTK_NS Dtk6)

project(${PLUGIN_NAME})
include(GNUInstallDirs)
# 启用 qt moc 的支持
set(CMAKE_AUTOMOC ON)
# 启用 qrc 资源文件的支持
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE SRCS
    "networkplugin.h"
    "networkplugin.cpp"
    "quickpanelwidget.h"
    "quickpanelwidget.cpp"
    "dockcontentwidget.h"
    "../net-view/*.h"
    "../net-view/*.cpp"
    "widget/jumpsettingbutton.h"
    "widget/jumpsettingbutton.cpp"
    "widget/commoniconbutton.h"
    "widget/commoniconbutton.cpp"
)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(CMAKE_PREFER_PTHREAD_FLAG ON)

find_package(${QT_NS} COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(${DTK_NS} REQUIRED COMPONENTS Widget)
find_package(KF6NetworkManagerQt REQUIRED)
find_package(DdeDock REQUIRED)

#if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -O0")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -O0")
#endif()

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")
add_library(${PLUGIN_NAME} SHARED ${SRCS} ../net-view/window/qrc/network.qrc)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ../../dde-dock)

file(GLOB TS_FILES "translations/*.ts")
qt_add_translations(${PLUGIN_NAME}_language
    TS_FILES ${TS_FILES}
    SOURCES ${SRCS}
    LUPDATE_OPTIONS -no-obsolete -no-ui-lines -locations none
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)
add_custom_target(${PLUGIN_NAME}_language ALL DEPENDS ${QM_FILES})
# get_target_property(KF6_QT_INCLUDE_DIRS KF6::NetworkManagerQt INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(${PLUGIN_NAME} PUBLIC
    ${DTK_NS}::Widget
    ${QT_NS}::Dbus
    ${QT_NS}::Network
    KF6::NetworkManagerQt
    # "../../../interfaces"
    "../net-view/operation"
    "../net-view/window"
    "../src"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${QT_NS}::Widgets
    ${QT_NS}::DBus
    ${QT_NS}::Network
    ${DTK_NS}::Widget
    KF6::NetworkManagerQt
    dde-network-core6
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/plugins/system-trays)
# 安装 .qm 文件
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PLUGIN_NAME}/translations)
# 安装显示在控制中心-个性化-任务栏-插件区域的图标
install(FILES "resources/dcc-network.dci" DESTINATION share/dde-dock/icons/dcc-setting)
