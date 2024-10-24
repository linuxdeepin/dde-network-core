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
    property bool userAlert: false
    property bool passwordAlert: false
    name: "pppoeTitle"
    displayName: qsTr("PPPoE")

    function setConfig(c) {
        config = c !== undefined ? c : {}
    }
    function getConfig() {
        return config
    }
    function checkInput() {
        if (!config.hasOwnProperty("username") || config.username.trim().length === 0) {
            userAlert = true
            return false
        }
        if (!config.hasOwnProperty("password") || config.password.length === 0) {
            passwordAlert = true
            return false
        }
        return true
    }

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
                showAlert: userAlert
                alertDuration: 2000
                onTextChanged: {
                    userAlert = false
                    config.username = text
                }
                onShowAlertChanged: {
                    if (showAlert) {
                        dccObj.trigger()
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
                onTextChanged: config.service = text
            }
        }
        DccObject {
            name: "password"
            parentName: root.parentName + "/pppoeGroup"
            displayName: qsTr("Password")
            weight: 30
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                property bool newInput: false
                placeholderText: qsTr("Required")
                text: config.hasOwnProperty("password") ? config.password : ""
                showAlert: passwordAlert
                alertDuration: 2000
                echoButtonVisible: newInput
                onTextChanged: {
                    passwordAlert = false
                    config.password = text
                }
                onShowAlertChanged: {
                    if (showAlert) {
                        dccObj.trigger()
                        forceActiveFocus()
                    }
                }
                Component.onCompleted: newInput = !config.hasOwnProperty("password")
            }
        }
    }
}
