// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccTitleObject {
    id: root
    property var config: null
    property string settingsID: ""
    function setConfig(c) {
        config = c
        settingsID = config.hasOwnProperty("id") ? config.id : ""
        root.configChanged()
    }
    function getConfig() {
        return config
    }
    function checkInput() {
        config.id = settingsID
        console.log("config.id.length", config.id, config.id.length)
        return config.id.length > 0
    }

    name: "genericTitle"
    displayName: qsTr("General")
    DccObject {
        name: "genericGroup"
        parentName: root.parentName
        weight: root.weight + 20
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "name"
            parentName: root.parentName + "/genericGroup"
            displayName: qsTr("Name")
            weight: 10
            pageType: DccObject.Editor
            page: D.LineEdit {
                enabled: config.type !== "802-11-wireless"
                text: settingsID
                onTextChanged: settingsID = text
            }
        }
        DccObject {
            name: "autoConnect"
            parentName: root.parentName + "/genericGroup"
            displayName: qsTr("Auto Connect")
            weight: 20
            pageType: DccObject.Editor
            page: D.Switch {
                checked: !config.hasOwnProperty("autoconnect") || config.autoconnect
                onClicked: config.autoconnect = checked
            }
        }
    }
}
