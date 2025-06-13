cmake_minimum_required(VERSION 3.7)

set(PLUGIN_NAME "dcc-network-plugin")

project(${PLUGIN_NAME})

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(CMAKE_PREFER_PTHREAD_FLAG ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# 增加安全编译参数
ADD_DEFINITIONS("-fstack-protector-strong -D_FORTITY_SOURCE=1 -z noexecstack -pie -fPIC -z lazy")

# 指定所有源码文件
# file(GLOB_RECURSE SRCS "*.h"
#     "*.cpp"
# )

# 指定要用到的库
# -DQT_MAJOR_VERSION=6
# find_package(QT NAMES Qt6 Qt5)
# set(QT_MAJOR_VERSION 6)
# set(QT_DEFAULT_MAJOR_VERSION 6)
# find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 COMPONENTS Core Gui DBus Network LinguistTools REQUIRED)
find_package(DdeControlCenter REQUIRED)
find_package(Dtk6 COMPONENTS Core REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)
#pkg_check_modules(DDE-Network-Core REQUIRED dde-network-core)


set(DccNetwork_Name network)
file(GLOB_RECURSE DccNetwork_SRCS
        "*.cpp"
        "*.h"
        "../net-view/operation/*.cpp"
        "../net-view/operation/*.h"
        "../net-view/window/qrc/network.qrc"
        # "qrc/systeminfo.qrc"
)

add_library(${DccNetwork_Name} MODULE
        ${DccNetwork_SRCS}
)

target_include_directories(${DccNetwork_Name} PUBLIC
    Dde::Control-Center
    "../net-view/operation"
    "../src"
)

set(DccNetwork_Libraries
        Qt6::DBus
        Qt6::Network
        Qt6::Gui
        # Qt6::Quick
        Dtk6::Core
        KF6::NetworkManagerQt
        Dde::Control-Center
        dde-network-core6
)

target_link_libraries(${DccNetwork_Name} PRIVATE
        ${DccNetwork_Libraries}
)

dcc_install_plugin(NAME ${DccNetwork_Name} TARGET ${DccNetwork_Name})

dcc_handle_plugin_translation(NAME ${DccNetwork_Name} )
