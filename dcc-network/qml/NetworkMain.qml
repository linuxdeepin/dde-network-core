// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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
        objectName: "NetworkMain_PageAirplane"
        Accessible.role: Accessible.Grouping
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
                    objectName: "NetworkMain_PageWiredDevice_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.WirelessDeviceItem
                delegate: PageWirelessDevice {
                    objectName: "NetworkMain_PageWirelessDevice_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.VPNControlItem
                delegate: PageVPN {
                    objectName: "NetworkMain_PageVPN_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    name: "networkVpn"
                    parentName: "network"
                    weight: 3010
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.DSLControlItem
                delegate: PageDSL {
                    objectName: "NetworkMain_PageDSL_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    name: "dsl"
                    parentName: "network"
                    weight: 3020
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.HotspotControlItem
                delegate: PageHotspot {
                    objectName: "NetworkMain_PageHotspot_" + model.item.id
                    Accessible.role: Accessible.Grouping
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
                    objectName: "NetworkMain_PageSystemProxy_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    name: "systemProxy"
                    parentName: "network"
                    weight: 3050
                    Component.onCompleted: setNetItem(model.item)
                }
            }
            DelegateChoice {
                roleValue: NetType.AppProxyControlItem
                delegate: PageAppProxy {
                    objectName: "NetworkMain_PageAppProxy_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    name: "applicationProxy"
                    parentName: "network"
                    weight: 3060
                    netItem: model.item
                }
            }
            DelegateChoice {
                roleValue: NetType.DetailsItem
                delegate: PageDetails {
                    objectName: "NetworkMain_PageDetails_" + model.item.id
                    Accessible.role: Accessible.Grouping
                    name: "networkDetails"
                    parentName: "network"
                    weight: 3070
                    netItem: model.item
                }
            }
        }
    }
}
