// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import Qt.labs.qmlmodels 1.2

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

import "NetUtils.js" as NetUtils

DccObject {
    id: root
    property var netItem: null
    property var airplaneItem: null
    readonly property var c_levelString: ["-signal-no", "-signal-low", "-signal-medium", "-signal-high", "-signal-full"]

    name: "wireless" + netItem.pathIndex
    parentName: "network"
    displayName: netItem.name
    description: qsTr("Connect, edit network settings")
    backgroundType: DccObject.Normal
    icon: "dcc_wifi"
    weight: 2010 + netItem.pathIndex
    pageType: DccObject.MenuEditor
    page: DeviceStatusItem{
        netItem: root.netItem
        connectedNameVisible: true
        statusVisible: true
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
                    root: dccObj.netItem
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
                            rightPadding: 0 // 移除右边距，让箭头区域能扩展到边缘
                            topPadding: 0 // 恢复适量上边距
                            bottomPadding: 0 // 恢复适量下边距
                            spacing: 10
                            icon {
                                name: "network-wireless" + (model.item.flags ? "-6" : "") + c_levelString[model.item.strengthLevel] + (model.item.secure ? "-secure" : "") + "-symbolic"
                                height: 16
                                width: 16
                            }
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
                                    implicitHeight: implicitContentHeight - 4
                                    topInset: -4
                                    bottomInset: -4
                                    visible: model.item.status !== NetType.CS_Connecting && itemDelegate.hovered
                                    text: model.item.status === NetType.CS_Connected ? qsTr("Disconnect") : qsTr("Connect")
                                    Layout.alignment: Qt.AlignVCenter
                                    onClicked: {
                                        dccData.exec(model.item.status === NetType.CS_Connected ? NetManager.Disconnect : NetManager.ConnectOrInfo, model.item.id, {})
                                    }
                                }

                                // 箭头悬浮效果 - 右侧圆角（包含分割线）
                                Item {
                                    width: 30 // 20px悬浮区域 + 10px扩展到边缘
                                    height: 40 // 填满ItemDelegate高度

                                    Canvas {
                                        id: hoverCanvas
                                        anchors.fill: parent
                                        property color bgColor: "transparent"

                                        onPaint: {
                                            var ctx = getContext("2d")
                                            ctx.clearRect(0, 0, width, height)

                                            if (bgColor.a > 0) {
                                                ctx.fillStyle = bgColor
                                                ctx.beginPath()

                                                // 绘制右侧圆角的矩形
                                                ctx.moveTo(0, 0) // 左上角
                                                ctx.lineTo(width - 6, 0) // 右上角前
                                                ctx.arcTo(width, 0, width, 6, 6) // 右上圆角
                                                ctx.lineTo(width, height - 6) // 右下角前
                                                ctx.arcTo(width, height, width - 6, height, 6) // 右下圆角
                                                ctx.lineTo(0, height) // 左下角
                                                ctx.lineTo(0, 0) // 回到左上角

                                                ctx.fill()
                                            }
                                        }

                                        onBgColorChanged: requestPaint()
                                    }

                                    // 分割线 - 一直显示
                                    Rectangle {
                                        width: 1
                                        height: 20
                                        anchors.left: parent.left
                                        anchors.leftMargin: 0 // 在悬浮区域最左边
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: palette.windowText
                                        opacity: 0.05
                                    }

                                    D.IconLabel {
                                        anchors.centerIn: parent
                                        icon {
                                            name: "arrow_ordinary_right"
                                            palette: D.DTK.makeIconPalette(palette)
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        acceptedButtons: Qt.LeftButton

                                        onClicked: {
                                            dccData.exec(NetManager.ConnectInfo, model.item.id, {})
                                        }

                                        onEntered: {
                                            hoverCanvas.bgColor = Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.1)
                                        }

                                        onExited: {
                                            hoverCanvas.bgColor = "transparent"
                                        }

                                        onPressed: {
                                            hoverCanvas.bgColor = Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.2)
                                        }

                                        onReleased: {
                                            if (containsMouse) {
                                                hoverCanvas.bgColor = Qt.rgba(palette.windowText.r, palette.windowText.g, palette.windowText.b, 0.1)
                                            } else {
                                                hoverCanvas.bgColor = "transparent"
                                            }
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
            displayName: root.netItem.name
            icon: "dcc_wifi"
            weight: 10
            backgroundType: DccObject.Normal
            pageType: DccObject.Editor
            page: DeviceStatusItem{
                netItem: root.netItem
                statusVisible: false
            }
        }
        DccObject {
            name: "mineTitle"
            parentName: root.name + "/page"
            displayName: qsTr("My Networks")
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
            property var netItem: null

            name: "mineNetwork"
            parentName: root.name + "/page"
            weight: 40
            pageType: DccObject.Item
            backgroundType: DccObject.Normal
            visible: root.netItem && root.netItem.isEnabled && !root.netItem.apMode && this.netItem && this.netItem.children.length > 0
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

                Timer {
                    id: refreshTimer
                    interval: 60000 // 一分钟间隔
                    running: true
                    repeat: true
                    triggeredOnStart: true
                    onTriggered: {
                        // 定期扫描网络
                        dccData.exec(NetManager.RequestScan, root.netItem.id, {})
                    }
                }
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
            property var netItem: null

            name: "otherNetwork"
            parentName: root.name + "/page"
            weight: 60
            visible: root.netItem && root.netItem.isEnabled && !root.netItem.apMode && this.netItem && this.netItem.children.length > 0
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
            visible: root.netItem && root.netItem.apMode
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

                    const items = new Array(root.netItem)
                    while (items.length !== 0) {
                        let tmpItem = items[0]
                        if (tmpItem.id === id) {
                            wirelessSettings.netItem = tmpItem
                            if ((wirelessSettings.displayName !== tmpItem.name) || (DccApp.activeObject !== wirelessSettings)) {
                                wirelessSettings.displayName = tmpItem.name
                                wirelessSettings.type = tmpItem.itemType
                                wirelessSettings.config = param

                                DccApp.showPage(wirelessSettings)
                            }
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
        for (let i in root.netItem.children) {
            const child = root.netItem.children[i]
            switch (child.itemType) {
            case NetType.WirelessMineItem:
                if (mineNetwork.netItem !== child) {
                    mineNetwork.netItem = child
                }
                hasNet |= 1
                break
            case NetType.WirelessOtherItem:
                if (otherNetwork.netItem !== child) {
                    otherNetwork.netItem = child
                }
                hasNet |= 2
                break
            }
        }
        if (!(hasNet & 1)) {
            mineNetwork.netItem = null
        }
        if (!(hasNet & 2)) {
            otherNetwork.netItem = null
        }
    }

    Connections {
        target: root.netItem
        function onChildrenChanged() {
            updateItem()
        }
    }
    Component.onCompleted: {
        updateItem()
    }
}
