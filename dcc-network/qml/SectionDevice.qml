// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0
import "NetUtils.js" as NetUtils

DccTitleObject {
    id: root
    property var config: new Object
    property int type: NetType.WiredItem
    property bool canNotBind: true
    property var devItems: []
    property var devData: []
    property bool hasMTU: false
    property string interfaceName: ""
    property string ssid: ""
    property bool ssidEnabled: false

    property string errorKey: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        if (c === undefined) {
            config = {}
        } else {
            config = c
        }

        hasMTU = config.hasOwnProperty("mtu")
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
        config["mac-address"] = config.hasOwnProperty("mac-address") ? NetUtils.macToStr(config["mac-address"]) : ""
        if (config.hasOwnProperty("cloned-mac-address")) {
            config["cloned-mac-address"] = NetUtils.macToStr(config["cloned-mac-address"])
        }
        ssid = config.hasOwnProperty("ssid") ? config["ssid"] : ""
        ssidEnabled = type === NetType.WirelessItem && !config.hasOwnProperty("ssid")
    }
    function getConfig() {
        let saveConfig = config ? config : {}
        saveConfig["interfaceName"] = interfaceName
        if (config.hasOwnProperty("mac-address") && config["mac-address"] !== "") {
            saveConfig["mac-address"] = NetUtils.strToMac(config["mac-address"])
        }
        if (config.hasOwnProperty("cloned-mac-address")) {
            saveConfig["cloned-mac-address"] = NetUtils.strToMac(config["cloned-mac-address"])
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
        errorKey = ""
        if (type === NetType.WirelessItem && ssid.length === 0) {
            errorKey = "ssid"
            return false
        }
        if (config.hasOwnProperty("cloned-mac-address") && !NetUtils.macRegExp.test(config["cloned-mac-address"])) {
            errorKey = "cloned-mac-address"
            console.log(errorKey, config[errorKey])
            return false
        }

        return true
    }
    onErrorKeyChanged: console.log("dev errorKey", errorKey)
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
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (text !== ssid) {
                        ssid = text
                        root.editClicked()
                    }
                }
                showAlert: errorKey === dccObj.name
                alertDuration: 2000
                onShowAlertChanged: {
                    if (showAlert) {
                        dccObj.trigger()
                        forceActiveFocus()
                    }
                }
            }
        }
        DccObject {
            name: "mac-address"
            parentName: root.parentName + "/devGroup"
            weight: 20
            displayName: qsTr("Device MAC Addr")
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: config.hasOwnProperty("mac-address") ? indexOfValue(config["mac-address"]) : 0
                model: devModel
                onActivated: {
                    config["mac-address"] = currentValue
                    let name = /\((\w+)\)/.exec(currentText)
                    interfaceName = (name && name.length > 1) ? name[1] : ""
                    root.editClicked()
                }
                Component.onCompleted: {
                    if (config.hasOwnProperty("mac-address")) {
                        currentIndex = indexOfValue(config["mac-address"])
                        let name = /\((\w+)\)/.exec(currentText)
                        interfaceName = (name && name.length > 1) ? name[1] : ""
                    }
                }
            }
        }
        DccObject {
            name: "cloned-mac-address"
            parentName: root.parentName + "/devGroup"
            weight: 30
            displayName: qsTr("Cloned MAC Addr")
            visible: type === NetType.WiredItem
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: config.hasOwnProperty("cloned-mac-address") ? config["cloned-mac-address"] : ""
                validator: RegularExpressionValidator {
                    regularExpression: NetUtils.macRegExp
                }
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (text.length === 0) {
                        delete config["cloned-mac-address"]
                        delete config["assigned-mac-address"]
                    } else if (config["cloned-mac-address"] !== text) {
                        config["cloned-mac-address"] = text
                        root.editClicked()
                    }
                }
                showAlert: errorKey === dccObj.name
                alertDuration: 2000
                onShowAlertChanged: {
                    if (showAlert) {
                        dccObj.trigger()
                        forceActiveFocus()
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
                onClicked: {
                    hasMTU = checked
                    root.editClicked()
                }
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
                        root.editClicked()
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
                root.editClicked()
            }
            Component.onCompleted: {
                currentIndex = indexOfValue(config["band"])
            }
        }
    }
}
