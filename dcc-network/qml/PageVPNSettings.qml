// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.5
import Qt.labs.platform 1.1

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0
import "NetUtils.js" as NetUtils

DccObject {
    id: root
    property var config: null
    property var item: null
    property int vpnType: NetUtils.VpnTypeEnum["l2tp"]
    property bool modified: false
    signal finished
    readonly property string parentUrl: parentName + "/" + name

    function setConfig(c) {
        if (c === undefined) {
            return
        }
        config = c
        vpnType = NetUtils.toVpnTypeEnum(config.vpn["service-type"])
        sectionGeneric.setConfig(config.connection)
        sectionVPN.setConfig(config["vpn"])
        sectionIPv4.setConfig(config.ipv4)
        sectionIPv6.setConfig(config.ipv6)
        // 合并IPv4和IPv6的DNS配置  
        let combinedDns = []
        if (config.hasOwnProperty("ipv4") && config.ipv4.hasOwnProperty("dns") && config.ipv4.dns.length > 0) {
            combinedDns = combinedDns.concat(config.ipv4.dns)
        }
        if (config.hasOwnProperty("ipv6") && config.ipv6.hasOwnProperty("dns") && config.ipv6.dns.length > 0) {
            combinedDns = combinedDns.concat(config.ipv6.dns)
        }
        sectionDNS.setConfig(combinedDns.length > 0 ? combinedDns : null)
        modified = config.connection.uuid === "{00000000-0000-0000-0000-000000000000}"

        if (c.check && (!sectionGeneric.checkInput() || !sectionVPN.checkInput() || !sectionIPv4.checkInput() || !sectionIPv6.checkInput() || !sectionDNS.checkInput())) {
            return
        }
    }

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
        DccObject {
            name: "vpnType"
            parentName: root.parentUrl + "/body"
            weight: 50
            visible: config && config.connection.uuid === "{00000000-0000-0000-0000-000000000000}"
            displayName: qsTr("VPN Type")
            backgroundType: DccObject.Normal
            pageType: DccObject.Editor
            page: D.ComboBox {
                flat: true
                textRole: "text"
                valueRole: "value"
                currentIndex: indexOfValue(vpnType)
                model: [{
                        "text": "L2TP",
                        "value": NetUtils.VpnTypeEnum["l2tp"]
                    }, {
                        "text": "PPTP",
                        "value": NetUtils.VpnTypeEnum["pptp"]
                    }, {
                        "text": "OpenVPN",
                        "value": NetUtils.VpnTypeEnum["openvpn"]
                    }, {
                        "text": "OpenConnect",
                        "value": NetUtils.VpnTypeEnum["openconnect"]
                    }, {
                        "text": "StrongSwan",
                        "value": NetUtils.VpnTypeEnum["strongswan"]
                    }, {
                        "text": "VPNC",
                        "value": NetUtils.VpnTypeEnum["vpnc"]
                    }]
                onActivated: {
                    vpnType = currentValue
                    if (config.hasOwnProperty("optionalName")) {
                        let key = NetUtils.toVpnKey(vpnType).substring(31)
                        config.connection.id = config.optionalName[key]
                        sectionGeneric.setConfig(config.connection)
                    }
                }
                Component.onCompleted: currentIndex = indexOfValue(vpnType)
                Connections {
                    target: root
                    function onVpnTypeChanged() {
                        currentIndex = indexOfValue(vpnType)
                    }
                }
            }
        }

        SectionGeneric {
            id: sectionGeneric
            parentName: root.parentUrl + "/body"
            weight: 100
            onEditClicked: modified = true
        }
        SectionVPN {
            id: sectionVPN
            vpnType: root.vpnType
            parentName: root.parentUrl + "/body"
            weight: 200
            onEditClicked: modified = true
        }
        SectionIPv4 {
            id: sectionIPv4
            type: NetType.VPNControlItem
            parentName: root.parentUrl + "/body"
            weight: 1000
            onEditClicked: modified = true
        }
        SectionIPv6 {
            id: sectionIPv6
            type: NetType.VPNControlItem
            parentName: root.parentUrl + "/body"
            visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"] | NetUtils.VpnTypeEnum["openconnect"])
            weight: 1100
            onEditClicked: modified = true
        }
        SectionDNS {
            id: sectionDNS
            parentName: root.parentUrl + "/body"
            weight: 1200
            onEditClicked: modified = true
        }
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
            name: "export"
            parentName: root.parentUrl + "/footer"
            weight: 20
            visible: config && config.connection.uuid !== "{00000000-0000-0000-0000-000000000000}" && (vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["openvpn"]))
            pageType: DccObject.Item
            page: NetButton {
                text: qsTr("Export")
                onClicked: {
                    exportFileDialog.createObject(this).open()
                }
            }
            Component {
                id: exportFileDialog
                FileDialog {
                    visible: false
                    fileMode: FileDialog.SaveFile
                    nameFilters: [qsTr("*.conf")]
                    currentFile: root.config.connection.id
                    onAccepted: {
                        dccData.exec(NetManager.ExportConnect, item.id, {
                                         "file": currentFile.toString().replace("file://", "")
                                     })
                        this.destroy(10)
                    }
                    onRejected: this.destroy(10)
                }
            }
        }
        DccObject {
            name: "spacer"
            parentName: root.parentUrl + "/footer"
            weight: 30
            pageType: DccObject.Item
            page: Item {
                Layout.fillWidth: true
            }
        }
        DccObject {
            name: "cancel"
            parentName: root.parentUrl + "/footer"
            weight: 40
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
            weight: 50
            enabled: root.modified
            pageType: DccObject.Item
            page: NetButton {
                text: qsTr("Save")
                onClicked: {
                    if (!sectionGeneric.checkInput() || !sectionVPN.checkInput() || !sectionIPv4.checkInput() || !sectionIPv6.checkInput() || !sectionDNS.checkInput()) {
                        return
                    }

                    let nConfig = {}
                    nConfig["connection"] = sectionGeneric.getConfig()
                    nConfig["vpn"] = sectionVPN.getConfig()
                    nConfig["ipv4"] = sectionIPv4.getConfig()
                    nConfig["ipv6"] = sectionIPv6.getConfig()
                    if (nConfig["ipv4"] === undefined) {
                        nConfig["ipv4"] = {}
                    }
                    if (nConfig["ipv6"] === undefined) {
                        nConfig["ipv6"] = {}
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
                    
                    // 分别保存到IPv4和IPv6配置中
                    nConfig["ipv4"]["dns"] = ipv4Dns
                    nConfig["ipv6"]["dns"] = ipv6Dns
                    nConfig["vpn"]["service-type"] = NetUtils.toVpnKey(vpnType)
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
