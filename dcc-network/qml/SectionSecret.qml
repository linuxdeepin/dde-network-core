// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0
import org.deepin.dcc.network 1.0

DccTitleObject {
    id: root
    property var configWSecurity: undefined
    property var config802_1x: undefined
    property string parentName: ""
    property int type: NetType.WiredItem
    property string eapType: "peap"
    property int pwdFlays: 0
    property string pwdStr: ""
    property string keyMgmt: ""

    function setConfigWSecurity(c) {
        if (c) {
            configWSecurity = c
            keyMgmt = configWSecurity.hasOwnProperty("key-mgmt") ? configWSecurity["key-mgmt"] : ""
        } else {
            configWSecurity = {}
            keyMgmt = ""
        }
        switch (keyMgmt) {
        case "none":
            pwdStr = configWSecurity.hasOwnProperty("wep-key0") ? configWSecurity["wep-key0"] : ""
            pwdFlays = configWSecurity.hasOwnProperty("wep-key-type") ? configWSecurity["wep-key-type"] : 0
            break
        case "wpa-psk":
        case "sae":
            pwdStr = configWSecurity.hasOwnProperty("psk") ? configWSecurity["psk"] : ""
            pwdFlays = configWSecurity.hasOwnProperty("psk-flags") ? configWSecurity["psk-flags"] : 0
            break
        }
    }
    function getConfigWSecurity(c) {
        let config = {}
        switch (keyMgmt) {
        case "none":
            config["key-mgmt"] = keyMgmt
            config["wep-key0"] = pwdStr
            config["wep-key-type"] = pwdFlays
            config["auth-alg"] = configWSecurity["auth-alg"]
            break
        case "wpa-psk":
        case "sae":
            config["key-mgmt"] = keyMgmt
            config["psk"] = pwdStr
            config["psk-flags"] = pwdFlays
            break
        case "wpa-eap":
            config["key-mgmt"] = keyMgmt
            break
        default:
            return undefined
        }
        return config
    }

    function setConfig802_1x(c) {
        if (keyMgmt === "" && c !== undefined) {
            keyMgmt = "wpa-eap"
        }
        switch (keyMgmt) {
        case "wpa-eap":
            config802_1x = c
            if (config802_1x && config802_1x.hasOwnProperty("eap") && config802_1x.eap.length > 0) {
                eapType = config802_1x["eap"][0]
                console.log("sss===eapType=", eapType)
            }
            switch (eapType) {
            case "tls":
                pwdStr = config802_1x.hasOwnProperty("private-key-password") ? config802_1x["private-key-password"] : ""
                pwdFlays = config802_1x.hasOwnProperty("private-key-password-flags") ? config802_1x["private-key-password-flags"] : 0
                break
            case "md5":
            case "leap":
            case "fast":
            case "ttls":
            case "peap":
                pwdStr = config802_1x.hasOwnProperty("password") ? config802_1x["password"] : ""
                pwdFlays = config802_1x.hasOwnProperty("password-flags") ? config802_1x["password-flags"] : 0
                break
            }
            break
        case "":
            // 无密码
            config802_1x = {}
            eapType = "tls"
            pwdStr = ""
            pwdFlays = 0
            break
        default:
            config802_1x = {}
            eapType = "tls"
            break
        }

        for (var k in config802_1x) {
            console.log("sss===ssss=", k, config802_1x[k])
        }
    }
    function getConfig802_1x() {
        let saveConfig = {}
        if (keyMgmt === "wpa-eap") {
            saveConfig.eap = [eapType]
            switch (eapType) {
            case "tls":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["private-key-password-flags"] = pwdFlays
                saveConfig["private-key-password"] = pwdStr
                // 文件类型需要以\0结尾
                saveConfig["private-key"] = config802_1x["private-key"] ? config802_1x["private-key"] + "\0" : undefined
                saveConfig["ca-cert"] = config802_1x["ca-cert"] ? config802_1x["ca-cert"] + "\0" : undefined
                saveConfig["client-cert"] = config802_1x["client-cert"] ? config802_1x["client-cert"] + "\0" : undefined
                break
            case "md5":
            case "leap":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["password-flags"] = pwdFlays
                saveConfig["password"] = pwdStr
                break
            case "fast":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["password-flags"] = pwdFlays
                saveConfig["password"] = pwdStr
                saveConfig["anonymous-identity"] = config802_1x["anonymous-identity"]
                saveConfig["phase1-fast-provisioning"] = config802_1x["phase1-fast-provisioning"]
                saveConfig["pac-file"] = config802_1x["pac-file"] ? config802_1x["pac-file"] + "\0" : undefined
                saveConfig["phase2-auth"] = config802_1x["phase2-auth"]
                break
            case "ttls":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["password-flags"] = pwdFlays
                saveConfig["password"] = pwdStr
                saveConfig["anonymous-identity"] = config802_1x["anonymous-identity"]
                saveConfig["ca-cert"] = config802_1x["ca-cert"] ? config802_1x["ca-cert"] + "\0" : undefined
                saveConfig["phase2-auth"] = config802_1x["phase2-auth"]
                break
            case "peap":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["password-flags"] = pwdFlays
                saveConfig["password"] = pwdStr
                saveConfig["anonymous-identity"] = config802_1x["anonymous-identity"]
                saveConfig["ca-cert"] = config802_1x["ca-cert"] ? config802_1x["ca-cert"] + "\0" : undefined
                saveConfig["phase1-peapver"] = config802_1x["phase1-peapver"]
                saveConfig["phase2-auth"] = config802_1x["phase2-auth"]
                break
            }
        } else {
            // delete saveConfig
            return undefined
        }

        return saveConfig
    }
    function checkInput() {
        return true
    }

    function removeTrailingNull(str) {
        return str ? str.toString().replace(/\0+$/, '') : ""
    }

    name: "secretTitle"
    displayName: qsTr("Security")

    ListModel {
        id: eapModel
        ListElement {
            text: qsTr("TLS")
            value: "tls"
        }
        ListElement {
            text: qsTr("MD5")
            value: "md5"
        }
        ListElement {
            text: qsTr("LEAP")
            value: "leap"
        }
        ListElement {
            text: qsTr("FAST")
            value: "fast"
        }
        ListElement {
            text: qsTr("Tunneled TLS")
            value: "ttls"
        }
        ListElement {
            text: qsTr("Protected EAP")
            value: "peap"
        }
    }
    ListModel {
        id: fastAuthModel
        ListElement {
            text: qsTr("GTC")
            value: "gtc"
        }
        ListElement {
            text: qsTr("MSCHAPV2")
            value: "mschapv2"
        }
    }
    ListModel {
        id: ttlsAuthModel
        ListElement {
            text: qsTr("PAP")
            value: "pap"
        }
        ListElement {
            text: qsTr("MSCHAP")
            value: "mschap"
        }
        ListElement {
            text: qsTr("MSCHAPV2")
            value: "mschapv2"
        }
        ListElement {
            text: qsTr("CHAP")
            value: "chap"
        }
    }
    ListModel {
        id: peapAuthModel
        ListElement {
            text: qsTr("GTC")
            value: "gtc"
        }
        ListElement {
            text: qsTr("MD5")
            value: "md5"
        }
        ListElement {
            text: qsTr("MSCHAPV2")
            value: "mschapv2"
        }
    }
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
    DccObject {
        name: "secretGroup"
        parentName: root.parentName
        weight: root.weight + 20
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "securityType"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Security")
            weight: 10
            visible: type === NetType.WirelessItem
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("None"),
                        "value": ""
                    }, {
                        "text": qsTr("WEP"),
                        "value": "none"
                    }, {
                        "text": qsTr("WPA/WPA2 Personal"),
                        "value": "wpa-psk"
                    }, {
                        "text": qsTr("WPA/WPA2 Enterprise"),
                        "value": "wpa-eap"
                    }, {
                        "text": qsTr("WPA3 Personal"),
                        "value": "sae"
                    }]
                currentIndex: indexOfValue(keyMgmt)
                onActivated: {
                    keyMgmt = currentValue
                }
                Component.onCompleted: {
                    currentIndex = indexOfValue(keyMgmt)
                }
            }
        }
        DccObject {
            name: "security"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Security")
            weight: 20
            visible: type === NetType.WiredItem
            pageType: DccObject.Editor
            page: D.Switch {
                checked: keyMgmt === "wpa-eap"
                onClicked: {
                    if ((keyMgmt === "wpa-eap") !== checked) {
                        keyMgmt = checked ? "wpa-eap" : ""
                    }
                }
            }
        }
        DccObject {
            name: "eapAuth"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("EAP Auth")
            weight: 30
            visible: keyMgmt === "wpa-eap"
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: indexOfValue(eapType)
                onActivated: {
                    eapType = currentValue
                }
                model: eapModel
                Component.onCompleted: {
                    currentIndex = indexOfValue(eapType)
                }
            }
        }
        DccObject {
            name: "identity"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Identity")
            weight: 40
            visible: keyMgmt === "wpa-eap"
            pageType: DccObject.Editor
            page: D.LineEdit {
                placeholderText: qsTr("Required")
                text: config802_1x && config802_1x.hasOwnProperty("identity") ? config802_1x.identity : ""
                onTextChanged: {
                    config802_1x.identity = text
                }
            }
        }
        DccObject {
            name: "pwdOptions"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Pwd Options")
            weight: 50
            visible: keyMgmt !== ""
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Save password for this user"),
                        "value": 1
                    }, {
                        "text": qsTr("Save password for all users"),
                        "value": 0
                    }, {
                        "text": qsTr("Ask me always"),
                        "value": 2
                    }]
                currentIndex: indexOfValue(pwdFlays)
                onActivated: {
                    pwdFlays = currentValue
                }
                Component.onCompleted: {
                    currentIndex = indexOfValue(pwdFlays)
                }
            }
        }
        DccObject {
            name: "password"
            parentName: root.parentName + "/secretGroup"
            displayName: keyMgmt === "none" ? qsTr("Key") : (eapType === "tls" ? qsTr("Private Pwd") : qsTr("Password"))
            weight: 60
            visible: keyMgmt !== "" && pwdFlays !== 2
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                property bool newInput: false
                placeholderText: qsTr("Required")
                text: pwdStr
                echoButtonVisible: newInput
                onTextChanged: {
                    if (text.length === 0) {
                        newInput = true
                    }
                    pwdStr = text
                }
                Component.onCompleted: newInput = pwdStr.length === 0
            }
        }
        DccObject {
            name: "Authentication"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Authentication")
            weight: 70
            visible: keyMgmt === "none"
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                model: [{
                        "text": qsTr("Shared key"),
                        "value": "shared"
                    }, {
                        "text": qsTr("Open system"),
                        "value": "open"
                    }]
                currentIndex: configWSecurity && configWSecurity.hasOwnProperty("auth-alg") ? indexOfValue(configWSecurity["auth-alg"]) : 0
                onActivated: {
                    configWSecurity["auth-alg"] = currentValue
                }
                Component.onCompleted: {
                    currentIndex = configWSecurity && configWSecurity.hasOwnProperty("auth-alg") ? indexOfValue(configWSecurity["auth-alg"]) : 0
                }
            }
        }
        DccObject {
            name: "privateKey"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Private Key")
            weight: 80
            visible: keyMgmt === "wpa-eap" && eapType === "tls"
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    id: privateKeyEdit
                    placeholderText: qsTr("Required")
                    text: config802_1x && config802_1x.hasOwnProperty("private-key") ? removeTrailingNull(config802_1x["private-key"]) : ""
                    onTextChanged: {
                        config802_1x["private-key"] = text
                    }
                }
                NetButton {
                    text: "..."
                    onClicked: {
                        fileDialog.createObject(this, {
                                                    "selectEdit": privateKeyEdit
                                                }).open()
                    }
                }
            }
        }
        DccObject {
            name: "anonymousID"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Anonymous ID")
            weight: 90
            visible: keyMgmt === "wpa-eap" && (eapType === "fast" || eapType === "ttls" || eapType === "peap")
            pageType: DccObject.Editor
            page: D.LineEdit {
                text: config802_1x && config802_1x.hasOwnProperty("anonymous-identity") ? config802_1x["anonymous-identity"] : ""
                onTextChanged: {
                    config802_1x["anonymous-identity"] = text
                }
            }
        }
        DccObject {
            name: "provisioning"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Provisioning")
            weight: 100
            visible: keyMgmt === "wpa-eap" && eapType === "fast"
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: (config802_1x && config802_1x.hasOwnProperty("phase1-fast-provisioning")) ? indexOfValue(config802_1x["phase1-fast-provisioning"]) : 0
                onActivated: {
                    config802_1x["phase1-fast-provisioning"] = currentValue
                }
                Component.onCompleted: {
                    currentIndex = (config802_1x && config802_1x.hasOwnProperty("phase1-fast-provisioning")) ? indexOfValue(config802_1x["phase1-fast-provisioning"]) : 0
                    if (!config802_1x.hasOwnProperty("phase1-fast-provisioning")) {
                        config802_1x["phase1-fast-provisioning"] = currentValue
                    }
                }
                model: [{
                        "text": qsTr("Disabled"),
                        "value": 0
                    }, {
                        "text": qsTr("Anonymous"),
                        "value": 1
                    }, {
                        "text": qsTr("Authenticated"),
                        "value": 2
                    }, {
                        "text": qsTr("Both"),
                        "value": 3
                    }]
            }
        }
        DccObject {
            name: "pacFile"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("PAC file")
            visible: keyMgmt === "wpa-eap" && eapType === "fast"
            weight: 110
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    id: pacFileEdit
                    text: config802_1x && config802_1x.hasOwnProperty("pac-file") ? removeTrailingNull(config802_1x["pac-file"]) : ""
                    onTextChanged: {
                        config802_1x["pac-file"] = text
                    }
                }
                NetButton {
                    text: "..."
                    onClicked: {
                        fileDialog.createObject(this, {
                                                    "selectEdit": pacFileEdit
                                                }).open()
                    }
                }
            }
        }
        DccObject {
            name: "caCert"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("CA Cert")
            weight: 120
            visible: keyMgmt === "wpa-eap" && (eapType === "tls" || eapType === "ttls" || eapType === "peap")
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    id: caCertEdit
                    text: config802_1x && config802_1x.hasOwnProperty("ca-cert") ? removeTrailingNull(config802_1x["ca-cert"]) : ""
                    onTextChanged: {
                        config802_1x["ca-cert"] = text
                    }
                }
                NetButton {
                    text: "..."
                    onClicked: {
                        fileDialog.createObject(this, {
                                                    "selectEdit": caCertEdit
                                                }).open()
                    }
                }
            }
        }
        DccObject {
            name: "userCert"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("User Cert")
            weight: 130
            visible: keyMgmt === "wpa-eap" && eapType === "tls"
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    id: clientCertEdit
                    placeholderText: qsTr("Required")
                    text: config802_1x && config802_1x.hasOwnProperty("client-cert") ? removeTrailingNull(config802_1x["client-cert"]) : ""
                    onTextChanged: {
                        config802_1x["client-cert"] = text
                    }
                }
                NetButton {
                    text: "..."
                    onClicked: {
                        fileDialog.createObject(this, {
                                                    "selectEdit": clientCertEdit
                                                }).open()
                    }
                }
            }
        }
        DccObject {
            name: "peapVersion"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("PEAP Version")
            weight: 140
            visible: keyMgmt === "wpa-eap" && eapType === "peap"
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: (config802_1x && config802_1x.hasOwnProperty("phase1-peapver")) ? indexOfValue(config802_1x["phase1-peapver"]) : 0
                onActivated: {
                    config802_1x["phase1-peapver"] = currentValue
                }
                Component.onCompleted: {
                    currentIndex = (config802_1x && config802_1x.hasOwnProperty("phase1-peapver")) ? indexOfValue(config802_1x["phase1-peapver"]) : 0
                    if (!config802_1x.hasOwnProperty("phase1-peapver")) {
                        config802_1x["phase1-peapver"] = currentValue
                    }
                }
                model: [{
                        "text": qsTr("Automatic"),
                        "value": undefined
                    }, {
                        "text": qsTr("Version 0"),
                        "value": 0
                    }, {
                        "text": qsTr("Version 1"),
                        "value": 1
                    }]
            }
        }
        DccObject {
            name: "innerAuth"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Inner Auth")
            weight: 150
            visible: keyMgmt === "wpa-eap" && (eapType === "fast" || eapType === "ttls" || eapType === "peap")
            pageType: DccObject.Editor
            page: D.ComboBox {
                textRole: "text"
                valueRole: "value"
                currentIndex: (config802_1x && config802_1x.hasOwnProperty("phase2-auth")) ? indexOfValue(config802_1x["phase2-auth"]) : 0
                onActivated: {
                    config802_1x["phase2-auth"] = currentValue
                }
                model: eapType === "peap" ? peapAuthModel : (eapType === "fast" ? fastAuthModel : ttlsAuthModel)
                Component.onCompleted: {
                    currentIndex = (config802_1x && config802_1x.hasOwnProperty("phase2-auth")) ? indexOfValue(config802_1x["phase2-auth"]) : 0
                    if (!config802_1x.hasOwnProperty("phase2-auth")) {
                        config802_1x["phase2-auth"] = currentValue
                    }
                }
            }
        }
    }
}
