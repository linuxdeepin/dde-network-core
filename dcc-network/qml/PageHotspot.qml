// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import Qt.labs.qmlmodels 1.2
import QtQml.Models

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var item: null
    property var airplaneItem: null
    property var config: null
    property bool isAirplane: false
    property string interfaceName: ""

    function setItem(netItem) {
        if (item !== netItem) {
            item = netItem
            config = item.config
            updateDevModel()
        }
    }
    function updateDevModel() {
        if (devModel.count > 1) {
            devModel.remove(1, devModel.count - 1)
        }
        for (let dev of config["802-11-wireless"].optionalDevice) {
            devModel.append({
                                "text": dev,
                                "value": dev.split(' ')[0]
                            })
        }
    }

    function securityName(type) {
        switch (type) {
        case "wpa-psk":
            return qsTr("WPA/WPA2 Personal")
        case "sae":
            return qsTr("WPA3 Personal")
        default:
            return qsTr("None")
        }
    }
    function bandName(value) {
        switch (value) {
        case "bg":
            return qsTr("2.4 GHz")
        case "a":
            return qsTr("5 GHz")
        default:
            return qsTr("Auto")
        }
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

    ListModel {
        id: devModel
        ListElement {
            text: qsTr("Not Bind")
            value: ""
        }
    }
    visible: item && item.enabledable
    displayName: qsTr("Personal Hotspot")
    description: qsTr("Share the network")
    icon: "dcc_hotspot"
    backgroundType: DccObject.Normal
    pageType: DccObject.MenuEditor
    page: devCheck
    Component {
        id: devCheck
        Item {
            implicitHeight: switchControl.implicitHeight
            implicitWidth: switchControl.implicitWidth
            D.Switch {
                id: switchControl
                anchors.fill: parent
                checked: item.isEnabled
                enabled: item.enabledable && !isAirplane && item.deviceEnabled
                onClicked: {
                    if (checked) {
                        if (config["connection"]["uuid"] === "{00000000-0000-0000-0000-000000000000}") {
                            dccData.exec(NetManager.SetConnectInfo, item.id, config)
                        } else {
                            dccData.exec(NetManager.Connect, item.id, {
                                             "uuid": config["connection"]["uuid"]
                                         })
                        }
                    } else {
                        dccData.exec(NetManager.Disconnect, item.id, {
                                         "uuid": config["connection"]["uuid"]
                                     })
                    }
                }
            }
        }
    }
    Connections {
        target: item
        function onConfigChanged(config) {
            root.config = config
            updateDevModel()
        }
    }

    DccObject {
        name: "menu"
        parentName: root.name
        DccObject {
            name: "title"
            parentName: root.name + "/menu"
            displayName: root.displayName
            icon: "dcc_hotspot"
            weight: 10
            backgroundType: DccObject.Normal
            pageType: DccObject.Editor
            page: devCheck
        }

        DccObject {
            name: "airplaneTips"
            parentName: root.name + "/menu"
            visible: isAirplane
            displayName: qsTr("If you want to use the personal hotspot, disable Airplane Mode first and then enable the wireless network adapter.")
            weight: 20
            pageType: DccObject.Item
            page: D.Label {
                textFormat: Text.RichText
                text: qsTr("If you want to use the personal hotspot, disable <a style='text-decoration: none;' href='network/airplaneMode'>Airplane Mode</a> first and then enable the wireless network adapter.")
                wrapMode: Text.WordWrap
                onLinkActivated: link => DccApp.showPage(link)
            }
        }
        DccObject {
            name: "devEnabledTips"
            parentName: root.name + "/menu"
            visible: !item.deviceEnabled && !isAirplane
            displayName: qsTr("Enable <a style='text-decoration: none;' href='network'>Wireless Network Adapter</a> first if you want to use the personal hotspot.")
            weight: 30
            pageType: DccObject.Item
            page: D.Label {
                textFormat: Text.RichText
                text: dccObj.displayName
                wrapMode: Text.WordWrap
                onLinkActivated: function (link) {
                    if (item.optionalDevicePath.length > 0) {
                        DccApp.showPage("network/wireless" + item.optionalDevicePath[0].slice(40))
                    }
                }
            }
        }
        DccTitleObject {
            name: "hotspotTitle"
            parentName: root.name + "/menu"
            weight: 40
            visible: !isAirplane
            enabled: item.deviceEnabled
            displayName: qsTr("My Hotspot")
            pageType: DccObject.Item
            page: RowLayout {
                Label {
                    font: DccUtils.copyFont(D.DTK.fontManager.t5, {
                                                "bold": true
                                            })
                    text: dccObj.displayName
                    Layout.alignment: Qt.AlignLeft
                }
                D.ActionButton {
                    Layout.alignment: Qt.AlignRight
                    focusPolicy: Qt.NoFocus
                    icon {
                        name: "dcc_network_edit"
                        width: 16
                        height: 16
                    }
                    onClicked: editDialog.createObject(this, {
                                                           "config": root.config
                                                       }).show()
                }
            }
            Component {
                id: editDialog
                D.DialogWindow {
                    id: editDlg
                    property var config: null
                    property string keyMgmt: editDlg.config.hasOwnProperty("802-11-wireless-security") ? editDlg.config["802-11-wireless-security"]["key-mgmt"] : ""
                    property bool ssidAlert: false
                    property bool passwordAlert: false

                    modality: Qt.ApplicationModal
                    width: 380
                    icon: "preferences-system"
                    onClosing: destroy()
                    ColumnLayout {
                        width: parent.width
                        spacing: 0
                        Label {
                            Layout.fillWidth: true
                            Layout.leftMargin: 50
                            Layout.rightMargin: 50
                            Layout.bottomMargin: 10
                            text: qsTr("Edit My Hotspot")
                            font.bold: true
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: colLayout.implicitHeight
                            color: palette.base
                            radius: DS.Style.control.radius
                            ColumnLayout {
                                id: colLayout
                                width: parent.width
                                NetItemEditor {
                                    text: qsTr("Name (SSID)")
                                    corners: getCornersForBackground(0, colLayout.children.length)
                                    content: D.LineEdit {
                                        showAlert: ssidAlert
                                        alertDuration: 2000
                                        placeholderText: qsTr("Required")
                                        text: editDlg.config["802-11-wireless"].ssid
                                        onTextChanged: {
                                            ssidAlert = false
                                            editDlg.config["802-11-wireless"].ssid = text
                                        }
                                    }
                                }
                                NetItemEditor {
                                    text: qsTr("Security")
                                    corners: getCornersForBackground(1, colLayout.children.length)
                                    content: D.ComboBox {
                                        flat: true
                                        textRole: "text"
                                        valueRole: "value"
                                        model: [{
                                                "text": qsTr("None"),
                                                "value": ""
                                            }, {
                                                "text": qsTr("WPA/WPA2 Personal"),
                                                "value": "wpa-psk"
                                            }, {
                                                "text": qsTr("WPA3 Personal"),
                                                "value": "sae"
                                            }]
                                        currentIndex: indexOfValue(keyMgmt)
                                        onActivated: {
                                            if (!editDlg.config.hasOwnProperty("802-11-wireless-security")) {
                                                editDlg.config["802-11-wireless-security"] = {}
                                            }
                                            keyMgmt = currentValue
                                        }
                                        Component.onCompleted: {
                                            currentIndex = indexOfValue(keyMgmt)
                                        }
                                    }
                                }
                                NetItemEditor {
                                    text: qsTr("Password")
                                    corners: getCornersForBackground(2, colLayout.children.length)
                                    visible: keyMgmt.length !== 0
                                    content: D.PasswordEdit {
                                        showAlert: passwordAlert
                                        alertDuration: 2000
                                        placeholderText: qsTr("Required")
                                        text: editDlg.config.hasOwnProperty("802-11-wireless-security") ? editDlg.config["802-11-wireless-security"].psk : ""
                                        onTextChanged: {
                                            passwordAlert = false
                                            if (!editDlg.config.hasOwnProperty("802-11-wireless-security")) {
                                                editDlg.config["802-11-wireless-security"] = {}
                                            }
                                            editDlg.config["802-11-wireless-security"].psk = text
                                        }
                                    }
                                }
                                NetItemEditor {
                                    text: qsTr("Band")
                                    corners: getCornersForBackground(3, colLayout.children.length)
                                    content: D.ComboBox {
                                        flat: true
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
                                        currentIndex: indexOfValue(editDlg.config["802-11-wireless"].band)
                                        onActivated: {
                                            editDlg.config["802-11-wireless"].band = currentValue
                                        }
                                        Component.onCompleted: {
                                            currentIndex = indexOfValue(editDlg.config["802-11-wireless"].band)
                                        }
                                    }
                                }
                            }
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
                                // contentItem: D.IconLabel {
                                text: qsTr("Save")
                                //     color: "red"
                                // }
                                onClicked: {
                                    let ssid = editDlg.config["802-11-wireless"].ssid.trim()
                                    if (ssid.length === 0) {
                                        ssidAlert = true
                                        return
                                    }
                                    if (keyMgmt.length !== 0 && (!editDlg.config["802-11-wireless-security"].hasOwnProperty("psk") || editDlg.config["802-11-wireless-security"].psk.length < 8)) {
                                        passwordAlert = true
                                        return
                                    }
                                    close()
                                    switch (keyMgmt) {
                                    case "wpa-psk":
                                        editDlg.config["802-11-wireless-security"]["proto"] = ['wpa', 'rsn']
                                        editDlg.config["802-11-wireless-security"]["group"] = ['ccmp']
                                        editDlg.config["802-11-wireless-security"]["pairwise"] = ['ccmp']
                                        editDlg.config["802-11-wireless-security"]["psk-flags"] = 1
                                        editDlg.config["802-11-wireless-security"]["key-mgmt"] = keyMgmt
                                        break
                                    case "sae":
                                        editDlg.config["802-11-wireless-security"]["proto"] = ['rsn']
                                        editDlg.config["802-11-wireless-security"]["group"] = ['ccmp']
                                        editDlg.config["802-11-wireless-security"]["pairwise"] = ['ccmp']
                                        editDlg.config["802-11-wireless-security"]["psk-flags"] = 1
                                        editDlg.config["802-11-wireless-security"]["key-mgmt"] = keyMgmt
                                        break
                                    default:
                                        delete editDlg.config["802-11-wireless-security"]
                                        break
                                    }
                                    editDlg.config["connection"]["id"] = editDlg.config["802-11-wireless"].ssid
                                    dccData.exec(NetManager.SetConnectInfo, item.id, editDlg.config)
                                }
                            }
                        }
                    }
                }
            }
        }
        DccObject {
            id: hotspotConfig
            name: "hotspotConfig"
            parentName: root.name + "/menu"
            weight: 50
            visible: !isAirplane
            enabled: item.deviceEnabled
            pageType: DccObject.Item
            page: DccGroupView {}
            DccObject {
                name: "ssid"
                parentName: hotspotConfig.parentName + "/" + hotspotConfig.name
                displayName: qsTr("Name (SSID)")
                weight: 10
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: DccLabel {
                    text: root.config["802-11-wireless"].ssid
                }
            }
            DccObject {
                name: "securityType"
                parentName: hotspotConfig.parentName + "/" + hotspotConfig.name
                displayName: qsTr("Security")
                weight: 20
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: DccLabel {
                    text: securityName(root.config.hasOwnProperty("802-11-wireless-security") ? root.config["802-11-wireless-security"]["key-mgmt"] : "")
                }
            }
            DccObject {
                name: "password"
                parentName: hotspotConfig.parentName + "/" + hotspotConfig.name
                displayName: qsTr("Password")
                weight: 30
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                visible: root.config.hasOwnProperty("802-11-wireless-security")
                page: Rectangle {
                    property bool isEchoMode: false
                    color: "transparent"
                    implicitHeight: layoutView.implicitHeight
                    implicitWidth: layoutView.implicitWidth
                    Layout.fillWidth: true
                    RowLayout {
                        id: layoutView
                        TextInput {
                            text: root.config.hasOwnProperty("802-11-wireless-security") ? root.config["802-11-wireless-security"].psk : ""
                            color: this.palette.text
                            selectedTextColor: this.palette.highlightedText
                            selectionColor: this.palette.highlight
                            readOnly: true
                            echoMode: isEchoMode ? TextInput.Normal : TextInput.Password
                        }
                        D.ActionButton {
                            focusPolicy: Qt.NoFocus
                            onClicked: isEchoMode = !isEchoMode
                            icon.name: isEchoMode ? "entry_password_shown" : "entry_password_hide"
                        }
                    }
                }
            }
            DccObject {
                name: "band"
                parentName: hotspotConfig.parentName + "/" + hotspotConfig.name
                displayName: qsTr("Band")
                weight: 40
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: DccLabel {
                    text: bandName(root.config["802-11-wireless"].band)
                }
            }
        }
        DccTitleObject {
            name: "shareTitle"
            parentName: root.name + "/menu"
            visible: !isAirplane
            enabled: item.deviceEnabled
            weight: 60
            displayName: qsTr("Shared Settings")
        }
        DccObject {
            id: shareConfig
            name: "shareConfig"
            parentName: root.name + "/menu"
            visible: !isAirplane
            enabled: item.deviceEnabled
            weight: 70
            pageType: DccObject.Item
            page: DccGroupView {}
            // TODO: 后端无接口
            // DccObject {
            //     name: "shareDevice"
            //     parentName: shareConfig.parentName + "/" + shareConfig.name
            //     displayName: qsTr("Priority Shared Network")
            //     weight: 10
            //     backgroundType: DccObject.Normal
            //     pageType: DccObject.Editor
            //     page: D.ComboBox {
            //         textRole: "text"
            //         valueRole: "value"
            //         model: devModel
            //     }
            // }
            DccObject {
                name: "deviceMAC"
                parentName: shareConfig.parentName + "/" + shareConfig.name
                displayName: qsTr("Hotspot Sharing Device")
                weight: 20
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: D.ComboBox {
                    function updateCurrentIndex() {
                        if (root.config["802-11-wireless"].hasOwnProperty("mac-address")) {
                            currentIndex = indexOfValue(macToString(root.config["802-11-wireless"]["mac-address"]))
                            let name = /\((\w+)\)/.exec(currentText)
                            interfaceName = (name && name.length > 1) ? name[1] : ""
                        } else {
                            currentIndex = 0
                        }
                    }
                    flat: true
                    textRole: "text"
                    valueRole: "value"
                    // currentIndex: root.config["802-11-wireless"].hasOwnProperty("mac-address") ? indexOfValue(macToString(root.config["802-11-wireless"]["mac-address"])) : 0
                    model: devModel
                    onActivated: {
                        root.config["802-11-wireless"]["mac-address"] = strToMac(currentValue)
                        let name = /\((\w+)\)/.exec(currentText)

                        interfaceName = (name && name.length > 1) ? name[1] : ""
                        if (interfaceName.length === 0) {
                            delete root.config["802-11-wireless"]["mac-address"]
                            delete root.config["connection"]["interface-name"]
                        } else {
                            root.config["connection"]["interface-name"] = interfaceName
                        }
                        root.config["setAndConn"] = item.isEnabled
                        dccData.exec(NetManager.SetConnectInfo, item.id, root.config)
                    }
                    Component.onCompleted: {
                        updateCurrentIndex()
                    }
                    Connections {
                        target: item
                        function onConfigChanged() {
                            updateCurrentIndex()
                        }
                    }
                }
            }
        }
    }
}
