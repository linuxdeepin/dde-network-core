// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import org.deepin.dcc 1.0

DccObject {
    property string cmd
    function showPage() {
        if (cmd.length !== 0 && children.length !== 0) {
            children[0].showPage(cmd)
            cmd = ""
        }
    }
    name: "network"
    parentName: "root"
    displayName: qsTr("Network")
    icon: "dcc_network"
    weight: 20
    onChildrenChanged: showPage()
    onActive: function (cmdParam) {
        cmd = cmdParam
        showPage()
    }
}
