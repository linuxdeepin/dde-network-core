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

import "NetUtils.js" as NetUtils

DccObject {
    id: root
    property var item: null

    component DevCheck: D.Switch {
        checked: item.isEnabled
        enabled: item.enabledable
        onClicked: {
            dccData.exec(item.isEnabled ? NetManager.DisabledDevice : NetManager.EnabledDevice, item.id, {})
        }
    }

    name: "wired" + item.pathIndex
    parentName: "network"
    displayName: item.name
    description: qsTr("Connect, edit network settings")
    backgroundType: DccObject.Normal
    icon: "dcc_ethernet"
    weight: 1010 + item.pathIndex
    pageType: DccObject.MenuEditor
    page: RowLayout {
        DccLabel {
            text: NetUtils.getStatusName(item.status)
        }
        DevCheck {}
    }

    DccObject {
        name: "page"
        parentName: root.name
        page: DccSettingsView {}
        DccObject {
            name: "body"
            parentName: root.name + "/page"
            pageType: DccObject.Item
            DccObject {
                name: "title"
                parentName: root.name + "/page/body"
                displayName: item.name
                icon: "dcc_ethernet"
                weight: 10
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: DevCheck {}
            }
            DccObject {
                name: "nocable"
                parentName: root.name + "/page/body"
                displayName: item.name
                weight: 20
                backgroundType: DccObject.Normal
                pageType: DccObject.Item
                visible: item.status === NetType.DS_NoCable
                page: Item {
                    implicitHeight: 80
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Plug in the network cable first")
                    }
                }
            }
            DccObject {
                name: "networkList"
                parentName: root.name + "/page/body"
                weight: 30
                pageType: DccObject.Item
                backgroundType: DccObject.Normal
                page: ColumnLayout {
                    objectName: "noPadding"
                    clip: true
                    spacing: 0
                    Repeater {
                        id: repeater
                        model: NetItemModel {
                            root: item
                        }

                        delegate: ItemDelegate {
                            id: itemDelegate
                            height: 36
                            text: model.item.name
                            // icon.name: model.icon
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
                                name: "network-online-symbolic"
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
                }
                PageSettings {
                    id: wiredSettings
                    name: "wiredSettings"
                    parentName: root.name + "/page/body/networkList"
                    onFinished: DccApp.showPage(root)
                    type: NetType.WiredItem
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
                                    if (tmpItem.itemType !== NetType.WiredItem) {
                                        wiredSettings.displayName = qsTr("Add Network Connection")
                                    } else {
                                        wiredSettings.displayName = tmpItem.name
                                    }
                                    wiredSettings.item = tmpItem
                                    wiredSettings.config = param

                                    DccApp.showPage(wiredSettings)
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
        }
        DccObject {
            name: "footer"
            parentName: root.name + "/page"
            pageType: DccObject.Item
            DccObject {
                name: "addNetwork"
                parentName: root.name + "/page/footer"
                displayName: qsTr("Add Network Connection")
                weight: 40
                pageType: DccObject.Item
                page: NetButton {
                    text: dccObj.displayName
                    Layout.alignment: Qt.AlignRight
                    onClicked: {
                        dccData.exec(NetManager.ConnectInfo, item.id, {})
                    }
                }
            }
        }
    }
}
