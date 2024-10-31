// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccTitleObject {
    id: root
    property var config: null
    property bool mppe: false

    property string errorKey: ""
    signal editClicked

    function setConfig(c) {
        errorKey = ""
        config = c !== undefined ? c : {}
        mppe = config.hasOwnProperty("require-mppe") && config["require-mppe"]
        root.configChanged()
    }
    function getConfig() {
        config["require-mppe"] = mppe
        if (!mppe) {
            delete config["require-mppe-128"]
            delete config["mppe-stateful"]
        }
        return config
    }
    function checkInput() {
        return true
    }
    name: "pppTitle"
    displayName: qsTr("PPP")
    Component {
        id: switchItem
        D.Switch {
            checked: config.hasOwnProperty(dccObj.name) && config[dccObj.name]
            onClicked: {
                config[dccObj.name] = checked
                root.editClicked()
            }
        }
    }
    DccObject {
        name: "pppGroup"
        parentName: root.parentName
        weight: root.weight + 20
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "useMPPE"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Use MPPE")
            weight: 10
            pageType: DccObject.Editor
            page: D.Switch {
                checked: mppe
                onClicked: {
                    mppe = checked
                    root.editClicked()
                }
            }
        }
        DccObject {
            name: "require-mppe-128"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("128-bit MPPE")
            weight: 20
            visible: mppe
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "mppe-stateful"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Stateful MPPE")
            weight: 30
            visible: mppe
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-eap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse EAP Authentication")
            weight: 40
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-pap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse PAP Authentication")
            weight: 50
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-chap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse CHAP Authentication")
            weight: 60
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-mschap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse MSCHAP Authentication")
            weight: 70
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-mschapv2"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse MSCHAPv2 Authentication")
            weight: 80
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nobsdcomp"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No BSD Data Compression")
            weight: 90
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nodeflate"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No Deflate Data Compression")
            weight: 100
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "no-vj-comp"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No TCP Header Compression")
            weight: 110
            pageType: DccObject.Editor
            page: switchItem
        }
        // DccObject {
        //     name: "nopcomp"
        //     parentName: root.parentName + "/pppGroup"
        //     displayName: qsTr("No Protocol Field Compression")
        //     weight: 120
        //     pageType: DccObject.Editor
        //     page: switchItem
        // }
        // DccObject {
        //     name: "noaccomp"
        //     parentName: root.parentName + "/pppGroup"
        //     displayName: qsTr("No Address/Control Compression")
        //     weight: 130
        //     pageType: DccObject.Editor
        //     page:switchItem
        // }
        DccObject {
            name: "lcpEchoInterval"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Send PPP Echo Packets")
            weight: 140
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config["lcp-echo-failure"] === 5 && config["lcp-echo-interval"] === 30
                onClicked: {
                    if (checked) {
                        config["lcp-echo-failure"] = 5
                        config["lcp-echo-interval"] = 30
                    } else {
                        config["lcp-echo-failure"] = 0
                        config["lcp-echo-interval"] = 0
                    }
                    root.editClicked()
                }
            }
        }
    }
}
