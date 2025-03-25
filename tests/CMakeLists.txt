cmake_minimum_required(VERSION 3.7)

project(tst-dde-network)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(CMAKE_PREFER_PTHREAD_FLAG ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets DBus Network REQUIRED)
find_package(Dtk6 COMPONENTS Core REQUIRED)
find_package(GTest REQUIRED)
find_package(KF6NetworkManagerQt REQUIRED)
find_package(PkgConfig REQUIRED)

# 用于测试覆盖率的编译条件
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-D SANITIZER_CHECK)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=address -O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address -O2")
endif()

aux_source_directory(. FILES)

add_executable(${PROJECT_NAME} ${FILES})

target_include_directories(${PROJECT_NAME} PUBLIC
    Qt6::Core
    Qt6::Network
    Dtk6::Core
    KF6::NetworkManagerQt
    ../src
    ../src/impl
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Dtk6::Core
    KF6::NetworkManagerQt
    ${GTEST_LIBRARIES}
    -lpthread
    -lm
    dde-network-core6
)
