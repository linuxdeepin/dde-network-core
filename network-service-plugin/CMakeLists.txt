# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.13)

set(BIN_NAME "network-service")

project(${BIN_NAME})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)
file(GLOB_RECURSE SRCS "*.h" "*.cpp")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-DQM_FILES_DIR="${CMAKE_BINARY_DIR}/network-service-plugin")
else()
    ADD_DEFINITIONS(-DQM_FILES_DIR="/usr/share/deepin-service-manager/network-service/translations")
endif()
set(QT_NS 6)
set(DTK_NS Dtk6)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(CMAKE_PREFER_PTHREAD_FLAG ON)

find_package(Qt${QT_NS} COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(KF${QT_NS}NetworkManagerQt REQUIRED)
find_package(${DTK_NS} COMPONENTS Core REQUIRED)

file(GLOB TS_FILES "translations/*.ts")
qt_add_translation(QM_FILES ${TS_FILES})
add_custom_target(${BIN_NAME}_language ALL DEPENDS ${QM_FILES})

pkg_check_modules(LibNM REQUIRED IMPORTED_TARGET libnm)

add_library(${BIN_NAME} MODULE
    ${SRCS}
)

target_include_directories(${BIN_NAME} PUBLIC
  Qt${QT_NS}::Core
  Qt${QT_NS}::DBus
  Qt${QT_NS}::Network
  KF${QT_NS}::NetworkManagerQt
  ${DTK_NS}::Core
  .
  system
  session
  utils
  accountnetwork/system
  accountnetwork/system/accountnetwork
  accountnetwork/session/
  accountnetwork/session/accountnetwork
)

target_link_libraries(${BIN_NAME} PRIVATE
  Qt${QT_NS}::Core
  Qt${QT_NS}::DBus
  Qt${QT_NS}::Network
  KF${QT_NS}::NetworkManagerQt
  ${DTK_NS}::Core
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    EXECUTE_PROCESS(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/share/system/)
    EXECUTE_PROCESS(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/share/user/)
    EXECUTE_PROCESS(COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/system/plugin-system-network.json ${CMAKE_BINARY_DIR}/share/system/)
    EXECUTE_PROCESS(COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/session/plugin-session-network.json ${CMAKE_BINARY_DIR}/share/user/)
else()
    # just need this in your project
    install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR}/deepin-service-manager/)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/system/plugin-system-network.json DESTINATION share/deepin-service-manager/system/)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/session/plugin-session-network.json DESTINATION share/deepin-service-manager/user/)
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/system/org.deepin.service.SystemNetwork.conf DESTINATION share/dbus-1/system.d/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/session/org.deepin.service.SessionNetwork.conf DESTINATION share/dbus-1/system.d/)
# 安装.qm文件
install(FILES ${QM_FILES} DESTINATION share/deepin-service-manager/network-service/translations/)
