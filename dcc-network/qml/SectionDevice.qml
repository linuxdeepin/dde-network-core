// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccTitleObject {
    id: root
    property var config: null
    property int type: NetType.WiredItem
    property bool canNotBind: true
    property var devItems: []
    property var devData: []
    property bool hasMTU: false
    property string interfaceName: ""
    property string ssid: ""
    property bool ssidEnabled: false

    function setConfig(c) {
        config = c
        hasMTU = config && config.hasOwnProperty("mtu")
        hasMTUChanged()
        if (canNotBind) {
            if (devModel.count > 1) {
                devModel.remove(1, devModel.count - 1)
            }
        } else {
            devModel.clear()
        }
        if (config.hasOwnProperty("optionalDevice")) {
            for (let dev of config.optionalDevice) {
                devModel.append({
                                    "text": dev,
                                    "value": dev.split(' ')[0]
                                })
            }
        }
        ssid = config.hasOwnProperty("ssid") ? config["ssid"] : ""
        ssidEnabled = type === NetType.WirelessItem && !config.hasOwnProperty("ssid")
    }
    function getConfig() {
        let saveConfig = config ? config : {}
        saveConfig["interfaceName"] = interfaceName
        if (config.hasOwnProperty("mac-address")) {
            saveConfig["mac-address"] = config["mac-address"]
        }
        if (config.hasOwnProperty("cloned-mac-address")) {
            saveConfig["cloned-mac-address"] = config["cloned-mac-address"]
        }
        if (hasMTU) {
            saveConfig["mtu"] = config["mtu"]
        } else {
            delete saveConfig["mtu"]
        }

        if (type === NetType.WirelessItem) {
            saveConfig["ssid"] = ssid
        }
        saveConfig["band"] = config["band"]
        return saveConfig
    }
    function checkInput() {
        return true
    }
    function macToString(mac) {
        return Array.prototype.map.call(new Uint8Array(mac), x => ('00' + x.toString(16)).toUpperCase().slice(-2)).join(':')
    }
    function strToMac(str) {
        if (str.length === 0)
            return new Uint8Array()
        let arr = str.split(":")
        let hexArr = arr.join("")
        return new Uint8Array(hexArr.match(/[\da-f]{2}/gi).map(bb => {
                                                                   return parseInt(bb, 16)
                                                               })).buffer
    }

    function ipToNum(ip) {
        let octets = ip.split('.')
        let cidr = 0
        let ipNum = 0
        for (let octet of octets) {
            let num = parseInt(octet, 10)
            ipNum |= ((num & 255) << cidr)
            cidr += 8
        }
        return ipNum
    }
    ListModel {
        id: devModel
        ListElement {
            text: qsTr("Not Bind")
            value: ""
        }
    }
    name: "devTitle"
    displayName: type === NetType.WirelessItem ? qsTr("WLAN") : qsTr("Ethernet")
    DccObject {
        name: "devGroup"
        parentName: root.parentName
        weight: root.weight + 20
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "ssid"
            parentName: root.parentName + "/devGroup"
            weight: 10
            displayName: qsTr("SSID")
            pageType: DccObject.Editor
            visible: type === NetType.WirelessItem
            page: D.LineEdit {
                enabled: ssidEnabled
                text: ssid
                onTextChanged: ssid = text
            }
        }
        DccObject {
            name: "deviceMAC"
            parentName: root.parentName + "/devGroup"
            weight: 20
            displayName: qsTr("Device MAC Addr")
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: config.hasOwnProperty("mac-address") ? indexOfValue(macToString(config["mac-address"])) : 0
                model: devModel
                onActivated: {
                    config["mac-address"] = strToMac(currentValue)
                    let name = /\((\w+)\)/.exec(currentText)
                    interfaceName = (name && name.length > 1) ? name[1] : ""
                }
                Component.onCompleted: {
                    if (config.hasOwnProperty("mac-address")) {
                        currentIndex = indexOfValue(macToString(config["mac-address"]))
                        let name = /\((\w+)\)/.exec(currentText)
                        interfaceName = (name && name.length > 1) ? name[1] : ""
                    }
                }
            }
        }
        DccObject {
            name: "clonedMAC"
            parentName: root.parentName + "/devGroup"
            weight: 30
            displayName: qsTr("Cloned MAC Addr")
            visible: type === NetType.WiredItem
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: config.hasOwnProperty("cloned-mac-address") ? macToString(config["cloned-mac-address"]) : ""
                onTextChanged: {
                    if (text.length === 0) {
                        delete config["cloned-mac-address"]
                        delete config["assigned-mac-address"]
                    } else {
                        let mac = strToMac(text)
                        config["cloned-mac-address"] = mac
                    }
                }
            }
        }
        DccObject {
            name: "customizeMTU"
            parentName: root.parentName + "/devGroup"
            weight: 40
            displayName: qsTr("Customize MTU")
            pageType: DccObject.Editor
            page: D.Switch {
                checked: hasMTU
                onClicked: hasMTU = checked
            }
        }
        DccObject {
            name: "mtu"
            parentName: root.parentName + "/devGroup"
            weight: 50
            displayName: qsTr("MTU")
            visible: hasMTU
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: config.hasOwnProperty("mtu") ? config.mtu : 0
                onValueChanged: {
                    if (hasMTU && (!config.hasOwnProperty("mtu") || config.mtu !== value)) {
                        config.mtu = value
                    }
                }
            }
        }
    }
    // 配置在config[config.connection.type]中，显示在Generic中
    DccObject {
        name: "band"
        parentName: root.parentName + "/genericGroup"
        displayName: qsTr("Band")
        weight: 30
        visible: type === NetType.WirelessItem
        pageType: DccObject.Editor
        page: ComboBox {
            textRole: "text"
            valueRole: "value"
            model: [{
                    "text": qsTr("Auto"),
                    "value": undefined
                }, {
                    "text": qsTr("2.4 GHz"),
                    "value": "bg"
                }, {
                    "text": qsTr("5 GHz"),
                    "value": "a"
                }]
            currentIndex: indexOfValue(config["band"])
            onActivated: {
                config["band"] = currentValue
            }
            Component.onCompleted: {
                currentIndex = indexOfValue(config["band"])
            }
        }
    }
}
