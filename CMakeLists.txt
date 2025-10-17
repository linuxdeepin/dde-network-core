cmake_minimum_required(VERSION 3.7)

set(VERSION "2.0.18" CACHE STRING "define project version")
project(dde-network-core
  VERSION ${VERSION}
  DESCRIPTION "DDE Network Core"
  HOMEPAGE_URL "https://github.com/linuxdeepin/dde-network-core"
  LANGUAGES CXX C
)

option(ENABLE_DEEPIN_NMQT "enable nmqt patch on deepin" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

set(DDE-Network-Core_INCLUDE_DIRS  "${CMAKE_SOURCE_DIR}/src-old")
set(DDE-Network-Core_LIBRARIES  dde-network-core)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LibNM REQUIRED IMPORTED_TARGET libnm)
include_directories(${LibNM_INCLUDE_DIRS})

pkg_check_modules(Gio REQUIRED gio-2.0)
include_directories(${Gio_INCLUDE_DIRS})

pkg_search_module(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})

if (QT_DEBUG)
    message(" KF5_QT_INCLUDE_DIRS : " ${KF5_QT_INCLUDE_DIRS})
    message(" GLIB_INCLUDE_DIRS :" ${GLIB_INCLUDE_DIRS})
    message(" CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR})
    message(STATUS ${result})
endif()

if (ENABLE_DEEPIN_NMQT)
    add_definitions(-DUSE_DEEPIN_NMQT)
endif()

message(STATUS DEEPIN_NMQT_TEST ": ${DEEPIN_NMQT_TEST}")

set(INTERFACE_VERSION "2.0")
add_definitions(-DINTERFACE_VERSION="${INTERFACE_VERSION}")
message("Interface Version: " ${INTERFACE_VERSION})
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()
include(GNUInstallDirs)

add_subdirectory("src")
# add_subdirectory("src-old")
# add_subdirectory("dcc-network-plugin")
add_subdirectory("dcc-network")
if (BUILD_TESTS)
    add_subdirectory("tests")
endif()
# add_subdirectory("example")
add_subdirectory("dock-network-plugin")
add_subdirectory("dss-network-plugin")
# add_subdirectory("dss_example")
#add_subdirectory("dock-example")
add_subdirectory("network-service-plugin")
