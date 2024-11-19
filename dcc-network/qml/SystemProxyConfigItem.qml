// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import Qt.labs.qmlmodels 1.2

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var config: null
    property bool hasAuth: config.auth
    property bool userAlert: false
    property bool passwordAlert: false
    property bool hasUrl: false

    function checkInput() {
        if (hasAuth) {
            userAlert = false
            if (!config.user || config.user.lenght === 0) {
                userAlert = true
                return false
            }
            passwordAlert = false
            if (!config.password || config.password.lenght === 0) {
                passwordAlert = true
                return false
            }
        }
        return true
    }
    pageType: DccObject.Item
    page: DccGroupView {}
    DccObject {
        name: "url"
        parentName: root.parentName + "/" + root.name
        displayName: root.displayName
        weight: 10
        pageType: DccObject.Editor
        page: D.LineEdit {
            topInset: 4
            bottomInset: 4
            text: config.url
            placeholderText: qsTr("Optional")
            onTextChanged: {
                hasUrl = text.length !== 0
                if (config.url !== text) {
                    config.url = text
                }
            }
        }
    }
    DccObject {
        name: "port"
        parentName: root.parentName + "/" + root.name
        displayName: qsTr("Port")
        weight: 20
        pageType: DccObject.Editor
        page: D.LineEdit {
            topInset: 4
            bottomInset: 4
            validator: IntValidator {
                bottom: 0
                top: 65535
            }
            text: config.port
            placeholderText: qsTr("Optional")
            onTextChanged: {
                if (config.port !== text) {
                    config.port = text
                }
            }
        }
    }
    DccObject {
        name: "auth"
        parentName: root.parentName + "/" + root.name
        displayName: qsTr("Authentication is required")
        weight: 30
        pageType: DccObject.Editor
        page: Switch {
            checked: config.auth
            onClicked: {
                if (config.auth !== checked) {
                    config.auth = checked
                    hasAuth = config.auth
                }
            }
        }
    }
    DccObject {
        id: user
        name: "user"
        parentName: root.parentName + "/" + root.name
        displayName: qsTr("Username")
        visible: hasAuth
        weight: 40
        pageType: DccObject.Editor
        page: D.LineEdit {
            topInset: 4
            bottomInset: 4
            text: config.user
            placeholderText: qsTr("Required")
            showAlert: userAlert
            onTextChanged: {
                userAlert = false
                if (config.user !== text) {
                    config.user = text
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
        name: "password"
        parentName: root.parentName + "/" + root.name
        displayName: qsTr("Password")
        visible: hasAuth
        weight: 50
        pageType: DccObject.Editor
        page: D.PasswordEdit {
            topInset: 4
            bottomInset: 4
            text: config.password
            placeholderText: qsTr("Required")
            showAlert: passwordAlert
            onTextChanged: {
                passwordAlert = false
                if (config.password !== text) {
                    config.password = text
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
}
