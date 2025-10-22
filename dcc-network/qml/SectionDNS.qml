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
    property var config: []
    property var dnsItems: []
    property bool isEdit: false

    property string errorKey: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        isEdit = false
        if (c === undefined) {
            root.config = ["", ""]
        } else {
            let tmpConfig = []
            for (var i in c) {
                if (c[i] !== 0) {
                    // 支持两种格式：数字格式（旧的IPv4）和字符串格式（新的IPv4/IPv6）
                    if (typeof c[i] === 'number') {
                        let ip = NetUtils.numToIp(c[i])
                        tmpConfig.push(ip)
                    } else if (typeof c[i] === 'string' && c[i].length > 0) {
                        tmpConfig.push(c[i])
                    }
                }
            }
            while (tmpConfig.length < 2) {
                tmpConfig.push("")
            }
            root.config = tmpConfig
        }
    }
    function getConfig() {
        console.log("[DNS-GetConfig] Starting DNS config processing, raw config:", root.config)
        let ipv4Data = []
        let ipv6Data = []

        for (var ip of root.config) {
            if (ip !== "") {
                console.log("[DNS-GetConfig] Processing IP:", ip)
                // 检查是否为有效的IP地址（IPv4或IPv6）
                if (NetUtils.isValidIpAddress(ip)) {
                    if (NetUtils.isIpv4Address(ip)) {
                        // IPv4 DNS保存为数字格式
                        let ipNum = NetUtils.ipToNum(ip)
                        console.log("[DNS-GetConfig] IPv4 processed:", ip, "->", ipNum)
                        if (ipNum !== 0) {
                            ipv4Data.push(ipNum)
                        }
                    } else if (NetUtils.isIpv6Address(ip)) {
                        // IPv6 DNS保存为字符串格式
                        console.log("[DNS-GetConfig] IPv6 processed:", ip)
                        ipv6Data.push(ip)
                    }
                } else {
                    console.log("[DNS-GetConfig] Invalid IP address:", ip)
                }
            } else {
                console.log("[DNS-GetConfig] Empty IP skipped")
            }
        }

        // 返回混合数据，后端会根据类型分别处理
        let saveData = []
        for (let ipv4 of ipv4Data) {
            saveData.push(ipv4)
        }
        for (let ipv6 of ipv6Data) {
            saveData.push(ipv6)
        }

        console.log("[DNS-GetConfig] Final save data:", saveData, "IPv4:", ipv4Data, "IPv6:", ipv6Data)
        return saveData
    }
    function checkInput() {
        console.log("[DNS-Check] Starting DNS validation, config:", root.config)
        errorKey = ""
        for (var i in root.config) {
            if (root.config[i] !== "") {
                console.log("[DNS-Check] Validating DNS entry", i, ":", root.config[i])
                if (!NetUtils.isValidIpAddress(root.config[i])) {
                    console.log("[DNS-Check] Validation failed for DNS entry", i, ":", root.config[i])
                    errorKey = "dns" + i
                    return false
                } else {
                    console.log("[DNS-Check] Validation passed for DNS entry", i, ":", root.config[i])
                }
            } else {
                console.log("[DNS-Check] Empty DNS entry", i, "skipped")
            }
        }

        console.log("[DNS-Check] All DNS entries validated successfully")
        return true
    }

    name: "dnsTitle"
    displayName: qsTr("DNS")
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
            // visible: root.config.method === "manual"
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
        name: "dnsGroup"
        parentName: root.parentName
        weight: root.weight + 20
        canSearch: false
        pageType: DccObject.Item
        page: DccGroupView {}
    }
    Component {
        id: dnsComponent
        DccObject {
            id: dnsItem
            property int index: 0
            weight: root.weight + 30 + index
            name: "dns" + index
            displayName: qsTr("DNS") + (index + 1)
            parentName: root.parentName + "/dnsGroup"
            canSearch: false
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    text: root.config[index]
                    // 移除正则验证器，改用手动验证以支持IPv6
                    // 显式允许所有字符输入，包括冒号
                    inputMethodHints: Qt.ImhNone
                    onTextChanged: {
                        console.log("[DNS-Input] Text changed in DNS field", index, ":", text)
                        if (showAlert) {
                            errorKey = ""
                        }
                        if (text !== root.config[index]) {
                            console.log("[DNS-Input] Updating config[" + index + "] from", root.config[index], "to", text)
                            root.config[index] = text
                            root.editClicked()
                        }
                    }
                    showAlert: errorKey === dccObj.name
                    alertDuration: 2000
                    alertText: qsTr("Invalid IP address")
                    onShowAlertChanged: {
                        if (showAlert) {
                            DccApp.showPage(dccObj)
                            forceActiveFocus()
                        }
                    }
                }
                D.IconLabel {
                    Layout.margins: 0
                    Layout.maximumHeight: 16
                    visible: isEdit && root.config.length < 3
                    // enabled: root.config.length < 3
                    icon {
                        name: "list_add"
                        width: 16
                        height: 16
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            root.config.push("")
                            root.configChanged()
                        }
                    }
                }
                D.IconLabel {
                    Layout.margins: 0
                    Layout.maximumHeight: 16
                    visible: isEdit && root.config.length > 2
                    // enabled: root.config.length > 2
                    icon {
                        name: "list_delete"
                        width: 16
                        height: 16
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            root.config.splice(index, 1)
                            root.configChanged()
                        }
                    }
                }
            }
        }
    }
    function addIpItem() {
        let dnsItem = dnsComponent.createObject(root, {
                                                    "index": dnsItems.length
                                                })
        DccApp.addObject(dnsItem)
        dnsItems.push(dnsItem)
    }
    function removeIpItem() {
        let tmpItem = dnsItems.pop()
        DccApp.removeObject(tmpItem)
        tmpItem.destroy()
    }

    onConfigChanged: {
        while (root.config.length > dnsItems.length) {
            addIpItem()
        }
        while (root.config.length < dnsItems.length) {
            removeIpItem()
        }
    }
}
