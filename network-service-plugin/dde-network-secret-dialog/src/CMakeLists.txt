# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(Dnetwork_Secret_Dialog_Name dde-network-secret-dialog)

add_executable(${Dnetwork_Secret_Dialog_Name}
    ${Dnetwork_Secret_Dialog_SRCS}
    main.cpp
    qrc/dde-network-secret-dialog.qrc
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
