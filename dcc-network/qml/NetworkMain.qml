// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15
import Qt.labs.qmlmodels

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var wiredDevs: []
    property var wirelessDevs: []

    function showPage(cmd) {
        dccData.exec(NetManager.ShowPage, cmd, {})
    }
    Component.onCompleted: dccModule.showPageFun = showPage
    DccTitleObject {
        name: "connectionSettings"
        parentName: "network"
        displayName: qsTr("Connection settings")
        weight: 5
        onParentItemChanged: {
            if (parentItem) {
                parentItem.topPadding = 10
            }
        }
    }
    DccTitleObject {
        name: "relatedSettings"
        parentName: "network"
        displayName: qsTr("Related Settings")
        weight: 3025
        onParentItemChanged: {
            if (parentItem) {
                parentItem.topPadding = 10
            }
        }
    }
    PageAirplane {
        name: "airplaneMode"
        parentName: "network"
        weight: 3040
        netItem: dccData.root
    }
    DccRepeater {
        model: NetItemModel {
            root: dccData.root
        }
        delegate: DelegateChooser {
            role: "type"
            DelegateChoice {
                roleValue: NetType.WiredDeviceItem
                delegate: PageWiredDevice {
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.WirelessDeviceItem
                delegate: PageWirelessDevice {
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.VPNControlItem
                delegate: PageVPN {
                    name: "networkVpn"
                    parentName: "network"
                    weight: 3010
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.DSLControlItem
                delegate: PageDSL {
                    name: "dsl"
                    parentName: "network"
                    weight: 3020
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.HotspotControlItem
                delegate: PageHotspot {
                    name: "personalHotspot"
                    parentName: "network"
                    isAirplane: dccData.root.isEnabled
                    weight: 3030
                    Component.onCompleted: setNetItem(model.item)
                }
            }
            DelegateChoice {
                roleValue: NetType.SystemProxyControlItem
                delegate: PageSystemProxy {
                    name: "systemProxy"
                    parentName: "network"
                    weight: 3050
                    Component.onCompleted: setNetItem(model.item)
                }
            }
            DelegateChoice {
                roleValue: NetType.AppProxyControlItem
                delegate: PageAppProxy {
                    name: "applicationProxy"
                    parentName: "network"
                    weight: 3060
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.DetailsItem
                delegate: PageDetails {
                    name: "networkDetails"
                    parentName: "network"
                    weight: 3070
                    netItem: model.item
                }
            }
        }
    }
}
