// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.5

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var config: null
    property var item: null
    property int type: NetType.WiredItem
    property bool modified: false
    readonly property string parentUrl: parentName + "/" + name
    signal finished

    weight: 10
    page: DccSettingsView {}
    DccObject {
        name: "body"
        parentName: root.parentUrl
        weight: 10
        pageType: DccObject.Item
        DccTitleObject {
            name: "mainTitle"
            parentName: root.parentUrl + "/body"
            weight: 10
            displayName: sectionGeneric.settingsID + qsTr(" Network Properties")
        }
        SectionGeneric {
            id: sectionGeneric
            parentName: root.parentUrl + "/body"
            weight: 100
            onEditClicked: modified = true
        }
        SectionSecret {
            id: sectionSecret
            parentName: root.parentUrl + "/body"
            weight: 200
            onEditClicked: modified = true
        }
        SectionIPv4 {
            id: sectionIPv4
            parentName: root.parentUrl + "/body"
            weight: 300
            onEditClicked: modified = true
        }
        SectionIPv6 {
            id: sectionIPv6
            parentName: root.parentUrl + "/body"
            weight: 400
            onEditClicked: modified = true
        }
        SectionDNS {
            id: sectionDNS
            parentName: root.parentUrl + "/body"
            weight: 500
            onEditClicked: modified = true
        }
        SectionDevice {
            id: sectionDevice
            parentName: root.parentUrl + "/body"
            weight: 600
            type: root.type
            onEditClicked: modified = true
            onSsidChanged: {
                if (root.type === NetType.WirelessItem && !root.config[root.config.connection.type].hasOwnProperty("id")) {
                    sectionGeneric.settingsID = sectionDevice.ssid
                }
            }
        }
    }
    onConfigChanged: {
        sectionGeneric.setConfig(config.connection)
        sectionSecret.type = type
        sectionSecret.setConfigWSecurity(config["802-11-wireless-security"])
        sectionSecret.setConfig802_1x(config["802-1x"])
        sectionIPv4.setConfig(config.ipv4)
        sectionIPv6.setConfig(config.ipv6)
        sectionDNS.setConfig((config.hasOwnProperty("ipv4") && config.ipv4.hasOwnProperty("dns")) ? config.ipv4.dns : null)
        sectionDevice.type = type
        sectionDevice.setConfig(config[config.connection.type])
        modified = config.connection.uuid === "{00000000-0000-0000-0000-000000000000}" && sectionGeneric.settingsID.length !== 0
    }

    DccObject {
        name: "footer"
        parentName: root.parentUrl
        weight: 20
        pageType: DccObject.Item
        DccObject {
            name: "delete"
            parentName: root.parentUrl + "/footer"
            weight: 10
            pageType: DccObject.Item
            visible: config && config.connection.uuid !== "{00000000-0000-0000-0000-000000000000}"
            page: NetButton {
                contentItem: IconLabel {
                    text: qsTr("Delete")
                    color: "red"
                }
                onClicked: {
                    deleteDialog.createObject(this).show()
                }
            }
        }
        Component {
            id: deleteDialog
            D.DialogWindow {
                modality: Qt.ApplicationModal
                width: 380
                icon: "preferences-system"
                onClosing: destroy(10)
                ColumnLayout {
                    width: parent.width
                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 50
                        Layout.rightMargin: 50
                        text: qsTr("Are you sure you want to delete this configuration?")
                        font.bold: true
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                    }
                    RowLayout {
                        Layout.topMargin: 10
                        Layout.bottomMargin: 10
                        Button {
                            Layout.fillWidth: true
                            text: qsTr("Cancel")
                            onClicked: close()
                        }
                        Rectangle {
                            implicitWidth: 2
                            Layout.fillHeight: true
                            color: this.palette.button
                        }

                        D.Button {
                            Layout.fillWidth: true
                            contentItem: IconLabel {
                                text: qsTr("Delete")
                                color: "red"
                            }
                            onClicked: {
                                close()
                                dccData.exec(NetManager.DeleteConnect, item.id, {
                                                 "uuid": config.connection.uuid
                                             })
                                root.finished()
                            }
                        }
                    }
                }
            }
        }

        DccObject {
            name: "spacer"
            parentName: root.parentUrl + "/footer"
            weight: 20
            pageType: DccObject.Item
            page: Item {
                Layout.fillWidth: true
            }
        }
        DccObject {
            name: "cancel"
            parentName: root.parentUrl + "/footer"
            weight: 30
            pageType: DccObject.Item
            page: NetButton {
                text: qsTr("Cancel")
                onClicked: {
                    root.finished()
                }
            }
        }
        DccObject {
            name: "save"
            parentName: root.parentUrl + "/footer"
            weight: 40
            enabled: root.modified
            pageType: DccObject.Item
            page: NetButton {
                text: qsTr("Save")
                onClicked: {
                    if (!sectionGeneric.checkInput() || !sectionSecret.checkInput() || !sectionIPv4.checkInput() || !sectionIPv6.checkInput() || !sectionDNS.checkInput() || !sectionDevice.checkInput()) {
                        return
                    }

                    let nConfig = {}
                    nConfig["connection"] = sectionGeneric.getConfig()
                    nConfig["802-11-wireless-security"] = sectionSecret.getConfigWSecurity()
                    nConfig["802-1x"] = sectionSecret.getConfig802_1x()
                    nConfig["ipv4"] = sectionIPv4.getConfig()
                    nConfig["ipv6"] = sectionIPv6.getConfig()
                    if (nConfig["ipv4"] === undefined) {
                        nConfig["ipv4"] = {}
                    }
                    nConfig["ipv4"]["dns"] = sectionDNS.getConfig()
                    let devConfig = sectionDevice.getConfig()
                    if (devConfig.interfaceName.length === 0) {
                        delete nConfig["connection"]["interface-name"]
                    } else {
                        nConfig["connection"]["interface-name"] = devConfig.interfaceName
                    }

                    nConfig[config.connection.type] = devConfig
                    if (item) {
                        dccData.exec(NetManager.SetConnectInfo, item.id, nConfig)
                    } else {
                        dccData.exec(NetManager.SetConnectInfo, "", nConfig)
                    }
                    root.modified = false
                    root.finished()
                }
            }
        }
    }
}
