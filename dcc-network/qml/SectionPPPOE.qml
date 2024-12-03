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

    property string errorKey: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        config = c !== undefined ? c : {}
    }
    function getConfig() {
        return config
    }
    function checkInput() {
        if (!config.hasOwnProperty("username") || config.username.trim().length === 0) {
            errorKey = "username"
            return false
        }
        if (!config.hasOwnProperty("password") || config.password.length === 0) {
            errorKey = "password"
            return false
        }
        return true
    }

    name: "pppoeTitle"
    displayName: qsTr("PPPoE")
    DccObject {
        name: "pppoeGroup"
        parentName: root.parentName
        weight: root.weight + 20
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "username"
            parentName: root.parentName + "/pppoeGroup"
            displayName: qsTr("Username")
            weight: 10
            pageType: DccObject.Editor
            page: D.LineEdit {
                placeholderText: qsTr("Required")
                text: config.hasOwnProperty("username") ? config.username : ""
                showAlert: errorKey === dccObj.name
                alertDuration: 2000
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (config.username !== text) {
                        config.username = text
                        root.editClicked()
                    }
                }
                onShowAlertChanged: {
                    if (showAlert) {
                        DccApp.showPage(dccObj)
                        forceActiveFocus()
                    }
                }
            }
        }
        DccObject {
            name: "service"
            parentName: root.parentName + "/pppoeGroup"
            displayName: qsTr("Service")
            weight: 20
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: config.hasOwnProperty("service") ? config.service : ""
                onTextChanged: {
                    if (config.service !== text) {
                        config.service = text
                        root.editClicked()
                    }
                }
            }
        }
        DccObject {
            name: "password"
            parentName: root.parentName + "/pppoeGroup"
            displayName: qsTr("Password")
            weight: 30
            pageType: DccObject.Editor
            page: NetPasswordEdit {
                dataItem: root
                text: config.hasOwnProperty("password") ? config.password : ""
                onTextUpdated: config.password = text
            }
        }
    }
}
