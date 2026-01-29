cmake_minimum_required(VERSION 3.7)

project(example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH $ENV{Qt6_DIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core DBus Widgets Network REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)
find_package(Dtk6 COMPONENTS Widget REQUIRED)
find_package(PkgConfig REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-DSERVICE_CONFIG_DIR="${CMAKE_BINARY_DIR}/share/")
    ADD_DEFINITIONS(-DSERVICE_LIB_DIR="${CMAKE_BINARY_DIR}/network-service-plugin/")
else()
    ADD_DEFINITIONS(-DSERVICE_CONFIG_DIR="${CMAKE_INSTALL_PREFIX}/share/deepin-service-manager/")
    ADD_DEFINITIONS(-DSERVICE_LIB_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/deepin-service-manager/")
endif()

# 包含example自己的源文件
file(GLOB COMPILEFILES "*.cpp" "*.h" "service/*.cpp" "service/*.h")
file(GLOB_RECURSE QRCFILE "*.qrc")

add_executable(${PROJECT_NAME}
    ${COMPILEFILES}
    ${QRCFILE}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    .
    ./service
    ../src
    Dtk6::Widget
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
    KF6::NetworkManagerQt
    ${DDE-Network-Core_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Dtk6::Widget
    Qt6::DBus
    Qt6::Network
    Qt6::Widgets
    KF6::NetworkManagerQt
    dde-network-core6
)
