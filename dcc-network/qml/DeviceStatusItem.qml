// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

RowLayout {
    id: root
    property var netItem: null
    property bool connectedNameVisible: false
    property alias statusVisible: loader.active

    function getStatusName(status) {
        switch (status) {
        case NetType.DS_Disabled:
            return qsTr("Off")
        case NetType.DS_Connected:
        case NetType.DS_ConnectNoInternet:
            return qsTr("Connected")
        case NetType.DS_IpConflicted:
            return qsTr("IP conflict")
        case NetType.DS_Connecting:
            return qsTr("Connecting")
        case NetType.DS_ObtainingIP:
            return qsTr("Obtaining address")
        case NetType.DS_Authenticating:
            return qsTr("Authenticating")
        case NetType.DS_ObtainIpFailed:
        case NetType.DS_ConnectFailed:
        case NetType.DS_Unknown:
        case NetType.DS_Enabled:
        case NetType.DS_NoCable:
        case NetType.DS_Disconnected:
        default:
            return qsTr("Disconnected")
        }
    }
    Loader {
        id: loader
        sourceComponent: DccLabel {
            text: {
                if (connectedNameVisible) {
                    switch (netItem.status) {
                    case NetType.DS_Connected:
                    case NetType.DS_ConnectNoInternet:
                        var childrenItem = [netItem]
                        while (childrenItem.length > 0) {
                            var childItem = childrenItem.pop()
                            if (childItem.status === NetType.CS_Connected) {
                                return childItem.name
                            }
                            for (let i in childItem.children) {
                                childrenItem.push(childItem.children[i])
                            }
                        }
                        break
                    default:
                        break
                    }
                }
                return getStatusName(netItem.status)
            }
        }
    }
    D.Switch {
        checked: root.netItem.isEnabled
        enabled: root.netItem.enabledable
        onClicked: {
            dccData.exec(root.netItem.isEnabled ? NetManager.DisabledDevice : NetManager.EnabledDevice, root.netItem.id, {})
        }
    }
}
