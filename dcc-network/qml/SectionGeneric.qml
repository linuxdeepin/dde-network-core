// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccTitleObject {
    id: root
    property var config: new Object()
    property string settingsID: ""

    property string errorKey: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        config = c
        settingsID = config.hasOwnProperty("id") ? config.id : ""
        root.configChanged()
    }
    function getConfig() {
        return config
    }
    function checkInput() {
        config.id = settingsID
        errorKey = ""
        console.log("config.id.length", config.id, config.id.length)
        if (config.type !== "802-11-wireless" && config.id.length === 0) {
            errorKey = "id"
        }

        return errorKey.length === 0
    }
    onErrorKeyChanged: console.log("generic errorKey", errorKey)
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
            enabled: config.type !== "802-11-wireless"
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: settingsID
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (settingsID !== text) {
                        root.editClicked()
                        settingsID = text
                    }
                }
                showAlert: errorKey === "id"
                alertDuration: 2000
                onShowAlertChanged: {
                    if (showAlert) {
                        DccApp.showPage(dccObj)
                        forceActiveFocus()
                    }
                }
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
                onClicked: {
                    config.autoconnect = checked
                    root.editClicked()
                }
            }
        }
    }
}
