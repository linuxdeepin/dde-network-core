cmake_minimum_required(VERSION 3.7)

project(dde-network-core)

# 增加安全编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
set(CMAKE_EXE_LINKER_FLAGS  "-z relro -z now -z noexecstack -pie")

set(DDE-Network-Core_INCLUDE_DIRS  "${CMAKE_SOURCE_DIR}/src")
set(DDE-Network-Core_LIBRARIES  dde-network-core)

find_package(KF5NetworkManagerQt REQUIRED)

set(TRY_KF5_LIBRARIES
IMPORTED_LOCATION_DEBIAN
IMPORTED_LOCATION_NOCONFIG
)

if (NOT KF5_QT_LIBRARIES)
    find_library(KF5_QT_LIBRARIES libKF5NetworkManagerQt.so)
endif()

find_package(Qt5 COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)
find_package(KF5NetworkManagerQt REQUIRED)
get_target_property(KF5_QT_INCLUDE_DIRS KF5::NetworkManagerQt INTERFACE_INCLUDE_DIRECTORIES)

# test networkmanagetqt
try_compile(DEEPIN_NMQT_TEST
              ${CMAKE_CURRENT_BINARY_DIR}/try_compile
            SOURCES
              ${CMAKE_CURRENT_SOURCE_DIR}/try-compile/networkmanagerqt.cc
            LINK_LIBRARIES
              ${KF5_QT_LIBRARIES}
              Qt5::Core
              Qt5::DBus
              Qt5::Network
            CMAKE_FLAGS
              "-DINCLUDE_DIRECTORIES=${KF5_QT_INCLUDE_DIRS}"
            OUTPUT_VARIABLE
              result
)

message(STATUS ${result})

if (DEEPIN_NMQT_TEST)
    add_definitions(-DUSE_DEEPIN_NMQT)
endif()


message(STATUS DEEPIN_NMQT_TEST ": ${DEEPIN_NMQT_TEST}")

add_subdirectory("src")
add_subdirectory("dcc-network-plugin")
add_subdirectory("tests")
add_subdirectory("example")
add_subdirectory("dock-network-plugin")
add_subdirectory("dde-network-dialog")
add_subdirectory("dss-network-plugin")
add_subdirectory("dss_example")
add_subdirectory("dock-example")
