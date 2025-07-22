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
        root.config = c !== undefined ? c : {}
        mppe = root.config.hasOwnProperty("require-mppe") && root.config["require-mppe"]
        root.configChanged()
    }
    function getConfig() {
        root.config["require-mppe"] = mppe
        if (!mppe) {
            delete root.config["require-mppe-128"]
            delete root.config["mppe-stateful"]
        }
        return root.config
    }
    function checkInput() {
        return true
    }
    name: "pppTitle"
    displayName: qsTr("PPP")
    canSearch: false
    Component {
        id: switchItem
        D.Switch {
            checked: root.config.hasOwnProperty(dccObj.name) && root.config[dccObj.name]
            onClicked: {
                root.config[dccObj.name] = checked
                root.editClicked()
            }
        }
    }
    DccObject {
        name: "pppGroup"
        parentName: root.parentName
        weight: root.weight + 20
        canSearch: false
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "useMPPE"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Use MPPE")
            canSearch: false
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
            canSearch: false
            weight: 20
            visible: mppe
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "mppe-stateful"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Stateful MPPE")
            canSearch: false
            weight: 30
            visible: mppe
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-eap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse EAP Authentication")
            canSearch: false
            weight: 40
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-pap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse PAP Authentication")
            canSearch: false
            weight: 50
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-chap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse CHAP Authentication")
            canSearch: false
            weight: 60
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-mschap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse MSCHAP Authentication")
            canSearch: false
            weight: 70
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-mschapv2"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse MSCHAPv2 Authentication")
            canSearch: false
            weight: 80
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nobsdcomp"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No BSD Data Compression")
            canSearch: false
            weight: 90
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nodeflate"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No Deflate Data Compression")
            canSearch: false
            weight: 100
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "no-vj-comp"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No TCP Header Compression")
            canSearch: false
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
            canSearch: false
            weight: 140
            pageType: DccObject.Editor
            page: D.Switch {
                checked: root.config["lcp-echo-failure"] === 5 && root.config["lcp-echo-interval"] === 30
                onClicked: {
                    if (checked) {
                        root.config["lcp-echo-failure"] = 5
                        root.config["lcp-echo-interval"] = 30
                    } else {
                        root.config["lcp-echo-failure"] = 0
                        root.config["lcp-echo-interval"] = 0
                    }
                    root.editClicked()
                }
            }
        }
    }
}
