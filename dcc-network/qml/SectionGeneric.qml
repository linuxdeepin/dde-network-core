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
        root.config = c
        settingsID = root.config.hasOwnProperty("id") ? root.config.id : ""
        root.configChanged()
    }
    function getConfig() {
        return root.config
    }
    function checkInput() {
        root.config.id = settingsID
        errorKey = ""
        console.log("root.config.id.length", root.config.id, root.config.id.length)
        if (root.config.id.length === 0) {
            errorKey = "id"
        }

        return errorKey.length === 0
    }
    name: "genericTitle"
    displayName: qsTr("General")
    canSearch: false
    DccObject {
        name: "genericGroup"
        parentName: root.parentName
        weight: root.weight + 20
        canSearch: false
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "name"
            parentName: root.parentName + "/genericGroup"
            displayName: root.config.type === "802-11-wireless" ? qsTr("Name (SSID)") : qsTr("Name")
            canSearch: false
            weight: 10
            enabled: root.config.type !== "802-11-wireless" || !root.config.hasOwnProperty("id") || root.config.id.length === 0
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: settingsID
                placeholderText: qsTr("Required")
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
            canSearch: false
            weight: 20
            pageType: DccObject.Editor
            page: D.Switch {
                checked: !root.config.hasOwnProperty("autoconnect") || root.config.autoconnect
                onClicked: {
                    root.config.autoconnect = checked
                    root.editClicked()
                }
            }
        }
    }
}
