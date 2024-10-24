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
    name: "pppTitle"
    displayName: qsTr("PPP")

    function setConfig(c) {
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
                onClicked: mppe = checked
            }
        }
        DccObject {
            name: "128bitMPPE"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("128-bit MPPE")
            weight: 20
            visible: mppe
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("require-mppe-128") && config["require-mppe-128"]
                onClicked: config["require-mppe-128"] = checked
            }
        }
        DccObject {
            name: "statefulMPPE"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Stateful MPPE")
            weight: 30
            visible: mppe
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("mppe-stateful") && config["mppe-stateful"]
                onClicked: config["mppe-stateful"] = checked
            }
        }
        DccObject {
            name: "refuseEap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse EAP Authentication")
            weight: 40
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("refuse-eap") && config["refuse-eap"]
                onClicked: config["refuse-eap"] = checked
            }
        }
        DccObject {
            name: "refusePap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse PAP Authentication")
            weight: 50
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("refuse-pap") && config["refuse-pap"]
                onClicked: config["refuse-pap"] = checked
            }
        }
        DccObject {
            name: "refuseChap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse CHAP Authentication")
            weight: 60
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("refuse-chap") && config["refuse-chap"]
                onClicked: config["refuse-chap"] = checked
            }
        }
        DccObject {
            name: "refuseMschap"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse MSCHAP Authentication")
            weight: 70
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("refuse-mschap") && config["refuse-mschap"]
                onClicked: config["refuse-mschap"] = checked
            }
        }
        DccObject {
            name: "refuseMschapv2"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("Refuse MSCHAPv2 Authentication")
            weight: 80
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("refuse-mschapv2") && config["refuse-mschapv2"]
                onClicked: config["refuse-mschapv2"] = checked
            }
        }
        DccObject {
            name: "nobsdcomp"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No BSD Data Compression")
            weight: 90
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("nobsdcomp") && config["nobsdcomp"]
                onClicked: config["nobsdcomp"] = checked
            }
        }
        DccObject {
            name: "nodeflate"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No Deflate Data Compression")
            weight: 100
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("nodeflate") && config["nodeflate"]
                onClicked: config["nodeflate"] = checked
            }
        }
        DccObject {
            name: "noVjComp"
            parentName: root.parentName + "/pppGroup"
            displayName: qsTr("No TCP Header Compression")
            weight: 110
            pageType: DccObject.Editor
            page: D.Switch {
                checked: config.hasOwnProperty("no-vj-comp") && config["no-vj-comp"]
                onClicked: config["no-vj-comp"] = checked
            }
        }
        // DccObject {
        //     name: "nopcomp"
        //     parentName: root.parentName + "/pppGroup"
        //     displayName: qsTr("No Protocol Field Compression")
        //     weight: 120
        //     pageType: DccObject.Editor
        //     page: D.Switch {
        //         checked: config.hasOwnProperty("nopcomp") && config["nopcomp"]
        //         onClicked: config["nopcomp"] = checked
        //     }
        // }
        // DccObject {
        //     name: "noaccomp"
        //     parentName: root.parentName + "/pppGroup"
        //     displayName: qsTr("No Address/Control Compression")
        //     weight: 130
        //     pageType: DccObject.Editor
        //     page: D.Switch {
        //         checked: config.hasOwnProperty("noaccomp") && config["noaccomp"]
        //         onClicked: config["noaccomp"] = checked
        //     }
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
                }
            }
        }
    }
}
