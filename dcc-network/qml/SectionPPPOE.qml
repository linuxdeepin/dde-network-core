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
        root.config = c !== undefined ? c : {}
    }
    function getConfig() {
        return root.config
    }
    function checkInput() {
        if (!root.config.hasOwnProperty("username") || root.config.username.trim().length === 0) {
            errorKey = "username"
            return false
        }
        if (!root.config.hasOwnProperty("password") || root.config.password.length === 0) {
            errorKey = "password"
            return false
        }
        return true
    }

    name: "pppoeTitle"
    displayName: qsTr("PPPoE")
    canSearch: false
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
            canSearch: false
            weight: 10
            pageType: DccObject.Editor
            page: D.LineEdit {
                placeholderText: qsTr("Required")
                text: root.config.hasOwnProperty("username") ? root.config.username : ""
                showAlert: errorKey === dccObj.name
                alertDuration: 2000
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (root.config.username !== text) {
                        root.config.username = text
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
            canSearch: false
            weight: 20
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: root.config.hasOwnProperty("service") ? root.config.service : ""
                onTextChanged: {
                    if (root.config.service !== text) {
                        root.config.service = text
                        root.editClicked()
                    }
                }
            }
        }
        DccObject {
            name: "password"
            parentName: root.parentName + "/pppoeGroup"
            displayName: qsTr("Password")
            canSearch: false
            weight: 30
            pageType: DccObject.Editor
            page: NetPasswordEdit {
                dataItem: root
                text: root.config.hasOwnProperty("password") ? root.config.password : ""
                onTextUpdated: root.config.password = text
            }
        }
    }
}
