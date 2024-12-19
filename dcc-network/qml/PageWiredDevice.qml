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

    name: "wired" + item.pathIndex
    parentName: "network"
    displayName: item.name
    description: qsTr("Connect, edit network settings")
    backgroundType: DccObject.Normal
    icon: "dcc_ethernet"
    weight: 1010 + item.pathIndex
    pageType: DccObject.MenuEditor
    page: devCheck

    Component {
        id: devCheck
        D.Switch {
            checked: item.isEnabled
            enabled: item.enabledable
            onClicked: {
                dccData.exec(item.isEnabled ? NetManager.DisabledDevice : NetManager.EnabledDevice, item.id, {})
            }
        }
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
                page: devCheck
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
                            implicitHeight: 36
                            text: model.item.name
                            // icon.name: model.icon
                            hoverEnabled: true
                            checked: true
                            backgroundVisible: false
                            corners: getCornersForBackground(index, repeater.count)
                            cascadeSelected: true
                            Layout.fillWidth: true
                            icon.name: "network-online-symbolic"
                            // "network-setting"
                            content: RowLayout {
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
