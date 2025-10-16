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
    signal finished
    readonly property string parentUrl: parentName + "/" + name

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
            displayName: qsTr("%1 Network Properties").arg(sectionGeneric.settingsID)
        }
        SectionGeneric {
            id: sectionGeneric
            parentName: root.parentUrl + "/body"
            weight: 100
            onEditClicked: modified = true
        }
        SectionPPPOE {
            id: sectionPPPOE
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
        SectionDNS {
            id: sectionDNS
            parentName: root.parentUrl + "/body"
            weight: 400
            onEditClicked: modified = true
        }
        SectionDevice {
            id: sectionDevice
            parentName: root.parentUrl + "/body"
            weight: 500
            canNotBind: false
            onEditClicked: modified = true
        }
        SectionPPP {
            id: sectionPPP
            parentName: root.parentUrl + "/body"
            weight: 600
            onEditClicked: modified = true
        }
    }
    onConfigChanged: {
        sectionGeneric.setConfig(config.connection)
        sectionPPPOE.setConfig(config["pppoe"])
        sectionIPv4.setConfig(config.ipv4)
        // 合并IPv4和IPv6的DNS配置
        let combinedDns = []
        if (config.hasOwnProperty("ipv4") && config.ipv4.hasOwnProperty("dns") && config.ipv4.dns.length > 0) {
            combinedDns = combinedDns.concat(config.ipv4.dns)
        }
        if (config.hasOwnProperty("ipv6") && config.ipv6.hasOwnProperty("dns") && config.ipv6.dns.length > 0) {
            combinedDns = combinedDns.concat(config.ipv6.dns)
        }
        sectionDNS.setConfig(combinedDns.length > 0 ? combinedDns : null)
        sectionDevice.setConfig(config["802-3-ethernet"])
        sectionPPP.setConfig(config["ppp"])
        modified = config.connection.uuid === "{00000000-0000-0000-0000-000000000000}"
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
                contentItem: D.IconLabel {
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
                            contentItem: D.IconLabel {
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
                    if (!sectionGeneric.checkInput() || !sectionPPPOE.checkInput() || !sectionIPv4.checkInput() || !sectionDNS.checkInput() || !sectionDevice.checkInput() || !sectionPPP.checkInput()) {
                        return
                    }

                    let nConfig = {}
                    nConfig["connection"] = sectionGeneric.getConfig()
                    nConfig["pppoe"] = sectionPPPOE.getConfig()
                    nConfig["ipv4"] = sectionIPv4.getConfig()
                    if (nConfig["ipv4"] === undefined) {
                        nConfig["ipv4"] = {}
                    }
                    
                    // 获取DNS配置并分离IPv4和IPv6
                    let dnsConfig = sectionDNS.getConfig()
                    let ipv4Dns = []
                    let ipv6Dns = []
                    
                    for (let dns of dnsConfig) {
                        if (typeof dns === 'number') {
                            // IPv4 DNS（数字格式）
                            ipv4Dns.push(dns)
                        } else if (typeof dns === 'string') {
                            // IPv6 DNS（字符串格式）
                            ipv6Dns.push(dns)
                        }
                    }
                    
                    // 保存IPv4 DNS
                    nConfig["ipv4"]["dns"] = ipv4Dns
                    
                    // 如果有IPv6 DNS，也要保存
                    if (ipv6Dns.length > 0) {
                        if (!nConfig["ipv6"]) {
                            nConfig["ipv6"] = {}
                        }
                        nConfig["ipv6"]["dns"] = ipv6Dns
                    }
                    let devConfig = sectionDevice.getConfig()
                    if (devConfig.interfaceName.length !== 0) {
                        nConfig["pppoe"]["parent"] = devConfig.interfaceName
                    }
                    nConfig["connection"]["interface-name"] = devConfig.interfaceName
                    nConfig["802-3-ethernet"] = devConfig
                    nConfig["ppp"] = sectionPPP.getConfig()
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
