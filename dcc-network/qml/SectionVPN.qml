// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0
import "NetUtils.js" as NetUtils

DccTitleObject {
    id: root
    property var config: null
    property var dataMap: new Object
    property var secretMap: new Object
    property int vpnType: 1

    // 控件变化
    property bool mppe: false
    property string currentMppeMethod: "require-mppe"
    property bool ipsecEnabled: false
    property string currentEncryption: "secure"

    function setConfig(c) {
        config = c !== undefined ? c : {}
        dataMap = config.hasOwnProperty("data") ? dccData.toMap(config.data) : {}
        secretMap = config.hasOwnProperty("secrets") ? dccData.toMap(config.secrets) : {}

        let mppeList = ["require-mppe", "require-mppe-40", "require-mppe-128"]
        mppe = false
        currentMppeMethod = "require-mppe"
        for (let mppeMethod of mppeList) {
            if (dataMap.hasOwnProperty(mppeMethod) && dataMap[mppeMethod] === "yes") {
                mppe = true
                currentMppeMethod = mppeMethod
            }
        }
        ipsecEnabled = dataMap["ipsec-enabled"] === "yes"

        switch (vpnType) {
        case NetUtils.VpnTypeEnum["vpnc"]:
            if (dataMap["Enable no encryption"] === "yes") {
                currentEncryption = "none"
            } else if (dataMap["Enable Single DES"] === "yes") {
                currentEncryption = "weak"
            } else {
                currentEncryption = "secure"
            }
            break
            // case NetUtils.VpnTypeEnum["l2tp"]:
            // case NetUtils.VpnTypeEnum["pptp"]:
            //     gateway = dataMap.hasOwnProperty("gateway") ? dataMap["gateway"] : ""
            //     username = dataMap.hasOwnProperty("user") ? dataMap["user"] : ""
            //     pwdFlays = dataMap.hasOwnProperty("password-flags") ? dataMap["password-flags"] : 0
            //     password = secretMap.hasOwnProperty("password") ? secretMap["password"] : ""
            //     break
            // case NetUtils.VpnTypeEnum["openconnect"]:
            //     gateway = dataMap.hasOwnProperty("gateway") ? dataMap["gateway"] : ""
            //     break
            // case NetUtils.VpnTypeEnum["openvpn"]:
            //     gateway = dataMap.hasOwnProperty("remote") ? dataMap["remote"] : ""
            //     break
            // default:
            //     gateway = dataMap.hasOwnProperty("gateway") ? dataMap["gateway"] : ""
            //     username = dataMap.hasOwnProperty("user") ? dataMap["user"] : ""
            //     pwdFlays = dataMap.hasOwnProperty("password-flags") ? dataMap["password-flags"] : 0
            //     password = secretMap.hasOwnProperty("password") ? secretMap["password"] : ""
            //     break
        }
    }
    function getConfig() {
        dataMap["require-mppe"] = mppe ? "yes" : "no"
        if (!mppe) {
            delete config["require-mppe-128"]
            delete config["mppe-stateful"]
        }
        switch (vpnType) {
        case NetUtils.VpnTypeEnum["vpnc"]:
            switch (currentEncryption) {
            case "none":
                delete dataMap["Enable Single DES"]
                dataMap["Enable no encryption"] = "yes"
                break
            case "weak":
                delete dataMap["Enable no encryption"]
                dataMap["Enable Single DES"] = "yes"
                break
            case "secure":
                delete dataMap["Enable Single DES"]
                delete dataMap["Enable no encryption"]
                break
            }
            break
        }
        config.data = dccData.toStringMap(dataMap)
        config.secrets = dccData.toStringMap(secretMap)
        return config
    }
    function checkInput() {
        return true
    }

    name: "VPNTitle"
    displayName: qsTr("VPN")
    Component {
        id: fileDialog
        FileDialog {
            property var selectEdit: null
            visible: false
            nameFilters: [qsTr("All files (*)")]
            onAccepted: {
                selectEdit.text = currentFile.toString().replace("file://", "")
                this.destroy(10)
            }
            onRejected: this.destroy(10)
        }
    }
    Component {
        id: fileLineEdit
        RowLayout {
            D.LineEdit {
                id: fileEdit
                text: dataMap.hasOwnProperty(dccObj.name) ? NetUtils.removeTrailingNull(dataMap[dccObj.name]) : ""
                onTextChanged: {
                    dataMap[dccObj.name] = text
                }
            }
            NetButton {
                text: "..."
                onClicked: {
                    fileDialog.createObject(this, {
                                                "selectEdit": fileEdit
                                            }).open()
                }
            }
        }
    }
    Component {
        id: requiredFileLineEdit
        RowLayout {
            D.LineEdit {
                id: fileEdit
                placeholderText: qsTr("Required")
                text: dataMap.hasOwnProperty(dccObj.name) ? NetUtils.removeTrailingNull(dataMap[dccObj.name]) : ""
                onTextChanged: {
                    dataMap[dccObj.name] = text
                }
            }
            NetButton {
                text: "..."
                onClicked: {
                    fileDialog.createObject(this, {
                                                "selectEdit": fileEdit
                                            }).open()
                }
            }
        }
    }
    Component {
        id: requiredLineEdit
        D.LineEdit {
            placeholderText: qsTr("Required")
            text: dataMap.hasOwnProperty(dccObj.name) ? dataMap[dccObj.name] : ""
            onTextChanged: dataMap[dccObj.name] = text
        }
    }
    Component {
        id: lineEdit
        D.LineEdit {
            text: dataMap.hasOwnProperty(dccObj.name) ? dataMap[dccObj.name] : ""
            onTextChanged: dataMap[dccObj.name] = text
        }
    }
    Component {
        id: switchItem
        D.Switch {
            checked: dataMap.hasOwnProperty(dccObj.name) && dataMap[dccObj.name] === "yes"
            onClicked: dataMap[dccObj.name] = checked ? "yes" : "no"
        }
    }

    DccObject {
        name: "vpnGroup"
        parentName: root.parentName
        weight: root.weight + 20
        pageType: DccObject.Item
        page: DccGroupView {}
        // l2tp/pptp
        DccObject {
            name: "gateway"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Gateway")
            weight: 10
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"] | NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "cacert"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("CA Cert")
            weight: 20
            visible: vpnType & (NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: fileLineEdit
        }
        DccObject {
            name: "proxy"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Proxy")
            weight: 30
            visible: vpnType & (NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "enable_csd_trojan"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Allow Cisco Secure Desktop Trojan")
            weight: 40
            visible: vpnType & (NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "csd_wrapper"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("CSD Script")
            weight: 50
            visible: vpnType & (NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "address"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Gateway")
            weight: 60
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "certificate"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("CA Cert")
            weight: 70
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: fileLineEdit
        }
        DccObject {
            name: "method"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Auth Type")
            weight: 80
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Private Key"),
                        "value": "key"
                    }, {
                        "text": qsTr("SSH Agent"),
                        "value": "agent"
                    }, {
                        "text": qsTr("Smart Card"),
                        "value": "smartcard"
                    }, {
                        "text": qsTr("EAP"),
                        "value": "eap"
                    }, {
                        "text": qsTr("Pre-Shared Key"),
                        "value": "psk"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "usercert"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("User Cert")
            weight: 90
            visible: (vpnType & (NetUtils.VpnTypeEnum["openconnect"])) || ((vpnType | NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "key" || dataMap["method"] === "agent"))
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "userkey"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Private Key")
            weight: 100
            visible: (vpnType & (NetUtils.VpnTypeEnum["openconnect"])) || ((vpnType | NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "key"))
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "user"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Username")
            weight: 110
            visible: (vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])) || ((vpnType | NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "eap" || dataMap["method"] === "psk"))
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "remote"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Gateway")
            weight: 120
            visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "connection-type"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Auth Type")
            weight: 130
            visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Certificates (TLS)"),
                        "value": "tls"
                    }, {
                        "text": qsTr("Password"),
                        "value": "password"
                    }, {
                        "text": qsTr("Certificates with Password (TLS)"),
                        "value": "password-tls"
                    }, {
                        "text": qsTr("Static Key"),
                        "value": "static-key"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "ca"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("CA Cert")
            weight: 140
            visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"]) && (dataMap["connection-type"] === "tls" || dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "username"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Username")
            weight: 150
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "password-flags"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Pwd Options")
            weight: 160
            visible: (vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])) || (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Saved"),
                        "value": "0"
                    }, {
                        "text": qsTr("Ask"),
                        "value": "2"
                    }, {
                        "text": qsTr("Not Required"),
                        "value": "4"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
            }
        }
        DccObject {
            name: "password"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Password")
            weight: 170
            visible: ((vpnType | NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "eap" || dataMap["method"] === "psk")) || (dataMap["password-flags"] === "0" && ((vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])) || (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")))
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                placeholderText: qsTr("Required")
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextChanged: secretMap[dccObj.name] = text
            }
        }
        DccObject {
            name: "cert"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("User Cert")
            weight: 180
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: fileLineEdit
        }
        DccObject {
            name: "key"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Private Key")
            weight: 190
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: fileLineEdit
        }
        DccObject {
            name: "cert-pass-flags"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Pwd Options")
            weight: 200
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Saved"),
                        "value": "0"
                    }, {
                        "text": qsTr("Ask"),
                        "value": "2"
                    }, {
                        "text": qsTr("Not Required"),
                        "value": "4"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
            }
        }
        DccObject {
            name: "cert-pass"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Private Pwd")
            weight: 210
            visible: dataMap["cert-pass-flags"] === "0" && (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                placeholderText: qsTr("Required")
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextChanged: secretMap[dccObj.name] = text
            }
        }
        DccObject {
            name: "static-key"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Static Key")
            weight: 220
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "has_static-key-direction"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Customize Key Direction")
            weight: 230
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("static-key-direction")
                onClicked: {
                    if (checked) {
                        dataMap["static-key-direction"] = "0"
                    } else {
                        delete dataMap["static-key-direction"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "static-key-direction"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Key Direction")
            weight: 240
            visible: dataMap.hasOwnProperty(this.name) && (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("0"),
                        "value": "0"
                    }, {
                        "text": qsTr("1"),
                        "value": "1"
                    }]
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                Connections {
                    target: root
                    function onDataMapChanged() {
                        currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                    }
                }
            }
        }
        DccObject {
            name: "remote-ip"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Remote IP")
            weight: 250
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "local-ip"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Local IP")
            weight: 260
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "domain"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("NT Domain")
            weight: 270
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "virtual"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Request an Inner IP Address")
            weight: 280
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "encap"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Enforce UDP Encapsulation")
            weight: 290
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "ipcomp"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Use IP Compression")
            weight: 300
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "proposal"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Enable Custom Cipher Proposals")
            weight: 310
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty(dccObj.name) ? dataMap[dccObj.name] === "yes" : false
                onClicked: {
                    dataMap[dccObj.name] = checked ? "yes" : "no"
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "ike"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("IKE")
            weight: 320
            visible: (vpnType & (NetUtils.VpnTypeEnum["strongswan"])) && dataMap["proposal"] === "yes"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "esp"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("ESP")
            weight: 330
            visible: (vpnType & (NetUtils.VpnTypeEnum["strongswan"])) && dataMap["proposal"] === "yes"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "pem_passphrase_fsid"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Use FSID for Key Passphrase")
            weight: 340
            visible: vpnType & (NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: switchItem
        }
        // vpnc
        DccObject {
            name: "IPSec gateway"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Gateway")
            weight: 350
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "Xauth username"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Username")
            weight: 360
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "Xauth password-flags"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Pwd Options")
            weight: 370
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Saved"),
                        "value": "0"
                    }, {
                        "text": qsTr("Ask"),
                        "value": "2"
                    }, {
                        "text": qsTr("Not Required"),
                        "value": "4"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    switch (currentValue) {
                    case "0":
                        dataMap["xauth-password-type"] = "save"
                        break
                    case "1":
                        dataMap["xauth-password-type"] = "ask"
                        break
                    case "2":
                        dataMap["xauth-password-type"] = "unused"
                        break
                    }
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
            }
        }
        DccObject {
            name: "Xauth password"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Password")
            weight: 380
            visible: dataMap["Xauth password-flags"] === "0" && (vpnType & (NetUtils.VpnTypeEnum["vpnc"]))
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                placeholderText: qsTr("Required")
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextChanged: secretMap[dccObj.name] = text
            }
        }
        DccObject {
            name: "IPSec ID"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Group Name")
            weight: 390
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "IPSec secret-flags"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Pwd Options")
            weight: 400
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Saved"),
                        "value": "0"
                    }, {
                        "text": qsTr("Ask"),
                        "value": "2"
                    }, {
                        "text": qsTr("Not Required"),
                        "value": "4"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    switch (currentValue) {
                    case "0":
                        dataMap["ipsec-secret-type"] = "save"
                        break
                    case "1":
                        dataMap["ipsec-secret-type"] = "ask"
                        break
                    case "2":
                        dataMap["ipsec-secret-type"] = "unused"
                        break
                    }
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 2
            }
        }
        DccObject {
            name: "IPSec secret"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Group Pwd")
            weight: 410
            visible: dataMap["IPSec secret-flags"] === "0" && (vpnType & (NetUtils.VpnTypeEnum["vpnc"]))
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                placeholderText: qsTr("Required")
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextChanged: secretMap[dccObj.name] = text
            }
        }
        DccObject {
            name: "IKE Authmode"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Use Hybrid Authentication")
            weight: 420
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty(dccObj.name) && dataMap[dccObj.name] === "hybrid"
                onClicked: {
                    if (checked) {
                        dataMap[dccObj.name] = "hybrid"
                    } else {
                        delete dataMap[dccObj.name]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "CA-File"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("CA File")
            weight: 430
            visible: (vpnType & (NetUtils.VpnTypeEnum["vpnc"])) && dataMap["IKE Authmode"] === "hybrid"
            pageType: DccObject.Editor
            page: fileLineEdit
        }
    }

    // VPN PPP
    DccTitleObject {
        name: "vpnPPPTitle"
        parentName: root.parentName
        displayName: qsTr("VPN PPP")
        weight: root.weight + 100
        visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])
    }
    DccObject {
        name: "vpnPPPMPPEGroup"
        parentName: root.parentName
        weight: root.weight + 110
        visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "useMPPE"
            parentName: root.parentName + "/vpnPPPMPPEGroup"
            displayName: qsTr("Use MPPE")
            weight: 10
            pageType: DccObject.Editor
            page: D.Switch {
                checked: mppe
                onClicked: mppe = checked
            }
        }
        DccObject {
            name: "security"
            parentName: root.parentName + "/vpnPPPMPPEGroup"
            displayName: qsTr("Security")
            weight: 20
            visible: mppe
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: indexOfValue(currentMppeMethod)
                model: [{
                        "text": qsTr("All Available (default)"),
                        "value": "require-mppe"
                    }, {
                        "text": qsTr("40-bit (less secure)"),
                        "value": "require-mppe-40"
                    }, {
                        "text": qsTr("128-bit (most secure)"),
                        "value": "require-mppe-128"
                    }]
                onActivated: currentMppeMethod = currentValue
                Component.onCompleted: currentIndex = indexOfValue(currentMppeMethod)
            }
        }
        DccObject {
            name: "statefulMPPE"
            parentName: root.parentName + "/vpnPPPMPPEGroup"
            displayName: qsTr("Stateful MPPE")
            weight: 30
            visible: mppe
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("mppe-stateful") && dataMap["mppe-stateful"] === "yes"
                onClicked: dataMap["mppe-stateful"] = checked ? "yes" : "no"
            }
        }
    }
    DccObject {
        name: "vpnPPPGroup"
        parentName: root.parentName
        weight: root.weight + 120
        visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "refuseEap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse EAP Authentication")
            weight: 40
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("refuse-eap") && dataMap["refuse-eap"] === "yes"
                onClicked: dataMap["refuse-eap"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "refusePap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse PAP Authentication")
            weight: 50
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("refuse-pap") && dataMap["refuse-pap"] === "yes"
                onClicked: dataMap["refuse-pap"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "refuseChap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse CHAP Authentication")
            weight: 60
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("refuse-chap") && dataMap["refuse-chap"] === "yes"
                onClicked: dataMap["refuse-chap"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "refuseMschap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse MSCHAP Authentication")
            weight: 70
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("refuse-mschap") && dataMap["refuse-mschap"] === "yes"
                onClicked: dataMap["refuse-mschap"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "refuseMschapv2"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse MSCHAPv2 Authentication")
            weight: 80
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("refuse-mschapv2") && dataMap["refuse-mschapv2"] === "yes"
                onClicked: dataMap["refuse-mschapv2"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "nobsdcomp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No BSD Data Compression")
            weight: 90
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("nobsdcomp") && dataMap["nobsdcomp"] === "yes"
                onClicked: dataMap["nobsdcomp"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "nodeflate"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No Deflate Data Compression")
            weight: 100
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("nodeflate") && dataMap["nodeflate"] === "yes"
                onClicked: dataMap["nodeflate"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "noVjComp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No TCP Header Compression")
            weight: 110
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("no-vj-comp") && dataMap["no-vj-comp"] === "yes"
                onClicked: dataMap["no-vj-comp"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "nopcomp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No Protocol Field Compression")
            weight: 120
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"])
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("nopcomp") && dataMap["nopcomp"] === "yes"
                onClicked: dataMap["nopcomp"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "noaccomp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No Address/Control Compression")
            weight: 130
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"])
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("noaccomp") && dataMap["noaccomp"] === "yes"
                onClicked: dataMap["noaccomp"] = checked ? "yes" : "no"
            }
        }
        DccObject {
            name: "lcpEchoInterval"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Send PPP Echo Packets")
            weight: 140
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap["lcp-echo-failure"] === "5" && dataMap["lcp-echo-interval"] === "30"
                onClicked: {
                    if (checked) {
                        dataMap["lcp-echo-failure"] = "5"
                        dataMap["lcp-echo-interval"] = "30"
                    } else {
                        dataMap["lcp-echo-failure"] = "0"
                        dataMap["lcp-echo-interval"] = "0"
                    }
                }
            }
        }
    }
    // VPN IPsec
    DccTitleObject {
        name: "vpnIPsecTitle"
        parentName: root.parentName
        displayName: qsTr("VPN IPsec")
        weight: root.weight + 200
        visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"])
    }
    DccObject {
        name: "vpnIPsecGroup"
        parentName: root.parentName
        weight: root.weight + 210
        visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "ipsecEnabled"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Enable IPsec")
            weight: 10
            pageType: DccObject.Editor
            page: D.Switch {
                checked: ipsecEnabled
                onClicked: ipsecEnabled = checked
            }
        }
        DccObject {
            name: "ipsecGroupName"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Group Name")
            weight: 20
            visible: ipsecEnabled
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("ipsec-group-name") ? dataMap["ipsec-group-name"] : ""
                onTextChanged: dataMap["ipsec-group-name"] = text
            }
        }
        DccObject {
            name: "ipsecGatewayId"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Group ID")
            weight: 30
            visible: ipsecEnabled
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("ipsec-gateway-id") ? dataMap["ipsec-gateway-id"] : ""
                onTextChanged: dataMap["ipsec-gateway-id"] = text
            }
        }
        DccObject {
            name: "ipsecPsk"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Pre-Shared Key")
            weight: 40
            visible: ipsecEnabled
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("ipsec-psk") ? dataMap["ipsec-psk"] : ""
                onTextChanged: dataMap["ipsec-psk"] = text
            }
        }
        DccObject {
            name: "ipsecIke"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Phase1 Algorithms")
            weight: 50
            visible: ipsecEnabled
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("ipsec-ike") ? dataMap["ipsec-ike"] : ""
                onTextChanged: dataMap["ipsec-ike"] = text
            }
        }
        DccObject {
            name: "ipsecEsp"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Phase2 Algorithms")
            weight: 60
            visible: ipsecEnabled
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("ipsec-esp") ? dataMap["ipsec-esp"] : ""
                onTextChanged: dataMap["ipsec-esp"] = text
            }
        }
    }
    // VPN Advanced
    DccTitleObject {
        name: "vpnAdvancedTitle"
        parentName: root.parentName
        displayName: qsTr("VPN Advanced")
        weight: root.weight + 300
        visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
    }
    DccObject {
        name: "vpnAdvancedGroup"
        parentName: root.parentName
        weight: root.weight + 310
        visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "has_port"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Customize Gateway Port")
            weight: 10
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("port")
                onClicked: {
                    if (checked) {
                        dataMap["port"] = "1194"
                    } else {
                        delete dataMap["port"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "port"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Gateway Port")
            weight: 20
            visible: dataMap.hasOwnProperty("port")
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 1194
                from: 0
                to: 65535
                onValueChanged: dataMap[dccObj.name] = value
            }
        }
        DccObject {
            name: "has_reneg-seconds"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Customize Renegotiation Interval")
            weight: 30
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("reneg-seconds")
                onClicked: {
                    if (checked) {
                        dataMap["reneg-seconds"] = "0"
                    } else {
                        delete dataMap["reneg-seconds"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "reneg-seconds"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Renegotiation Interval")
            weight: 40
            visible: dataMap.hasOwnProperty("reneg-seconds")
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 0
                from: 0
                to: 65535
                onValueChanged: dataMap[dccObj.name] = value
            }
        }
        DccObject {
            name: "comp-lzo"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Use LZO Data Compression")
            weight: 50
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap["comp-lzo"] === "yes" || dataMap["comp-lzo"] === "adaptive"
                onClicked: {
                    if (checked) {
                        dataMap["comp-lzo"] = "yes"
                    } else {
                        delete dataMap["comp-lzo"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "proto-tcp"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Use TCP Connection")
            weight: 60
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "devType"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Use TAP Device")
            weight: 70
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("dev-type") && dataMap["dev-type"] === "tap"
                onClicked: {
                    if (checked) {
                        dataMap["dev-type"] = "tap"
                    } else {
                        delete dataMap["dev-type"]
                    }
                }
            }
        }
        DccObject {
            name: "has_tunnel-mtu"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Customize Tunnel MTU")
            weight: 80
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("tunnel-mtu")
                onClicked: {
                    if (checked) {
                        dataMap["tunnel-mtu"] = "1500"
                    } else {
                        delete dataMap["tunnel-mtu"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "tunnel-mtu"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("MTU")
            weight: 90
            visible: dataMap.hasOwnProperty("tunnel-mtu")
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 1500
                from: 0
                to: 65535
                onValueChanged: dataMap[dccObj.name] = value
            }
        }
        DccObject {
            name: "has_fragment-size"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Customize UDP Fragment Size")
            weight: 100
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("fragment-size")
                onClicked: {
                    if (checked) {
                        dataMap["fragment-size"] = "1300"
                    } else {
                        delete dataMap["fragment-size"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "fragment-size"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("UDP Fragment Size")
            weight: 110
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 1300
                from: 0
                to: 65535
                onValueChanged: dataMap[dccObj.name] = value
            }
        }
        DccObject {
            name: "mssfix"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Restrict Tunnel TCP MSS")
            weight: 120
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "remote-random"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Randomize Remote Hosts")
            weight: 130
            pageType: DccObject.Editor
            page: switchItem
        }
    }
    // VPN Advanced vpn vpnc
    DccTitleObject {
        name: "vpncAdvancedTitle"
        parentName: root.parentName
        displayName: qsTr("VPN Advanced")
        weight: root.weight + 350
        visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
    }
    DccObject {
        name: "vpncAdvancedGroup"
        parentName: root.parentName
        weight: root.weight + 360
        visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "Domain"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Domain")
            weight: 10
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "Vendor"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Vendor")
            weight: 20
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Cisco (default)"),
                        "value": "cisco"
                    }, {
                        "text": qsTr("Netscreen"),
                        "value": "netscreen"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: dataMap[dccObj.name] = currentValue
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "Application Version"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Version")
            weight: 30
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "encryption"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Encryption")
            weight: 40
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Secure (default)"),
                        "value": "secure"
                    }, {
                        "text": qsTr("Weak"),
                        "value": "weak"
                    }, {
                        "text": qsTr("None"),
                        "value": "none"
                    }]
                currentIndex: indexOfValue(currentEncryption)
                onActivated: currentEncryption = currentValue
                Component.onCompleted: currentIndex = indexOfValue(currentEncryption)
            }
        }
        DccObject {
            name: "NAT Traversal Mode"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("NAT Traversal Mode")
            weight: 50
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("NAT-T When Available (default)"),
                        "value": "natt"
                    }, {
                        "text": qsTr("NAT-T Always"),
                        "value": "force-natt"
                    }, {
                        "text": qsTr("Cisco UDP"),
                        "value": "cisco-udp"
                    }, {
                        "text": qsTr("Disabled"),
                        "value": "none"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: dataMap[dccObj.name] = currentValue
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "IKE DH Group"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("IKE DH Group")
            weight: 60
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("DH Group 1"),
                        "value": "dh1"
                    }, {
                        "text": qsTr("DH Group 2 (default)"),
                        "value": "dh2"
                    }, {
                        "text": qsTr("DH Group 5"),
                        "value": "dh5"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: dataMap[dccObj.name] = currentValue
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "Perfect Forward Secrecy"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Forward Secrecy")
            weight: 70
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Server (default)"),
                        "value": "server"
                    }, {
                        "text": qsTr("None"),
                        "value": "nopfs"
                    }, {
                        "text": qsTr("DH Group 1"),
                        "value": "dh1"
                    }, {
                        "text": qsTr("DH Group 2"),
                        "value": "dh2"
                    }, {
                        "text": qsTr("DH Group 5"),
                        "value": "dh5"
                    }]
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: dataMap[dccObj.name] = currentValue
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "Local Port"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Local Port")
            weight: 80
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 0
                from: 0
                to: 65535
                onValueChanged: dataMap[dccObj.name] = value
            }
        }
        DccObject {
            name: "DPD idle timeout (our side)"
            parentName: root.parentName + "/vpncAdvancedGroup"
            displayName: qsTr("Disable Dead Peer Detection")
            weight: 90
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap[dccObj.name] === "0"
                onClicked: {
                    if (checked) {
                        dataMap[dccObj.name] = "0"
                    } else {
                        delete dataMap[dccObj.name]
                    }
                }
            }
        }
    }
    // VPN Security
    DccTitleObject {
        name: "vpnSecurityTitle"
        parentName: root.parentName
        displayName: qsTr("VPN Security")
        weight: root.weight + 400
        visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
    }
    DccObject {
        name: "vpnSecurityGroup"
        parentName: root.parentName
        weight: root.weight + 410
        visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "cipher"
            parentName: root.parentName + "/vpnSecurityGroup"
            displayName: qsTr("Cipher")
            weight: 10
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: dataMap.hasOwnProperty("cipher") ? indexOfValue(dataMap["cipher"]) : 0
                model: [{
                        "text": qsTr("Default"),
                        "value": "default"
                    }, {
                        "text": qsTr("None"),
                        "value": "none"
                    }, {
                        "text": qsTr("DES-CBC"),
                        "value": "DES-CBC"
                    }, {
                        "text": qsTr("RC2-CBC"),
                        "value": "RC2-CBC"
                    }, {
                        "text": qsTr("DES-EDE-CBC"),
                        "value": "DES-EDE-CBC"
                    }, {
                        "text": qsTr("DES-EDE3-CBC"),
                        "value": "DES-EDE3-CBC"
                    }, {
                        "text": qsTr("DESX-CBC"),
                        "value": "DESX-CBC"
                    }, {
                        "text": qsTr("BF-CBC"),
                        "value": "BF-CBC"
                    }, {
                        "text": qsTr("RC2-40-CBC"),
                        "value": "RC2-40-CBC"
                    }, {
                        "text": qsTr("CAST5-CBC"),
                        "value": "CAST5-CBC"
                    }, {
                        "text": qsTr("RC2-64-CBC"),
                        "value": "RC2-64-CBC"
                    }, {
                        "text": qsTr("AES-128-CBC"),
                        "value": "AES-128-CBC"
                    }, {
                        "text": qsTr("AES-192-CBC"),
                        "value": "AES-192-CBC"
                    }, {
                        "text": qsTr("AES-256-CBC"),
                        "value": "AES-256-CBC"
                    }, {
                        "text": qsTr("CAMELLIA-128-CBC"),
                        "value": "CAMELLIA-128-CBC"
                    }, {
                        "text": qsTr("CAMELLIA-192-CBC"),
                        "value": "CAMELLIA-192-CBC"
                    }, {
                        "text": qsTr("CAMELLIA-256-CBC"),
                        "value": "CAMELLIA-256-CBC"
                    }, {
                        "text": qsTr("SEED-CBC"),
                        "value": "SEED-CBC"
                    }]
                onActivated: dataMap["cipher"] = currentValue
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty("cipher") ? indexOfValue(dataMap["cipher"]) : 0
            }
        }
        DccObject {
            name: "auth"
            parentName: root.parentName + "/vpnSecurityGroup"
            displayName: qsTr("HMAC Auth")
            weight: 20
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: dataMap.hasOwnProperty("auth") ? indexOfValue(dataMap["auth"]) : 0
                model: [{
                        "text": qsTr("Default"),
                        "value": "default"
                    }, {
                        "text": qsTr("None"),
                        "value": "none"
                    }, {
                        "text": qsTr("RSA MD-4"),
                        "value": "RSA-MD4"
                    }, {
                        "text": qsTr("MD-5"),
                        "value": "MD5"
                    }, {
                        "text": qsTr("SHA-1"),
                        "value": "SHA1"
                    }, {
                        "text": qsTr("SHA-224"),
                        "value": "SHA224"
                    }, {
                        "text": qsTr("SHA-256"),
                        "value": "SHA256"
                    }, {
                        "text": qsTr("SHA-384"),
                        "value": "SHA384"
                    }, {
                        "text": qsTr("SHA-512"),
                        "value": "SHA512"
                    }, {
                        "text": qsTr("RIPEMD-160"),
                        "value": "RIPEMD160"
                    }]
                onActivated: dataMap["auth"] = currentValue
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty("auth") ? indexOfValue(dataMap["auth"]) : 0
            }
        }
    }
    // VPN Proxy
    DccTitleObject {
        name: "vpnProxyTitle"
        parentName: root.parentName
        displayName: qsTr("VPN Proxy")
        weight: root.weight + 500
        visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
    }
    DccObject {
        name: "vpnProxyGroup"
        parentName: root.parentName
        weight: root.weight + 510
        visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "proxy-type"
            parentName: root.parentName + "/vpnProxyGroup"
            displayName: qsTr("Proxy Type")
            weight: 10
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: dataMap.hasOwnProperty("proxy-type") ? indexOfValue(dataMap["proxy-type"]) : 0
                model: [{
                        "text": qsTr("Not Required"),
                        "value": "none"
                    }, {
                        "text": qsTr("HTTP"),
                        "value": "http"
                    }, {
                        "text": qsTr("SOCKS"),
                        "value": "socks"
                    }]
                onActivated: {
                    dataMap["proxy-type"] = currentValue
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty("proxy-type") ? indexOfValue(dataMap["proxy-type"]) : 0
            }
        }
        DccObject {
            name: "proxy-server"
            parentName: root.parentName + "/vpnProxyGroup"
            displayName: qsTr("Server IP")
            weight: 20
            visible: dataMap["proxy-type"] !== "none"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "proxy-port"
            parentName: root.parentName + "/vpnProxyGroup"
            displayName: qsTr("Port")
            weight: 30
            visible: dataMap["proxy-type"] !== "none"
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 1300
                from: 0
                to: 65535
                onValueChanged: dataMap[dccObj.name] = value
            }
        }
        DccObject {
            name: "proxy-retry"
            parentName: root.parentName + "/vpnProxyGroup"
            displayName: qsTr("Retry Indefinitely When Failed")
            weight: 40
            visible: dataMap["proxy-type"] !== "none"
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "http-proxy-username"
            parentName: root.parentName + "/vpnProxyGroup"
            displayName: qsTr("Username")
            weight: 50
            visible: dataMap["proxy-type"] === "http"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "http-proxy-password"
            parentName: root.parentName + "/vpnProxyGroup"
            displayName: qsTr("Password")
            weight: 60
            visible: dataMap["proxy-type"] === "http"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
    }
    // VPN TLS Authentication
    DccTitleObject {
        name: "vpnTLSAuthenticationTitle"
        parentName: root.parentName
        displayName: qsTr("VPN TLS Authentication")
        weight: 2000
        visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] !== "static-key")
    }
    DccObject {
        name: "vpnTLSAuthenticationGroup"
        parentName: root.parentName
        weight: 2010
        visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] !== "static-key")
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "tls-remote"
            parentName: root.parentName + "/vpnTLSAuthenticationGroup"
            displayName: qsTr("Subject Match")
            weight: 10
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("tls-remote") && dataMap["tls-remote"]
                onTextChanged: dataMap["tls-remote"] = text
            }
        }
        DccObject {
            name: "remote-cert-tls"
            parentName: root.parentName + "/vpnTLSAuthenticationGroup"
            displayName: qsTr("Remote Cert Type")
            weight: 20
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: dataMap[dccObj.name] = currentValue
                model: [{
                        "text": qsTr("Default"),
                        "value": "default"
                    }, {
                        "text": qsTr("Client"),
                        "value": "client"
                    }, {
                        "text": qsTr("Server"),
                        "value": "server"
                    }]
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "ta"
            parentName: root.parentName + "/vpnTLSAuthenticationGroup"
            displayName: qsTr("Key File")
            weight: 30
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: dataMap.hasOwnProperty("ta") && dataMap["ta"]
                onTextChanged: dataMap["ta"] = text
            }
        }
        DccObject {
            name: "has_ta-dir"
            parentName: root.parentName + "/vpnTLSAuthenticationGroup"
            displayName: qsTr("Customize Key Direction")
            weight: 40
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dataMap.hasOwnProperty("ta-dir")
                onClicked: {
                    if (checked) {
                        dataMap["ta-dir"] = "0"
                    } else {
                        delete dataMap["ta-dir"]
                    }
                    dataMapChanged()
                }
            }
        }
        DccObject {
            name: "ta-dir"
            parentName: root.parentName + "/vpnTLSAuthenticationGroup"
            displayName: qsTr("Key Direction")
            weight: 50
            visible: dataMap.hasOwnProperty(this.name)
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: dataMap[dccObj.name] = currentValue
                model: [{
                        "text": qsTr("0"),
                        "value": "0"
                    }, {
                        "text": qsTr("1"),
                        "value": "1"
                    }]
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                Connections {
                    target: root
                    function onDataMapChanged() {
                        currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                    }
                }
            }
        }
    }
}
