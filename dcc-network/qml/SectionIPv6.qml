// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0
import "NetUtils.js" as NetUtils

DccObject {
    id: root
    property var config: null
    property int type: NetType.WiredItem
    property var ipItems: []
    property var addressData: []
    property bool isEdit: false
    property string method: "auto"
    property var gateway: []

    property string errorKey: ""
    property string errorMsg: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        if (c === undefined) {
            root.config = {}
            method = "auto"
            addressData = []
            gateway = []
        } else {
            root.config = c
            method = root.config.method
            addressData = (root.config && root.config.hasOwnProperty("address-data")) ? root.config["address-data"] : []
            gateway = []
            gateway[0] = (root.config && root.config.hasOwnProperty("gateway")) ? root.config["gateway"] : ""
        }
    }
    function getConfig() {
        let sConfig = root.config
        sConfig["method"] = method
        if (method === "manual") {
            sConfig["address-data"] = addressData
            for (let k in gateway) {
                if (gateway[k].length !== 0) {
                    sConfig["gateway"] = gateway[k]
                    break
                }
            }
        } else {
            delete sConfig["address-data"]
            delete sConfig["gateway"]
            delete sConfig["addresses"]

            // 禁用和忽略模式下不应该有任何IPv6配置字段
            if (method === "disabled" || method === "ignore") {
                delete sConfig["dns"]
                delete sConfig["dns-search"]
                delete sConfig["ignore-auto-dns"]
                delete sConfig["ignore-auto-routes"]
            }
        }
        return sConfig
    }
    function checkInput() {
        errorKey = ""
        errorMsg = ""
        if (method === "manual") {
            let gatewayCount = 0
            let ipSet = new Set()
            for (let k in addressData) {
                if (!NetUtils.ipv6RegExp.test(addressData[k].address)) {
                    errorKey = k + "address"
                    errorMsg = qsTr("Invalid IP address")
                    return false
                }
                if (addressData[k].prefix < 0 || addressData[k].prefix > 128) {
                    errorKey = k + "prefix"
                    errorMsg = qsTr("Invalid netmask")
                    return false
                }
                let ip = addressData[k].address
                if (ipSet.has(ip) && ip !== "") {
                    errorKey = k + "address"
                    errorMsg = qsTr("Duplicate IP address")
                    return false
                }
                // 检查网关
                if (gateway[k].length !== 0) {
                    gatewayCount++
                    if (gatewayCount >= 2) {
                        errorKey = k + "gateway"
                        errorMsg = qsTr("Only one gateway is allowed")
                        return false
                    }
                }
                if (gateway[k].length !== 0 && !NetUtils.ipv6RegExp.test(gateway[k])) {
                    errorKey = k + "gateway"
                    errorMsg = qsTr("Invalid gateway")
                    return false
                }
            }
        }
        return true
    }
    function resetAddressData() {
        if (method === "manual") {
            addressData = (root.config && root.config.hasOwnProperty("address-data")) ? root.config["address-data"] : []
            if (addressData.length === 0) {
                addressData.push({
                                     "address": "",
                                     "prefix": 64
                                 })
                addressDataChanged()
            }
        } else {
            addressData = []
        }
    }

    ListModel {
        id: allModel
        ListElement {
            value: "disabled"
            text: qsTr("Disabled")
        }
        ListElement {
            value: "manual"
            text: qsTr("Manual")
        }
        ListElement {
            value: "ignore"
            text: qsTr("Ignore")
        }
        ListElement {
            value: "auto"
            text: qsTr("Auto")
        }
    }
    ListModel {
        id: vpnModel
        ListElement {
            value: "disabled"
            text: qsTr("Disabled")
        }
        ListElement {
            value: "ignore"
            text: qsTr("Ignore")
        }
        ListElement {
            value: "auto"
            text: qsTr("Auto")
        }
    }

    name: "ipv6Title"
    displayName: qsTr("IPv6")
    canSearch: false
    pageType: DccObject.Item
    visible: root.visible
    page: RowLayout {
        DccLabel {
            property D.Palette textColor: D.Palette {
                normal: Qt.rgba(0, 0, 0, 0.9)
                normalDark: Qt.rgba(1, 1, 1, 0.9)
            }
            font: DccUtils.copyFont(D.DTK.fontManager.t5, {
                                        "weight": 500
                                    })
            text: dccObj.displayName
            color: D.ColorSelector.textColor
        }
        Item {
            Layout.fillWidth: true
        }
        Label {
            visible: root.method === "manual"
            text: isEdit ? qsTr("Done") : qsTr("Edit")
            color: palette.link
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    isEdit = !isEdit
                }
            }
        }
    }
    onParentItemChanged: {
        if (parentItem) {
            parentItem.leftPadding = 12
        }
    }
    DccObject {
        name: "ipv6Type"
        parentName: root.parentName
        weight: root.weight + 20
        backgroundType: DccObject.Normal
        visible: root.visible
        displayName: qsTr("Method")
        canSearch: false
        pageType: DccObject.Editor
        page: ComboBox {
            flat: true
            textRole: "text"
            valueRole: "value"
            currentIndex: indexOfValue(root.method)
            onActivated: {
                root.method = currentValue
                resetAddressData()
                root.editClicked()
            }
            model: type === NetType.VPNControlItem ? vpnModel : allModel
            Component.onCompleted: {
                currentIndex = indexOfValue(method)
                isEdit = false
                resetAddressData()
            }
            Connections {
                target: root
                function onConfigChanged() {
                    currentIndex = indexOfValue(root.method)
                }
            }
        }
    }
    DccObject {
        name: "ipv6Never"
        parentName: root.parentName
        weight: root.weight + 90
        visible: root.visible && type === NetType.VPNControlItem
        displayName: qsTr("Only applied in corresponding resources")
        canSearch: false
        pageType: DccObject.Item
        page: D.CheckBox {
            text: dccObj.displayName
            checked: root.config.hasOwnProperty("never-default") ? root.config["never-default"] : false
            onClicked: {
                root.config["never-default"] = checked
                root.editClicked()
            }
        }
    }
    Component {
        id: ipComponent
        DccObject {
            id: ipv6Item
            property int index: 0
            weight: root.weight + 30 + index
            name: "ipv6_" + index
            visible: root.visible
            displayName: "IP-" + index
            canSearch: false
            parentName: root.parentName
            pageType: DccObject.Item
            page: DccGroupView {
                isGroup: false
            }
            DccObject {
                name: "ipTitel"
                parentName: ipv6Item.parentName + "/" + ipv6Item.name
                weight: 10
                displayName: "IP-" + index
                canSearch: false
                pageType: DccObject.Item
                page: RowLayout {
                    Label {
                        text: dccObj.displayName
                        font: DccUtils.copyFont(D.DTK.fontManager.t6, {
                                                    "bold": true
                                                })
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    D.IconLabel {
                        Layout.margins: 0
                        Layout.maximumHeight: 16
                        visible: isEdit
                        icon {
                            name: "list_add"
                            width: 16
                            height: 16
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: {
                                root.addressData.push({
                                                          "address": "",
                                                          "prefix": 64
                                                      })
                                root.addressDataChanged()
                                root.editClicked()
                            }
                        }
                    }
                    D.IconLabel {
                        Layout.margins: 0
                        Layout.maximumHeight: 16
                        visible: isEdit && root.addressData.length > 1
                        icon {
                            name: "list_delete"
                            width: 16
                            height: 16
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: {
                                root.addressData.splice(index, 1)
                                root.addressDataChanged()
                                root.editClicked()
                            }
                        }
                    }
                }
            }
            DccObject {
                id: ipGroup
                name: "ipGroup"
                parentName: ipv6Item.parentName + "/" + ipv6Item.name
                canSearch: false
                weight: 20
                pageType: DccObject.Item
                page: DccGroupView {}
                DccObject {
                    name: "address"
                    parentName: ipGroup.parentName + "/" + ipGroup.name
                    weight: 10
                    displayName: qsTr("IP Address")
                    canSearch: false
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        text: root.addressData.length > index ? root.addressData[index].address : ""
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.ipv6RegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (root.addressData.length > index && root.addressData[index].address !== text) {
                                var addr = root.addressData[index]
                                addr.address = text
                                root.addressData[index] = addr
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 2000
                        alertText: errorKey === index + dccObj.name ? root.errorMsg : ""
                        onShowAlertChanged: {
                            if (showAlert) {
                                DccApp.showPage(dccObj)
                                forceActiveFocus()
                            }
                        }
                    }
                }
                DccObject {
                    name: "prefix"
                    parentName: ipGroup.parentName + "/" + ipGroup.name
                    weight: 20
                    displayName: qsTr("Prefix")
                    canSearch: false
                    pageType: DccObject.Editor
                    page: D.SpinBox {
                        editable: true
                        value: root.addressData.length > index ? root.addressData[index].prefix : 64
                        from: 0
                        to: 128
                        onValueChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (root.addressData.length > index && root.addressData[index].prefix !== value) {
                                var addr = root.addressData[index]
                                addr.prefix = value
                                root.addressData[index] = addr
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 2000
                        alertText: errorKey === index + dccObj.name ? root.errorMsg : ""
                        onShowAlertChanged: {
                            if (showAlert) {
                                DccApp.showPage(dccObj)
                                forceActiveFocus()
                            }
                        }
                    }
                }
                DccObject {
                    name: "gateway"
                    parentName: ipGroup.parentName + "/" + ipGroup.name
                    weight: 30
                    displayName: qsTr("Gateway")
                    canSearch: false
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        enabled: index === 0
                        text: index === 0 ? gateway[index] : ""
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.ipv6RegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (gateway[index] !== text) {
                                gateway[index] = text
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 2000
                        alertText: errorKey === index + dccObj.name ? root.errorMsg : ""
                        onShowAlertChanged: {
                            if (showAlert) {
                                DccApp.showPage(dccObj)
                                forceActiveFocus()
                            }
                        }
                    }
                }
            }
        }
    }
    function addIpItem() {
        let ipItem = ipComponent.createObject(root, {
                                                  "index": ipItems.length
                                              })
        DccApp.addObject(ipItem)
        ipItems.push(ipItem)
    }
    function removeIpItem() {
        let tmpItem = ipItems.pop()
        DccApp.removeObject(tmpItem)
        tmpItem.destroy()
    }

    onAddressDataChanged: {
        while (root.addressData.length > ipItems.length) {
            addIpItem()
        }
        while (root.addressData.length < ipItems.length) {
            removeIpItem()
        }
    }
}
