// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import Qt.labs.qmlmodels 1.2

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var item: null
    property var airplaneItem: null
    readonly property var c_levelString: ["-signal-no", "-signal-low", "-signal-medium", "-signal-high", "-signal-full"]

    name: "wireless" + item.pathIndex
    parentName: "network"
    displayName: item.name
    description: qsTr("Connect, edit network settings")
    backgroundType: DccObject.Normal
    icon: "dcc_wifi"
    weight: 2010 + item.pathIndex
    pageType: DccObject.MenuEditor
    page: devCheck

    Component {
        id: devCheck
        D.Switch {
            checked: root.item.isEnabled
            enabled: root.item.enabledable
            onClicked: {
                dccData.exec(root.item.isEnabled ? NetManager.DisabledDevice : NetManager.EnabledDevice, root.item.id, {})
            }
        }
    }

    Component {
        id: networkList
        ColumnLayout {
            objectName: "noPadding"
            clip: true
            spacing: 0
            Repeater {
                id: repeater
                model: NetItemModel {
                    root: dccObj.item
                }
                delegate: DelegateChooser {
                    role: "type"
                    DelegateChoice {
                        roleValue: NetType.WirelessItem
                        delegate: ItemDelegate {
                            id: itemDelegate
                            height: 36
                            text: model.item.name
                            hoverEnabled: true
                            checked: true
                            backgroundVisible: false
                            corners: getCornersForBackground(index, repeater.count)
                            cascadeSelected: true
                            Layout.fillWidth: true
                            leftPadding: 10
                            rightPadding: 10
                            spacing: 10
                            icon {
                                name: "network-wireless" + (item.flags & 0x10 ? "-6" : "") + c_levelString[item.strengthLevel] + (item.secure ? "-secure" : "") + "-symbolic"
                                height: 16
                                width: 16
                            }
                            // "network-online-symbolic"
                            // "network-setting"
                            content: RowLayout {
                                spacing: 10
                                BusyIndicator {
                                    running: model.item.status === NetType.CS_Connecting
                                    visible: running
                                }
                                DccCheckIcon {
                                    visible: model.item.status === NetType.CS_Connected && !itemDelegate.hovered
                                }
                                NetButton {
                                    visible: model.item.status !== NetType.CS_Connecting && itemDelegate.hovered
                                    text: model.item.status === NetType.CS_Connected ? qsTr("Disconnect") : qsTr("Connect")
                                    Layout.alignment: Qt.AlignCenter
                                    onClicked: {
                                        dccData.exec(model.item.status === NetType.CS_Connected ? NetManager.Disconnect : NetManager.ConnectOrInfo, model.item.id, {})
                                    }
                                }
                                D.IconLabel {
                                    icon.name: "arrow_ordinary_right"
                                    MouseArea {
                                        anchors.fill: parent
                                        acceptedButtons: Qt.LeftButton
                                        onClicked: {
                                            dccData.exec(NetManager.ConnectInfo, model.item.id, {})
                                        }
                                    }
                                }
                            }
                            onDoubleClicked: {
                                if (model.item.status === NetType.CS_UnConnected) {
                                    dccData.exec(NetManager.ConnectOrInfo, model.item.id, {})
                                }
                            }
                            background: DccItemBackground {
                                separatorVisible: true
                            }
                        }
                    }
                    DelegateChoice {
                        roleValue: NetType.WirelessHiddenItem
                        delegate: ItemDelegate {
                            implicitHeight: 36
                            checked: true
                            backgroundVisible: false
                            corners: getCornersForBackground(index, repeater.count)
                            cascadeSelected: true
                            Layout.fillWidth: true
                            leftPadding: 36
                            contentItem: Label {
                                verticalAlignment: Text.AlignVCenter
                                text: model.item.name
                                color: palette.link
                            }
                            onClicked: {
                                dccData.exec(NetManager.ConnectInfo, model.item.id, {})
                            }
                            background: DccItemBackground {
                                separatorVisible: true
                            }
                        }
                    }
                }
            }
        }
    }
    DccObject {
        name: "page"
        parentName: root.name
        DccObject {
            name: "title"
            parentName: root.name + "/page"
            displayName: root.item.name
            icon: "dcc_wifi"
            weight: 10
            backgroundType: DccObject.Normal
            pageType: DccObject.Editor
            page: devCheck
        }
        DccObject {
            name: "mineTitle"
            parentName: root.name + "/page"
            displayName: qsTr("My Networks")
            visible: mineNetwork.visible
            weight: 30
            pageType: DccObject.Item
            page: Label {
                font: DccUtils.copyFont(D.DTK.fontManager.t5, {
                                            "bold": true
                                        })
                text: dccObj.displayName
            }
            onParentItemChanged: {
                if (parentItem) {
                    parentItem.topPadding = 5
                    parentItem.bottomPadding = 0
                    parentItem.leftPadding = 10
                }
            }
        }
        DccObject {
            id: mineNetwork
            property var item: null

            name: "mineNetwork"
            parentName: root.name + "/page"
            weight: 40
            pageType: DccObject.Item
            backgroundType: DccObject.Normal
            visible: root.item && root.item.isEnabled && !root.item.apMode && this.item && this.item.children.length > 0
            page: networkList
        }
        DccObject {
            name: "otherTitle"
            parentName: root.name + "/page"
            displayName: qsTr("Other Networks")
            visible: otherNetwork.visible
            weight: 50
            pageType: DccObject.Item
            page: Label {
                font: DccUtils.copyFont(D.DTK.fontManager.t6, {
                                            "bold": true
                                        })
                text: dccObj.displayName
            }
            onParentItemChanged: {
                if (parentItem) {
                    parentItem.topPadding = 5
                    parentItem.bottomPadding = 0
                    parentItem.leftPadding = 10
                }
            }
        }
        DccObject {
            id: otherNetwork
            property var item: null

            name: "otherNetwork"
            parentName: root.name + "/page"
            weight: 60
            visible: root.item && root.item.isEnabled && !root.item.apMode && this.item && this.item.children.length > 0
            pageType: DccObject.Item
            backgroundType: DccObject.Normal
            page: networkList
        }
        DccObject {
            name: "airplaneTips"
            parentName: root.name + "/page"
            visible: root.airplaneItem && root.airplaneItem.isEnabled && root.airplaneItem.enabledable
            displayName: qsTr("Disable Airplane Mode first if you want to connect to a wireless network")
            weight: 70
            pageType: DccObject.Item
            page: D.Label {
                textFormat: Text.RichText
                text: qsTr("Disable <a style='text-decoration: none;' href='network/airplaneMode'>Airplane Mode</a> first if you want to connect to a wireless network")
                wrapMode: Text.WordWrap
                onLinkActivated: link => DccApp.showPage(link)
            }
        }
        DccObject {
            name: "airplaneTips"
            parentName: root.name + "/page"
            visible: root.item && root.item.apMode
            displayName: qsTr("Disable hotspot first if you want to connect to a wireless network")
            weight: 70
            pageType: DccObject.Item
            page: D.Label {
                textFormat: Text.RichText
                text: qsTr("<a style='text-decoration: none;' href='NetHotspotControlItem'>Disable hotspot</a> first if you want to connect to a wireless network")
                wrapMode: Text.WordWrap
                onLinkActivated: link => dccData.exec(NetManager.DisabledDevice, link, {})
            }
        }
        PageSettings {
            id: wirelessSettings
            name: "wirelessSettings"
            parentName: root.name + "/page/otherNetwork"
            onFinished: DccApp.showPage(root)
            type: NetType.WirelessItem
            Connections {
                target: dccData
                function onRequest(cmd, id, param) {
                    if (cmd !== NetManager.ConnectInfo) {
                        return
                    }

                    const items = new Array(root.item)
                    while (items.length !== 0) {
                        let tmpItem = items[0]
                        if (tmpItem.id === id) {
                            wirelessSettings.displayName = tmpItem.name
                            wirelessSettings.item = tmpItem
                            wirelessSettings.config = param

                            DccApp.showPage(wirelessSettings)
                            break
                        }
                        for (let i in tmpItem.children) {
                            items.push(tmpItem.children[i])
                        }
                        // items=items.concat(tmpItem.children)
                        items.shift()
                    }
                }
            }
        }
    }
    function updateItem() {
        let hasNet = 0
        for (let i in root.item.children) {
            const child = root.item.children[i]
            switch (child.itemType) {
            case NetType.WirelessMineItem:
                if (mineNetwork.item !== child) {
                    mineNetwork.item = child
                }
                hasNet |= 1
                break
            case NetType.WirelessOtherItem:
                if (otherNetwork.item !== child) {
                    otherNetwork.item = child
                }
                hasNet |= 2
                break
            }
        }
        if (!(hasNet & 1)) {
            mineNetwork.item = null
        }
        if (!(hasNet & 2)) {
            otherNetwork.item = null
        }
    }

    Connections {
        target: root.item
        function onChildrenChanged() {
            updateItem()
        }
    }
    Component.onCompleted: {
        updateItem()
    }
}
