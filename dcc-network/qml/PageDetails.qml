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
    property var detailsItems: []

    visible: item
    displayName: qsTr("Network Details")
    description: qsTr("View all network configurations")
    icon: "dcc_netinfo"
    page: DccSettingsView {}
    Component {
        id: detailsInfo
        DccObject {
            property var infoItem: null

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
                    Label {
                        Layout.alignment: Qt.AlignLeft
                        font: DccUtils.copyFont(D.DTK.fontManager.t4, {
                                                    "bold": true
                                                })
                        text: dccObj.displayName
                    }
                    D.IconLabel {
                        property bool clipboard: false
                        Layout.alignment: Qt.AlignRight
                        icon {
                            name: "copy"
                            palette: D.DTK.makeIconPalette(palette)
                        }
                        D.ToolTip {
                            id: tip
                            palette: parent.palette
                        }
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
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
                            implicitHeight: 36
                            text: modelData[0]
                            checked: false
                            backgroundVisible: true
                            corners: getCornersForBackground(index, repeater.count)
                            cascadeSelected: true
                            Layout.fillWidth: true
                            leftPadding: 10
                            rightPadding: 10
                            content: TextInput {
                                id: textInput
                                text: modelData[1]
                                color: palette.text
                                selectedTextColor: palette.highlightedText
                                selectionColor: palette.highlight
                                readOnly: true
                                selectByMouse: true
                                Loader {
                                    id: menuLoader
                                    active: false
                                    sourceComponent: D.Menu {
                                        D.MenuItem {
                                            text: qsTr("Copy")
                                            enabled: textInput.selectionStart !== textInput.selectionEnd
                                            onTriggered: {
                                                textInput.copy()
                                            }
                                        }
                                        D.MenuItem {
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

    DccObject {
        name: "body"
        parentName: root.name
        pageType: DccObject.Item
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
                text: qsTr("Network Detection")
                Layout.alignment: Qt.AlignRight
                onClicked: {
                    dccData.exec(NetManager.GoToSecurityTools, "net-check", {})
                }
            }
        }
    }

    function updateDetailsItems() {
        if (item) {
            const delDetailsItems = detailsItems.concat()
            for (let i in item.children) {
                let child = item.children[i]
                let index = delDetailsItems.findIndex(tmpItem => tmpItem.infoItem === child)
                if (index >= 0) {
                    delDetailsItems.splice(index, 1)
                } else {
                    let details = detailsInfo.createObject(root, {
                                                               "infoItem": child
                                                           })
                    DccApp.addObject(details)
                    detailsItems.push(details)
                }
            }
            for (const delItem of delDetailsItems) {
                DccApp.removeObject(delItem)
                let index = detailsItems.findIndex(item => delItem === item)
                if (index >= 0) {
                    detailsItems.splice(index, 1)
                }
                delItem.destroy()
            }
        }
    }

    onItemChanged: updateDetailsItems()
    Connections {
        target: item
        function onChildrenChanged() {
            updateDetailsItems()
        }
    }
}
