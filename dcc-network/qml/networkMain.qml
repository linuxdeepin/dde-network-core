// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccObject {
    id: root
    property var wiredDevs: []
    property var wirelessDevs: []

    Component {
        id: wiredComponent
        PageWiredDevice {}
    }
    Component {
        id: wirelessComponent
        PageWirelessDevice {}
    }

    PageVPN {
        id: vpnPage
        name: "networkVpn"
        parentName: "network"
        weight: 3010
    }
    PageSystemProxy {
        id: systemProxyPage
        name: "systemProxy"
        parentName: "network"
        weight: 3020
    }
    PageAppProxy {
        id: appProxyPage
        name: "applicationProxy"
        parentName: "network"
        weight: 3030
    }
    PageHotspot {
        id: hotspotPage
        name: "personalHotspot"
        parentName: "network"
        weight: 3040
    }
    PageAirplane {
        name: "airplaneMode"
        parentName: "network"
        weight: 3050
        item: dccData.root
    }
    PageDSL {
        id: dslPage
        name: "dsl"
        parentName: "network"
        weight: 3060
    }
    PageDetails {
        id: detailsPage
        name: "networkDetails"
        parentName: "network"
        weight: 3070
    }
    function updateDevice() {
        const delWiredDevs = wiredDevs.concat()
        const delWirelessDevs = wirelessDevs.concat()
        for (let i in dccData.root.children) {
            let item = dccData.root.children[i]
            switch (item.itemType) {
            case NetType.WiredDeviceItem:
            {
                let index = delWiredDevs.findIndex(d => d.item === item)
                if (index >= 0) {
                    delWiredDevs.splice(index, 1)
                } else {
                    let dev = wiredComponent.createObject(root, {
                                                              "item": item
                                                          })
                    DccApp.addObject(dev)
                    wiredDevs.push(dev)
                }
            }
            break
            case NetType.WirelessDeviceItem:
            {
                let index = delWirelessDevs.findIndex(d => d.item === item)
                if (index >= 0) {
                    delWirelessDevs.splice(index, 1)
                } else {
                    let dev = wirelessComponent.createObject(root, {
                                                                 "item": item,
                                                                 "airplaneItem": dccData.root
                                                             })
                    DccApp.addObject(dev)
                    wirelessDevs.push(dev)
                }
            }
            break
            case NetType.VPNControlItem:
                if (vpnPage.item !== item) {
                    vpnPage.item = item
                }
                break
            case NetType.SystemProxyControlItem:
                if (systemProxyPage.item !== item) {
                    systemProxyPage.setItem(item)
                }
                break
            case NetType.AppProxyControlItem:
                if (appProxyPage.item !== item) {
                    appProxyPage.item = item
                }
                break
            case NetType.HotspotControlItem:
                hotspotPage.setItem(item)
                break
            case NetType.DSLControlItem:
                if (dslPage.item !== item) {
                    dslPage.item = item
                }
                break
            case NetType.DetailsItem:
                if (detailsPage.item !== item) {
                    detailsPage.item = item
                }
                break
            }
        }
        for (const delDev of delWiredDevs) {
            DccApp.removeObject(delDev)
            let index = wiredDevs.findIndex(item => delDev === item)
            if (index >= 0) {
                wiredDevs.splice(index, 1)
            }
            delDev.destroy()
        }
        for (const delDev of delWirelessDevs) {
            DccApp.removeObject(delDev)
            let index = wirelessDevs.findIndex(item => delDev === item)
            if (index >= 0) {
                wirelessDevs.splice(index, 1)
            }
            delDev.destroy()
        }
    }
    Connections {
        target: dccData.root
        function onChildrenChanged() {
            updateDevice()
        }
    }
    Component.onCompleted: {
        updateDevice()
    }
}
