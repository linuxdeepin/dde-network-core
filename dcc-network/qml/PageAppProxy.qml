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
    property var item: null
    property bool proxyEnable: item && item.isEnabled
    property var config: item ? item.config : {}
    property bool urlAlert: false
    property bool portAlert: false

    function checkInput() {
        if (proxyEnable) {
            urlAlert = false
            var ipv4Pattern = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
            if (!ipv4Pattern.test(root.config.url)) {
                urlAlert = true
                return false
            }
            portAlert = false
            let port = parseInt(root.config.port, 10)
            if (isNaN(port) || port <= 0 || port > 65535) {
                portAlert = true
                return false
            }
        }
        return true
    }

    visible: item
    displayName: qsTr("Application Proxy")
    description: qsTr("Set up proxy servers")
    icon: "dcc_app_agent"
    page: DccSettingsView {
        Component.onDestruction: {
            urlAlert = false
            portAlert = false
            proxyEnable = item.isEnabled
        }
    }

    DccObject {
        name: "body"
        parentName: root.name
        pageType: DccObject.Item
        DccObject {
            name: "title"
            parentName: root.name + "/body"
            displayName: root.displayName
            icon: "dcc_app_agent"
            weight: 10
            backgroundType: DccObject.Normal
            pageType: DccObject.Editor
            page: D.Switch {
                checked: proxyEnable
                enabled: item.enabledable
                onClicked: {
                    if (checked != proxyEnable) {
                        proxyEnable = checked
                    }
                    if (!proxyEnable) {
                        dccData.exec(NetManager.SetConnectInfo, item.id, {
                                         "enable": false
                                     })
                    }
                }
            }
        }
        DccObject {
            id: proxyConfig
            name: "config"
            parentName: root.name + "/body"
            weight: 20
            backgroundType: DccObject.Normal
            visible: proxyEnable
            pageType: DccObject.Item
            page: DccGroupView {}
            DccObject {
                name: "type"
                parentName: proxyConfig.parentName + "/" + proxyConfig.name
                displayName: qsTr("Proxy Type")
                weight: 10
                pageType: DccObject.Editor
                page: ComboBox {
                    flat: true
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: indexOfValue(root.config.type)
                    onActivated: root.config.type = currentValue
                    model: [{
                            "value": "http",
                            "text": qsTr("http")
                        }, {
                            "value": "socks4",
                            "text": qsTr("socks4")
                        }, {
                            "value": "socks5",
                            "text": qsTr("socks5")
                        }]
                    Component.onCompleted: {
                        root.config = item.config
                    }
                    Connections {
                        target: root
                        function onConfigChanged() {
                            currentIndex = indexOfValue(root.config.type)
                        }
                    }
                }
            }
            DccObject {
                name: "url"
                parentName: proxyConfig.parentName + "/" + proxyConfig.name
                displayName: qsTr("IP Address")
                weight: 20
                pageType: DccObject.Editor
                page: D.LineEdit {
                    topInset: 4
                    bottomInset: 4
                    validator: RegularExpressionValidator {
                        regularExpression: /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/
                    }
                    text: root.config.url
                    placeholderText: qsTr("Required")
                    showAlert: urlAlert
                    alertDuration: 2000
                    alertText: qsTr("Invalid IP address")
                    onShowAlertChanged: {
                        if (showAlert) {
                            DccApp.showPage(dccObj)
                            forceActiveFocus()
                        }
                    }
                    onTextChanged: {
                        if (root.config.url !== text) {
                            root.config.url = text
                        }
                    }
                }
            }
            DccObject {
                name: "port"
                parentName: proxyConfig.parentName + "/" + proxyConfig.name
                displayName: qsTr("Port")
                weight: 30
                pageType: DccObject.Editor
                page: D.LineEdit {
                    topInset: 4
                    bottomInset: 4
                    validator: IntValidator {
                        bottom: 0
                        top: 65535
                    }
                    text: root.config.port
                    placeholderText: qsTr("Required")
                    showAlert: portAlert
                    alertDuration: 2000
                    alertText: qsTr("Invalid port")
                    onShowAlertChanged: {
                        if (showAlert) {
                            DccApp.showPage(dccObj)
                            forceActiveFocus()
                        }
                    }
                    onTextChanged: {
                        if (root.config.port !== text) {
                            root.config.port = text
                        }
                    }
                }
            }
            DccObject {
                name: "user"
                parentName: proxyConfig.parentName + "/" + proxyConfig.name
                displayName: qsTr("Username")
                weight: 40
                pageType: DccObject.Editor
                page: D.LineEdit {
                    topInset: 4
                    bottomInset: 4
                    text: root.config.user
                    placeholderText: qsTr("Optional")
                    onTextChanged: {
                        if (root.config.user !== text) {
                            root.config.user = text
                        }
                    }
                }
            }
            DccObject {
                name: "password"
                parentName: proxyConfig.parentName + "/" + proxyConfig.name
                displayName: qsTr("Password")
                weight: 50
                pageType: DccObject.Editor
                page: D.PasswordEdit {
                    topInset: 4
                    bottomInset: 4
                    text: root.config.password
                    placeholderText: qsTr("Optional")
                    onTextChanged: {
                        if (root.config.password !== text) {
                            root.config.password = text
                        }
                    }
                }
            }
        }
        DccObject {
            name: "ignoreHostsTips"
            parentName: root.name + "/body"
            displayName: qsTr("Check \"Use a proxy\" in application context menu in Launcher after configured")
            canSearch: false
            weight: 90
            pageType: DccObject.Item
            page: Label {
                text: dccObj.displayName
                wrapMode: Text.WordWrap
            }
        }
    }
    DccObject {
        name: "footer"
        parentName: root.name
        pageType: DccObject.Item
        DccObject {
            name: "spacer"
            parentName: root.name + "/footer"
            visible: proxyEnable
            weight: 20
            pageType: DccObject.Item
            page: Item {
                Layout.fillWidth: true
            }
        }
        DccObject {
            name: "cancel"
            parentName: root.name + "/footer"
            visible: proxyEnable
            weight: 30
            pageType: DccObject.Item
            page: Button {
                implicitHeight: implicitContentHeight + 10
                implicitWidth: implicitContentWidth + 10
                topPadding: 0
                bottomPadding: 0
                leftPadding: 0
                rightPadding: 0
                spacing: 0
                text: qsTr("Cancel")
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    proxyEnable = item.isEnabled
                    root.config = root.item.config
                }
            }
        }
        DccObject {
            name: "Save"
            parentName: root.name + "/footer"
            visible: proxyEnable
            weight: 40
            pageType: DccObject.Item
            page: Button {
                implicitHeight: implicitContentHeight + 10
                implicitWidth: implicitContentWidth + 10
                topPadding: 0
                bottomPadding: 0
                leftPadding: 0
                rightPadding: 0
                spacing: 0
                text: qsTr("Save")
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    if (!checkInput()) {
                        return
                    }
                    let config = {}
                    if (proxyEnable) {
                        config = root.config
                    }
                    config.enable = proxyEnable

                    dccData.exec(NetManager.SetConnectInfo, item.id, config)
                }
            }
        }
    }
}
