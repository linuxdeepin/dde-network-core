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

DccObject {
    id: root
    property var netItem: null
    property var detailsItems: []

    visible: netItem
    displayName: qsTr("Network Details")
    description: qsTr("View all network configurations")
    icon: "dcc_netinfo"
    page: DccSettingsView {}
    DccObject {
        name: "body"
        parentName: root.name
        pageType: DccObject.Item
        DccRepeater {
            model: NetItemModel {
                root: netItem
            }
            delegate: DccObject {
                property var infoItem: model.item

                name: infoItem.name
                parentName: root.name + "/body"
                weight: 10 + infoItem.index
                pageType: DccObject.Item
                page: DccGroupView {
                    isGroup: false
                }
                DccObject {
                    name: "title"
                    parentName: root.name + "/body/" + infoItem.name
                    displayName: infoItem.name
                    weight: 10
                    pageType: DccObject.Item
                    page: RowLayout {
                        DccLabel {
                            property D.Palette textColor: D.Palette {
                                normal: Qt.rgba(0, 0, 0, 0.9)
                                normalDark: Qt.rgba(1, 1, 1, 0.9)
                            }
                            Layout.alignment: Qt.AlignLeft
                            Layout.fillWidth: true
                            font: DccUtils.copyFont(D.DTK.fontManager.t5, {
                                                        "weight": 500
                                                    })
                            color: D.ColorSelector.textColor
                            text: dccObj.displayName
                            elide: Text.ElideMiddle
                        }
                        D.ActionButton {
                            Accessible.id: "CopyBtn"
                            Accessible.role: Accessible.Button
                            property bool clipboard: false
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 30
                            icon {
                                name: "copy"
                                width: 16
                                height: 16
                            }
                            D.ToolTip {
                                id: tip
                                palette: parent.palette
                            }
                            hoverEnabled: true
                            background: Rectangle {
                                anchors.fill: parent
                                property D.Palette pressedColor: D.Palette {
                                    normal: Qt.rgba(0, 0, 0, 0.2)
                                    normalDark: Qt.rgba(1, 1, 1, 0.25)
                                }
                                property D.Palette hoveredColor: D.Palette {
                                    normal: Qt.rgba(0, 0, 0, 0.1)
                                    normalDark: Qt.rgba(1, 1, 1, 0.1)
                                }
                                radius: 6
                                color: parent.pressed ? D.ColorSelector.pressedColor : (parent.hovered ? D.ColorSelector.hoveredColor : "transparent")
                            }
                            focusPolicy: Qt.StrongFocus
                            onClicked: {
                                let text = [infoItem.name]
                                for (let i in infoItem.details) {
                                    text.push(infoItem.details[i][0] + "\t" + infoItem.details[i][1])
                                }
                                dccData.setClipboard(text.join("\n"))
                                tip.show(qsTr("Details has been copied"), 2000)
                            }
                        }
                    }
                }
                DccObject {
                    name: "details"
                    parentName: root.name + "/body/" + infoItem.name
                    weight: 20
                    pageType: DccObject.Item
                    onParentItemChanged: {
                        if (parentItem) {
                            parentItem.topPadding = 0
                            parentItem.bottomPadding = 0
                            parentItem.leftPadding = 0
                            parentItem.rightPadding = 0
                        }
                    }
                    page: ColumnLayout {
                        spacing: 0
                        Repeater {
                            id: repeater
                            model: infoItem.details
                            delegate: ItemDelegate {
                                Accessible.id: "ModelData"
                                implicitHeight: 36
                                text: modelData[0]
                                font: D.DTK.fontManager.t6
                                checked: false
                                backgroundVisible: true
                                corners: getCornersForBackground(index, repeater.count)
                                cascadeSelected: true
                                Layout.fillWidth: true
                                leftPadding: 10
                                rightPadding: 10
                                content: TextInput {
                                    Accessible.id: "TextInput"
                                    id: textInput
                                    text: modelData[1]
                                    color: palette.text
                                    font: D.DTK.fontManager.t7
                                    selectedTextColor: palette.highlightedText
                                    selectionColor: palette.highlight
                                    readOnly: true
                                    selectByMouse: true
                                    Loader {
                                        id: menuLoader
                                        active: false
                                        sourceComponent: D.Menu {
                                            Accessible.id: "Loader_Menu"
                                            D.MenuItem {
                                                Accessible.id: "Copy"
                                                text: qsTr("Copy")
                                                enabled: textInput.selectionStart !== textInput.selectionEnd
                                                onTriggered: {
                                                    textInput.copy()
                                                }
                                            }
                                            D.MenuItem {
                                                Accessible.id: "SelectAll"
                                                text: qsTr("Select All")
                                                onTriggered: {
                                                    textInput.selectAll()
                                                }
                                            }
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        acceptedButtons: Qt.RightButton
                                        onClicked: {
                                            menuLoader.active = true
                                            var mousePos = mapToItem(textInput, Qt.point(mouseX, mouseY))
                                            menuLoader.item.popup(textInput, mousePos)
                                        }
                                    }
                                }
                                background: DccItemBackground {
                                    separatorVisible: true
                                    backgroundType: DccObject.Normal
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    DccObject {
        name: "footer"
        parentName: root.name
        pageType: DccObject.Item
        DccObject {
            name: "checkNetwork"
            parentName: root.name + "/footer"
            weight: 40
            pageType: DccObject.Item
            visible: dccData.netCheckAvailable()
            page: NetButton {
                Accessible.id: "NetworkDetection"
                Accessible.role: Accessible.Button
                text: qsTr("Network Detection")
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    dccData.exec(NetManager.GoToSecurityTools, "net-check", {})
                }
            }
        }
    }
}
