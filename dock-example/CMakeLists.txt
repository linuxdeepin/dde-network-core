cmake_minimum_required(VERSION 3.7)

project(example-dock)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH $ENV{Qt6_DIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core DBus Widgets Network REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)
find_package(Dtk6 COMPONENTS Widget REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(DdeDock REQUIRED)

# 只包含dock-example自己的源文件
file(GLOB SRCS "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
    .
    ../src
    ../dock-network-plugin
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
    dock-network-plugin
    dde-network-core6
)
