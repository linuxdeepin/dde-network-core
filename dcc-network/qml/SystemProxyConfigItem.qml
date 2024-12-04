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
    property bool hasUrl: false

    property string errorKey: ""
    signal editClicked

    function checkInput() {
        errorKey = ""
        if (hasAuth) {
            if (!root.config.user || root.config.user.length === 0) {
                errorKey = "user"
                return false
            }
            if (!root.config.password || root.config.password.length === 0) {
                errorKey = "password"
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
            text: root.config.url
            placeholderText: qsTr("Optional")
            onTextChanged: {
                hasUrl = text.length !== 0
                if (root.config.url !== text) {
                    root.config.url = text
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
            text: root.config.port
            placeholderText: qsTr("Optional")
            onTextChanged: {
                if (root.config.port !== text) {
                    root.config.port = text
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
            checked: root.config.auth
            onClicked: {
                if (root.config.auth !== checked) {
                    root.config.auth = checked
                    hasAuth = root.config.auth
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
            text: root.config.user
            placeholderText: qsTr("Required")
            showAlert: errorKey === dccObj.name
            onTextChanged: {
                if (showAlert) {
                    errorKey = ""
                }
                if (root.config.user !== text) {
                    root.config.user = text
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
        page: NetPasswordEdit {
            dataItem: root
            text: root.config.password
            onTextUpdated: root.config.password = text
        }
    }
}
