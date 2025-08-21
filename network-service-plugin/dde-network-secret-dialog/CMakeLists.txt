# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(Dnetwork_Secret_Dialog_Name dde-network-secret-dialog)

file(GLOB_RECURSE Dnetwork_Secret_Dialog_SRCS
    "*.h"
    "*.cpp"
)
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


add_executable(${Dnetwork_Secret_Dialog_Name}
    ${Dnetwork_Secret_Dialog_SRCS}
    main.cpp
)

target_include_directories(${Dnetwork_Secret_Dialog_Name} PUBLIC
    ${Dnetwork_Secret_Dialog_Includes}
)

target_link_libraries(${Dnetwork_Secret_Dialog_Name} PRIVATE
    ${Dnetwork_Secret_Dialog_Libraries}
)

## bin
set(DAEMON_LIB_PATH "lib/deepin-daemon" CACHE STRING "daemon lib path")
install(TARGETS ${Dnetwork_Secret_Dialog_Name} DESTINATION ${DAEMON_LIB_PATH})
