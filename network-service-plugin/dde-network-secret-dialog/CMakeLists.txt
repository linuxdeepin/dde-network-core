# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE Dnetwork_Secret_Dialog_SRCS
    "src/*.h"
    "src/*.cpp"
)

list(REMOVE_ITEM Dnetwork_Secret_Dialog_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

set(QT_VERSION_MAJOR 6)
set(DTK_VERSION_MAJOR 6)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget)

set(Dmemory_Warning_Dialog_Includes
    Dtk${DTK_VERSION_MAJOR}::Widget
    Qt${QT_VERSION_MAJOR}::Widgets
)
set(Dnetwork_Secret_Dialog_Libraries
    Dtk${DTK_VERSION_MAJOR}::Widget
    Qt${QT_VERSION_MAJOR}::Widgets
)

add_subdirectory("src")
if (BUILD_TESTS)
    add_subdirectory(tests)
endif()
