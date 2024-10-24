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
    property var dnsItems: []
    property var dnsData: []
    property bool isEdit: false

    function setConfig(c) {
        isEdit = false
        config = c
        if (config === null) {
            dnsData = [0, 0]
        } else {
            dnsData = config
            while (dnsData.length < 2) {
                dnsData.push(0)
            }
            dnsDataChanged()
        }
    }
    function getConfig() {
        let saveData = []
        for (var d of dnsData) {
            if (d !== 0) {
                saveData.push(d)
            }
        }
        return saveData
    }
    function checkInput() {
        return true
    }
    function numToIp(num) {
        if (num === 0) {
            return ""
        }
        let ipArray = [0, 0, 0, 0]
        for (var i = 0; i < 4; i++) {
            ipArray[i] = (num >> (8 * i)) & 255
        }
        return ipArray.join('.')
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
    name: "dnsTitle"
    displayName: qsTr("DNS")
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
            displayName: qsTr("DNS") + index
            // hasBackground: true
            parentName: root.parentName + "/dnsGroup"
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    text: numToIp(dnsData[index])
                    onTextChanged: {
                        if (text.lenght !== 0 && dnsData.length > index) {
                            let ipNum = ipToNum(text)
                            if (ipNum > 0) {
                                dnsData[index] = ipNum
                            }
                        }
                    }
                }
                D.IconLabel {
                    Layout.margins: 0
                    Layout.maximumHeight: 16
                    visible: isEdit && root.dnsData.length < 3
                    // enabled: root.dnsData.length < 3
                    icon {
                        name: "list_add"
                        width: 16
                        height: 16
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            root.dnsData.push(0)
                            root.dnsDataChanged()
                        }
                    }
                }
                D.IconLabel {
                    Layout.margins: 0
                    Layout.maximumHeight: 16
                    visible: isEdit && root.dnsData.length > 2
                    // enabled: root.dnsData.length > 2
                    icon {
                        name: "list_delete"
                        width: 16
                        height: 16
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            root.dnsData.splice(index, 1)
                            root.dnsDataChanged()
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

    onDnsDataChanged: {
        while (dnsData.length > dnsItems.length) {
            addIpItem()
        }
        while (dnsData.length < dnsItems.length) {
            removeIpItem()
        }
    }
}
