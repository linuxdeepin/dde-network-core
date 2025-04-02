cmake_minimum_required(VERSION 3.7)

set(PLUGIN_NAME "dss-network-plugin")

project(${PLUGIN_NAME})

include(GNUInstallDirs)
# 启用 qt moc 的支持
set(CMAKE_AUTOMOC ON)
# 启用 qrc 资源文件的支持
set(CMAKE_AUTORCC ON)
set(NET_USE_NOTIFICATION ON)

file(GLOB_RECURSE SRCS "networkmodule.h" "networkmodule.cpp"  "../net-view/*.h" "../net-view/*.cpp")

find_package(Qt6 COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(PkgConfig REQUIRED)
# find_package(Qt5Svg REQUIRED)
find_package(Dtk6 COMPONENTS Widget REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)
find_package(DdeSessionShell REQUIRED)

# if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -O0")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -O0")
# endif()


file(GLOB TS_FILES "translations/*.ts")
qt_add_translation(QM_FILES ${TS_FILES})
add_custom_target(${TARGET_NAME}_language ALL DEPENDS ${QM_FILES})

add_definitions("${QT_DEFINITIONS} -DQT_PLUGIN")

if (NET_USE_NOTIFICATION)
    add_definitions(-DNET_USE_NOTIFICATION)
    file(GLOB_RECURSE NOTIFICATION_SRCS "notification/*.h" "notification/*.cpp")
    set(SRCS ${SRCS} ${NOTIFICATION_SRCS})
endif()

add_library(${PLUGIN_NAME} SHARED ${SRCS} ../net-view/window/qrc/network.qrc)

target_include_directories(${PLUGIN_NAME} PUBLIC
    Dtk6::Widget
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
    ${DDESESSIONSHELL_INCLUDE_DIR}
    KF6::NetworkManagerQt
    "../net-view/operation"
    "../net-view/window"
    "../src"
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    Dtk6::Widget
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
    dde-network-core6
    KF6::NetworkManagerQt
)

install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-session-shell/modules)
# 安装 .qm 文件
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PLUGIN_NAME}/translations)

install(FILES 10-network-manager.pkla DESTINATION ${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/polkit-1/localauthority/10-vendor.d)
