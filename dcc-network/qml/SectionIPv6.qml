// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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
    property string gateway: ""

    property string errorKey: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        if (c === undefined) {
            config = {}
            method = "auto"
            addressData = []
            gateway = ""
        } else {
            config = c
            method = config.method
            addressData = (config && config.hasOwnProperty("address-data")) ? config["address-data"] : []
            gateway = (config && config.hasOwnProperty("gateway")) ? config["gateway"] : ""
        }
    }
    function getConfig() {
        let sConfig = config
        sConfig["method"] = method
        sConfig["address-data"] = addressData
        sConfig["gateway"] = gateway
        return sConfig
    }
    function checkInput() {
        errorKey = ""
        if (method === "manual") {
            for (let k in addressData) {
                if (!NetUtils.ipv6RegExp.test(addressData[k].address)) {
                    errorKey = k + "address"
                    return false
                }
                if (addressData[k].prefix < 0 || addressData[k].prefix > 128) {
                    errorKey = k + "prefix"
                    return false
                }
                if (k === "0" && gateway.length !== 0 && !NetUtils.ipv6RegExp.test(gateway)) {
                    errorKey = k + "gateway"
                    return false
                }
            }
        }
        return true
    }
    function resetAddressData() {
        if (method === "manual") {
            addressData = (config && config.hasOwnProperty("address-data")) ? config["address-data"] : []
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
    pageType: DccObject.Item
    visible: root.visible
    page: RowLayout {
        Label {
            text: dccObj.displayName
            font: DccUtils.copyFont(D.DTK.fontManager.t4, {
                                        "bold": true
                                    })
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
    DccObject {
        name: "ipv6Type"
        parentName: root.parentName
        weight: root.weight + 20
        backgroundType: DccObject.Normal
        visible: root.visible
        displayName: qsTr("Method")
        pageType: DccObject.Editor
        page: ComboBox {
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
        pageType: DccObject.Item
        page: D.CheckBox {
            text: dccObj.displayName
            checked: config.hasOwnProperty("never-default") ? config["never-default"] : false
            onClicked: {
                config["never-default"] = checked
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
                            }
                        }
                    }
                }
            }
            DccObject {
                id: ipGroup
                name: "ipGroup"
                parentName: ipv6Item.parentName + "/" + ipv6Item.name
                weight: 20
                pageType: DccObject.Item
                page: DccGroupView {}
                DccObject {
                    name: "address"
                    parentName: ipGroup.parentName + "/" + ipGroup.name
                    weight: 10
                    displayName: qsTr("IP Address")
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        text: addressData.length > index ? addressData[index].address : ""
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.ipv6RegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (addressData.length > index && addressData[index].address !== text) {
                                addressData[index].address = text
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 2000
                        alertText: qsTr("Invalid IP address")
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
                    pageType: DccObject.Editor
                    page: D.SpinBox {
                        value: addressData.length > index ? addressData[index].prefix : 64
                        from: 0
                        to: 128
                        onValueChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (addressData.length > index && addressData[index].prefix !== value) {
                                addressData[index].prefix = value
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 2000
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
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        enabled: index === 0
                        text: index === 0 ? gateway : ""
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.ipv6RegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (gateway !== text) {
                                gateway = text
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 2000
                        alertText: qsTr("Invalid IP address")
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
        while (addressData.length > ipItems.length) {
            addIpItem()
        }
        while (addressData.length < ipItems.length) {
            removeIpItem()
        }
    }
}
