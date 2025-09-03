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

    property string errorKey: ""
    property string errorMsg: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        if (c === undefined) {
            root.config = {}
            method = "auto"
            addressData = []
        } else {
            root.config = c
            method = root.config.method
            resetAddressData()
        }
    }
    function getConfig() {
        let sConfig = root.config
        sConfig["method"] = method
        if (method === "manual") {
            let tmpIpData = []
            for (let ipData of addressData) {
                let ip = NetUtils.ipToNum(ipData[0])
                let prefix = NetUtils.ipToPrefix(ipData[1])
                let gateway = NetUtils.ipToNum(ipData[2])
                tmpIpData.push([ip, prefix, gateway])
            }
            sConfig["addresses"] = tmpIpData
        } else {
            delete sConfig["addresses"]
            delete sConfig["address-data"]
            delete sConfig["gateway"]  // 非手动模式下不应保留手动设置的网关
            
            // 禁用模式下不应该有任何IPv4配置字段
            if (method === "disabled") {
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
            for (let k in addressData) {
                if (!NetUtils.ipRegExp.test(addressData[k][0])) {
                    errorKey = k + "address"
                    errorMsg = qsTr("Invalid IP address")
                    return false
                }
                if (!NetUtils.maskRegExp.test(addressData[k][1])) {
                    errorKey = k + "prefix"
                    errorMsg = qsTr("Invalid netmask")
                    return false
                }
                if (addressData[k][2].length !== 0 && !NetUtils.ipRegExp.test(addressData[k][2])) {
                    errorKey = k + "gateway"
                    errorMsg = qsTr("Invalid gateway")
                    return false
                }
            }
            let ipSet = new Set()
            for (let k in addressData) {
                if (!addressData[k] || !addressData[k][0]) {
                    continue
                }
                let ip = addressData[k][0]
                if (ipSet.has(ip)) {
                    errorKey = k + "address"
                    errorMsg = qsTr("Duplicate IP address")
                    return false
                }
                ipSet.add(ip)
            }
        }
        return true
    }

    function resetAddressData() {
        if (method === "manual") {
            let tmpIpData = []
            if (root.config.hasOwnProperty("addresses")) {
                for (let ipData of root.config["addresses"]) {
                    let ip = NetUtils.numToIp(ipData[0])
                    let prefix = NetUtils.prefixToIp(ipData[1])
                    let gateway = ipData[2] === 0 ? "" : NetUtils.numToIp(ipData[2])
                    tmpIpData.push([ip, prefix, gateway])
                }
            }
            addressData = tmpIpData
            if (addressData.length === 0) {
                addressData.push(["0.0.0.0", "255.255.255.0", ""])
                addressDataChanged()
            }
        } else {
            addressData = []
        }
    }

    name: "ipv4Title"
    displayName: qsTr("IPv4")
    canSearch: false
    pageType: DccObject.Item
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
            value: "auto"
            text: qsTr("Auto")
        }
    }
    DccObject {
        name: "ipv4Type"
        parentName: root.parentName
        weight: root.weight + 20
        backgroundType: DccObject.Normal
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
        name: "ipv4Never"
        parentName: root.parentName
        weight: root.weight + 90
        visible: type === NetType.VPNControlItem
        displayName: qsTr("Only applied in corresponding resources")
        canSearch: false
        pageType: DccObject.Item
        page: D.CheckBox {
            text: dccObj.displayName
            checked: root.config && root.config.hasOwnProperty("never-default") ? root.config["never-default"] : false
            onClicked: {
                root.config["never-default"] = checked
                root.editClicked()
            }
        }
    }
    Component {
        id: ipComponent
        DccObject {
            id: ipv4Item
            property int index: 0
            weight: root.weight + 30 + index
            name: "ipv4_" + index
            displayName: "IP-" + index
            parentName: root.parentName
            canSearch: false
            pageType: DccObject.Item
            page: DccGroupView {
                isGroup: false
            }
            DccObject {
                name: "ipTitel"
                parentName: ipv4Item.parentName + "/" + ipv4Item.name
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
                                root.addressData.push(["0.0.0.0", "255.255.255.0", ""])
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
                parentName: ipv4Item.parentName + "/" + ipv4Item.name
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
                        text: addressData.length > index ? addressData[index][0] : "0.0.0.0"
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.ipRegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (addressData.length > index && addressData[index][0] !== text) {
                                addressData[index][0] = text
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
                    displayName: qsTr("Netmask")
                    canSearch: false
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        text: addressData.length > index ? addressData[index][1] : "255.255.255.0"
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.maskRegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (addressData.length > index && addressData[index][1] !== text) {
                                addressData[index][1] = text
                                root.editClicked()
                            }
                        }
                        showAlert: errorKey === index + dccObj.name
                        alertDuration: 1
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
                        text: index === 0 && addressData.length > index ? addressData[index][2] : ""
                        validator: RegularExpressionValidator {
                            regularExpression: NetUtils.ipRegExp
                        }
                        onTextChanged: {
                            if (showAlert) {
                                errorKey = ""
                            }
                            if (addressData.length > index && addressData[index][2] !== text) {
                                addressData[index][2] = text
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
            }
        }
    }
    onAddressDataChanged: {
        while (addressData.length > ipItems.length) {
            let ipItem = ipComponent.createObject(root, {
                "index": ipItems.length
            })
            DccApp.addObject(ipItem)
            ipItems.push(ipItem)
        }
        while (addressData.length < ipItems.length) {
            let tmpItem = ipItems.pop()
            DccApp.removeObject(tmpItem)
            tmpItem.destroy()
        }
    }
}
