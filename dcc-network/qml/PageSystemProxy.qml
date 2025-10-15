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
    property int method: NetType.None
    property bool autoUrlAlert: false
    property int inputItem: 0

    function setItem(netItem) {
        item = netItem
        method = item.method
        autoUrlAlert = false
        // methodChanged()
    }
    function resetData() {
        root.method = root.item.method
        autoUrl.config = root.item.autoProxy
        http.config = root.item.manualProxy.http
        https.config = root.item.manualProxy.https
        ftp.config = root.item.manualProxy.ftp
        socks.config = root.item.manualProxy.socks
        ignoreHosts.config = root.item.manualProxy.ignoreHosts
    }

    visible: item
    displayName: qsTr("System Proxy")
    description: qsTr("Set up proxy servers")
    icon: "dcc_system_agent"
    pageType: DccObject.MenuEditor
    page: devCheck
    Component {
        id: devCheck
        D.Switch {
            checked: item.isEnabled
            enabled: item.enabledable
            onClicked: {
                dccData.exec(item.isEnabled ? NetManager.DisabledDevice : NetManager.EnabledDevice, item.id, {})
            }
        }
    }
    DccObject {
        name: "menu"
        parentName: root.name
        page: DccSettingsView {
            Component.onCompleted: root.resetData()
        }
        DccObject {
            name: "body"
            parentName: root.name + "/menu"
            pageType: DccObject.Item
            DccObject {
                name: "title"
                parentName: root.name + "/menu/body"
                displayName: root.displayName
                icon: "dcc_system_agent"
                weight: 10
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: D.Switch {
                    checked: root.method !== NetType.None
                    enabled: item.enabledable
                    onClicked: {
                        if (checked) {
                            root.method = item.lastMethod
                        } else {
                            root.method = NetType.None
                            dccData.exec(NetManager.SetConnectInfo, item.id, {
                                             "method": method
                                         })
                        }
                    }
                }
                Connections {
                    target: item
                    function onMethodChanged(m) {
                        method = m
                    }
                }
            }
            DccObject {
                id: methodObj

                name: "method"
                parentName: root.name + "/menu/body"
                displayName: qsTr("Proxy Type")
                weight: 20
                backgroundType: DccObject.Normal
                visible: method !== NetType.None
                pageType: DccObject.Editor
                page: ComboBox {
                    flat: true
                    textRole: "text"
                    valueRole: "value"
                    currentIndex: indexOfValue(method)
                    onActivated: {
                        method = currentValue
                    }
                    model: [{
                            "value": NetType.Auto,
                            "text": qsTr("Auto")
                        }, {
                            "value": NetType.Manual,
                            "text": qsTr("Manual")
                        }]
                    Component.onCompleted: {
                        currentIndex = indexOfValue(method)
                    }
                    Connections {
                        target: root
                        function onMethodChanged() {
                            currentIndex = indexOfValue(method)
                        }
                    }
                }
            }
            DccObject {
                id: autoUrl
                property var config: item.autoProxy
                name: "autoUrl"
                parentName: root.name + "/menu/body"
                displayName: qsTr("Configuration URL")
                weight: 30
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                visible: method === NetType.Auto
                page: D.LineEdit {
                    topInset: 4
                    bottomInset: 4
                    placeholderText: qsTr("Required")
                    text: dccObj.config
                    Layout.fillWidth: true
                    showAlert: autoUrlAlert
                    onTextChanged: {
                        autoUrlAlert = false
                        if (text.length === 0) {
                            inputItem &= 0x0f
                        } else {
                            inputItem |= 0x10
                        }

                        if (dccObj.config !== text) {
                            dccObj.config = text
                        }
                    }
                    onShowAlertChanged: {
                        if (showAlert) {
                            this.forceActiveFocus()
                        }
                    }
                }
            }
            SystemProxyConfigItem {
                id: http
                name: "http"
                parentName: root.name + "/menu/body"
                displayName: qsTr("HTTP Proxy")
                visible: method === NetType.Manual
                weight: 40
                config: root.item.manualProxy.http
                onHasUrlChanged: {
                    if (hasUrl) {
                        inputItem |= 0x01
                    } else {
                        inputItem &= ~0x01
                    }
                }
            }
            SystemProxyConfigItem {
                id: https
                name: "https"
                parentName: root.name + "/menu/body"
                displayName: qsTr("HTTPS Proxy")
                visible: method === NetType.Manual
                weight: 50
                config: root.item.manualProxy.https
                onHasUrlChanged: {
                    if (hasUrl) {
                        inputItem |= 0x02
                    } else {
                        inputItem &= ~0x02
                    }
                }
            }
            SystemProxyConfigItem {
                id: ftp
                name: "ftp"
                parentName: root.name + "/menu/body"
                displayName: qsTr("FTP Proxy")
                visible: method === NetType.Manual
                weight: 60
                config: root.item.manualProxy.ftp
                onHasUrlChanged: {
                    if (hasUrl) {
                        inputItem |= 0x04
                    } else {
                        inputItem &= ~0x04
                    }
                }
            }
            SystemProxyConfigItem {
                id: socks
                name: "socks"
                parentName: root.name + "/menu/body"
                displayName: qsTr("SOCKS Proxy")
                visible: method === NetType.Manual
                weight: 70
                config: root.item.manualProxy.socks
                onHasUrlChanged: {
                    if (hasUrl) {
                        inputItem |= 0x08
                    } else {
                        inputItem &= ~0x08
                    }
                }
            }
            DccObject {
                id: ignoreHosts
                property var config: root.item.manualProxy.ignoreHosts

                name: "ignoreHosts"
                parentName: root.name + "/menu/body"
                visible: method === NetType.Manual
                weight: 80
                pageType: DccObject.Item
                page: TextArea {
                    wrapMode: TextEdit.WordWrap
                    text: dccObj.config
                    onTextChanged: {
                        if (dccObj.config !== text) {
                            dccObj.config = text
                        }
                    }
                }
            }
            DccObject {
                name: "ignoreHostsTips"
                parentName: root.name + "/menu/body"
                displayName: qsTr("Ignore the proxy configurations for the above hosts and domains")
                weight: 90
                visible: ignoreHosts.visibleToApp
                pageType: DccObject.Item
                page: Label {
                    text: dccObj.displayName
                    wrapMode: Text.WordWrap
                }
            }
        }
        DccObject {
            name: "footer"
            parentName: root.name + "/menu"
            pageType: DccObject.Item
            DccObject {
                name: "spacer"
                parentName: root.name + "/menu/footer"
                visible: method !== NetType.None
                weight: 20
                pageType: DccObject.Item
                page: Item {
                    Layout.fillWidth: true
                }
            }
            DccObject {
                name: "cancel"
                parentName: root.name + "/menu/footer"
                visible: method !== NetType.None
                displayName: qsTr("Cancel")
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
                    text: dccObj.displayName
                    Layout.alignment: Qt.AlignRight
                    onClicked: root.resetData()
                }
            }
            DccObject {
                name: "Save"
                parentName: root.name + "/menu/footer"
                displayName: qsTr("Save")
                visible: method !== NetType.None
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
                    enabled: (method === NetType.Auto && (inputItem & 0xf0)) || (method === NetType.Manual && (inputItem & 0x0f))
                    text: dccObj.displayName
                    Layout.alignment: Qt.AlignRight
                    function printfObj(obj) {
                        for (let k in obj) {
                            console.log(k, obj[k])
                        }
                    }

                    onClicked: {
                        console.log("save:", method)
                        console.log("autoUrl.config :", autoUrl.config)
                        console.log("http.config:", http.config)
                        printfObj(http.config)
                        console.log("https.config:", https.config)
                        printfObj(https.config)
                        console.log("ftp.config:", ftp.config)
                        printfObj(ftp.config)
                        console.log("socks.config:", socks.config)
                        printfObj(socks.config)
                        console.log("ignoreHosts.config:", ignoreHosts.config)
                        let config = {}
                        switch (method) {
                        case NetType.None:
                            break
                        case NetType.Auto:
                            autoUrlAlert = false
                            if (autoUrl.config.length === 0) {
                                autoUrlAlert = true
                                return
                            }

                            config["autoUrl"] = autoUrl.config
                            break
                        case NetType.Manual:
                            if (!http.checkInput() || !https.checkInput() || !ftp.checkInput() || !socks.checkInput()) {
                                return
                            }
                            config["httpAddr"] = http.config["url"]
                            config["httpPort"] = http.config["port"]
                            config["httpAuth"] = http.config["auth"]
                            config["httpUser"] = http.config["user"]
                            config["httpPassword"] = http.config["password"]

                            config["httpsAddr"] = https.config["url"]
                            config["httpsPort"] = https.config["port"]
                            config["httpsAuth"] = https.config["auth"]
                            config["httpsUser"] = https.config["user"]
                            config["httpsPassword"] = https.config["password"]

                            config["ftpAddr"] = ftp.config["url"]
                            config["ftpPort"] = ftp.config["port"]
                            config["ftpAuth"] = ftp.config["auth"]
                            config["ftpUser"] = ftp.config["user"]
                            config["ftpPassword"] = ftp.config["password"]

                            config["socksAddr"] = socks.config["url"]
                            config["socksPort"] = socks.config["port"]
                            config["socksAuth"] = socks.config["auth"]
                            config["socksUser"] = socks.config["user"]
                            config["socksPassword"] = socks.config["password"]
                            config["ignoreHosts"] = ignoreHosts.config
                            break
                        }
                        config["method"] = method
                        dccData.exec(NetManager.SetConnectInfo, item.id, config)
                    }
                }
            }
        }
    }
}
