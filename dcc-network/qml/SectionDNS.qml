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
            config = ["", ""]
        } else {
            let tmpConfig = []
            for (var i in c) {
                if (c[i] !== 0) {
                    let ip = NetUtils.numToIp(c[i])
                    tmpConfig.push(ip)
                }
            }
            while (tmpConfig.length < 2) {
                tmpConfig.push("")
            }
            config = tmpConfig
        }
    }
    function getConfig() {
        let saveData = []
        for (var ip of config) {
            let ipNum = NetUtils.ipToNum(ip)
            if (ipNum !== 0) {
                saveData.push(ipNum)
            }
        }
        return saveData
    }
    function checkInput() {
        errorKey = ""
        for (var i in config) {
            if (config[i] !== "" && !NetUtils.ipRegExp.test(config[i])) {
                errorKey = "dns" + i
                return false
            }
        }

        return true
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
            parentName: root.parentName + "/dnsGroup"
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    text: config[index]
                    validator: RegularExpressionValidator {
                        regularExpression: NetUtils.ipRegExp
                    }
                    onTextChanged: {
                        if (showAlert) {
                            errorKey = ""
                        }
                        if (text !== config[index]) {
                            config[index] = text
                            root.editClicked()
                        }
                    }
                    showAlert: errorKey === dccObj.name
                    alertDuration: 2000
                    alertText: qsTr("Invalid IP address")
                    onShowAlertChanged: {
                        if (showAlert) {
                            dccObj.trigger()
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
        while (config.length > dnsItems.length) {
            addIpItem()
        }
        while (config.length < dnsItems.length) {
            removeIpItem()
        }
    }
}
