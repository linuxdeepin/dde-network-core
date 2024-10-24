// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var config: null
    property int type: NetType.WiredItem
    property var ipItems: []
    property var addressData: []
    property bool isEdit: false
    property string method: "auto"

    function setConfig(c) {
        config = c
        if (config === undefined) {
            method = "auto"
            addressData = []
        } else {
            method = config.method
            addressData = config.hasOwnProperty("addresses") ? config["addresses"] : []
        }
    }
    function getConfig() {
        let sConfig = {}
        sConfig["method"] = method
        sConfig["addresses"] = addressData //dccData.toNMUIntListList(addressData) // dccData.toNMList( addressData)
        return sConfig
    }
    function checkInput() {
        return true
    }
    function numToIp(num) {
        let ips = [0, 0, 0, 0]
        for (var i = 0; i < 4; i++) {
            ips[i] = (num >> (i * 8)) & 255
        }
        return ips.join('.')
    }
    function ipToNum(ip) {
        let ips = ip.split('.')
        let cidr = 0
        let ipNum = 0
        if (ips.length !== 4) {
            return "0.0.0.0"
        }
        for (let ipStr of ips) {
            let num = parseInt(ipStr, 10)
            ipNum |= ((num & 255) << cidr)
            cidr += 8
        }
        return ipNum
    }

    function prefixToIp(subnetMask) {
        if (subnetMask < 0 || subnetMask > 32) {
            throw new Error("Subnet mask must be between 0 and 32")
        }

        let maskArray = [255, 255, 255, 255]

        for (var i = 0; i < 4; i++) {
            let byteBits = i * 8 + 8 - subnetMask
            if (byteBits > 0) {
                maskArray[i] = (255 << byteBits) & 255
            }
        }

        return maskArray.join('.')
    }
    function ipToPrefix(decimalSubnet) {
        let octets = decimalSubnet.split('.')
        let cidr = 0

        for (let octet of octets) {
            let num = parseInt(octet, 10)
            for (var i = 0; i < 8; i++) {
                if ((num & (1 << (7 - i))) !== 0) {
                    cidr++
                } else {
                    break
                    // 一旦遇到0，就可以停止检查该字节的剩余位
                }
            }
            // 如果已经达到了32位，可以提前退出循环（虽然对于标准子网掩码这不是必需的）
            if (cidr === 32)
                break
        }

        return cidr
    }
    name: "ipv4Title"
    displayName: qsTr("IPv4")
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
        hasBackground: true
        displayName: qsTr("Method")
        pageType: DccObject.Editor
        page: ComboBox {
            textRole: "text"
            valueRole: "value"
            currentIndex: indexOfValue(root.method)
            onActivated: {
                root.method = currentValue
                if (root.method === "manual") {
                    root.addressData = (config && config.hasOwnProperty("addresses")) ? config["addresses"] : []
                    if (root.addressData.length === 0) {
                        root.addressData.push([0, 24, 0])
                        root.addressDataChanged()
                    }
                } else {
                    root.addressData = []
                }
            }
            model: type === NetType.VPNControlItem ? vpnModel : allModel
            Component.onCompleted: {
                currentIndex = indexOfValue(method)
                isEdit = false
                if (root.method === "manual") {
                    root.addressData = (config && config.hasOwnProperty("addresses")) ? config["addresses"] : []
                    if (root.addressData.length === 0) {
                        root.addressData.push([0, 24, 0])
                        root.addressDataChanged()
                    }
                } else {
                    root.addressData = []
                }
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
        pageType: DccObject.Item
        page: D.CheckBox {
            text: dccObj.displayName
            checked: config && config.hasOwnProperty("never-default") ? config["never-default"] : false
            onClicked: config["never-default"] = checked
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
            pageType: DccObject.Item
            page: DccGroupView {
                isGroup: false
            }
            DccObject {
                name: "ipTitel"
                parentName: ipv4Item.parentName + "/" + ipv4Item.name
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
                                root.addressData.push([0, 24, 0])
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
                parentName: ipv4Item.parentName + "/" + ipv4Item.name
                weight: 20
                pageType: DccObject.Item
                page: DccGroupView {}
                DccObject {
                    name: "addresses"
                    parentName: ipGroup.parentName + "/" + ipGroup.name
                    weight: 10
                    displayName: qsTr("IP Address")
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        text: addressData.length > index ? numToIp(addressData[index][0]) : "0.0.0.0"
                        onTextChanged: {
                            let ipNum = ipToNum(text)
                            if (addressData.length > index && addressData[index][0] !== ipNum) {
                                addressData[index][0] = ipNum
                            }
                        }
                    }
                }
                DccObject {
                    name: "prefix"
                    parentName: ipGroup.parentName + "/" + ipGroup.name
                    weight: 20
                    displayName: qsTr("Netmask")
                    pageType: DccObject.Editor
                    page: D.LineEdit {
                        text: addressData.length > index ? root.prefixToIp(addressData[index][1]) : "255.255.255.0"
                        onTextChanged: {
                            let prefix = root.ipToPrefix(text)
                            if (addressData.length > index && addressData[index][1] !== prefix) {
                                addressData[index][1] = prefix
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
                        text: index === 0 && addressData.length > index ? root.numToIp(addressData[index][2]) : ""
                        onTextChanged: {
                            let ipNum = ipToNum(text)
                            if (addressData.length > index && addressData[index][2] !== ipNum) {
                                addressData[index][2] = ipNum
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
