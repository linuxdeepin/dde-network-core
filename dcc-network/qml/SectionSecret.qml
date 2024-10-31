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
    property var configWSecurity: undefined
    property var config802_1x: undefined
    property string parentName: ""
    property int type: NetType.WiredItem
    property string eapType: "peap"
    property int pwdFlays: 0
    property string pwdStr: ""
    property string keyMgmt: ""

    property string errorKey: ""
    signal editClicked

    function setConfigWSecurity(c) {
        errorKey = ""
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
        errorKey = ""
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
                if (config802_1x.hasOwnProperty("private-key") && config802_1x["private-key"] !== "") {
                    saveConfig["private-key"] = NetUtils.strToByteArray(config802_1x["private-key"])
                }
                if (config802_1x.hasOwnProperty("ca-cert") && config802_1x["ca-cert"] !== "") {
                    saveConfig["ca-cert"] = NetUtils.strToByteArray(config802_1x["ca-cert"])
                }
                if (config802_1x.hasOwnProperty("client-cert") && config802_1x["client-cert"] !== "") {
                    saveConfig["client-cert"] = NetUtils.strToByteArray(config802_1x["client-cert"])
                }
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
                if (config802_1x.hasOwnProperty("pac-file") && config802_1x["pac-file"] !== "") {
                    saveConfig["pac-file"] = NetUtils.strToByteArray(config802_1x["pac-file"])
                }
                saveConfig["phase2-auth"] = config802_1x["phase2-auth"]
                break
            case "ttls":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["password-flags"] = pwdFlays
                saveConfig["password"] = pwdStr
                saveConfig["anonymous-identity"] = config802_1x["anonymous-identity"]
                if (config802_1x.hasOwnProperty("ca-cert") && config802_1x["ca-cert"] !== "") {
                    saveConfig["ca-cert"] = NetUtils.strToByteArray(config802_1x["ca-cert"])
                }
                saveConfig["phase2-auth"] = config802_1x["phase2-auth"]
                break
            case "peap":
                saveConfig["identity"] = config802_1x["identity"]
                saveConfig["password-flags"] = pwdFlays
                saveConfig["password"] = pwdStr
                saveConfig["anonymous-identity"] = config802_1x["anonymous-identity"]
                if (config802_1x.hasOwnProperty("ca-cert") && config802_1x["ca-cert"] !== "") {
                    saveConfig["ca-cert"] = NetUtils.strToByteArray(config802_1x["ca-cert"])
                }
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
        errorKey = ""
        switch (keyMgmt) {
        case "none":
            if (pwdFlays !== 2 && !dccData.CheckPasswordValid("wep-key0", pwdStr)) {
                errorKey = "password"
                return false
            }
            break
        case "wpa-psk":
        case "sae":
            if (pwdFlays !== 2 && !dccData.CheckPasswordValid("psk", pwdStr)) {
                errorKey = "password"
                return false
            }
            break
        case "wpa-eap":
            console.log("identity", config802_1x["identity"], typeof config802_1x["identity"])
            if (!config802_1x["identity"] || config802_1x["identity"] === "") {
                errorKey = "identity"
                return false
            }
            if (pwdFlays !== 2 && !dccData.CheckPasswordValid("password", pwdStr)) {
                errorKey = "password"
                return false
            }
            switch (eapType) {
            case "tls":
                if (!config802_1x["private-key"] || config802_1x["private-key"] === "") {
                    errorKey = "private-key"
                    return false
                }
                if (!config802_1x["client-cert"] || config802_1x["client-cert"] === "") {
                    errorKey = "client-cert"
                    return false
                }
                break
            default:
                break
            }
            break
        default:
            return true
        }
        return true
    }

    function removeTrailingNull(str) {
        return str ? str.toString().replace(/\0+$/, '') : ""
    }

    name: "secretTitle"
    displayName: qsTr("Security")

    ListModel {
        id: eapModelWired
        ListElement {
            text: qsTr("TLS")
            value: "tls"
        }
        ListElement {
            text: qsTr("MD5")
            value: "md5"
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
        id: eapModelWireless
        ListElement {
            text: qsTr("TLS")
            value: "tls"
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
                    root.editClicked()
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
                        root.editClicked()
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
                    root.editClicked()
                }
                model: type === NetType.WiredItem ? eapModelWired : eapModelWireless
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
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (config802_1x.identity !== text) {
                        config802_1x.identity = text
                        root.editClicked()
                    }
                }
                showAlert: errorKey === dccObj.name
                alertDuration: 2000
                onShowAlertChanged: {
                    if (showAlert) {
                        dccObj.trigger()
                        forceActiveFocus()
                    }
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
                    root.editClicked()
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
            page: NetPasswordEdit {
                text: pwdStr
                dataItem: root
                onTextUpdated: pwdStr = text
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
                    root.editClicked()
                }
                Component.onCompleted: {
                    currentIndex = configWSecurity && configWSecurity.hasOwnProperty("auth-alg") ? indexOfValue(configWSecurity["auth-alg"]) : 0
                }
            }
        }
        DccObject {
            name: "private-key"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("Private Key")
            weight: 80
            visible: keyMgmt === "wpa-eap" && eapType === "tls"
            pageType: DccObject.Editor
            page: NetFileChooseEdit {
                dataItem: root
                placeholderText: qsTr("Required")
                text: config802_1x && config802_1x.hasOwnProperty("private-key") ? removeTrailingNull(config802_1x["private-key"]) : ""
                onTextUpdated: config802_1x["private-key"] = text
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
                    if (config802_1x["anonymous-identity"] !== text) {
                        config802_1x["anonymous-identity"] = text
                        root.editClicked()
                    }
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
                    root.editClicked()
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
            name: "pac-file"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("PAC file")
            visible: keyMgmt === "wpa-eap" && eapType === "fast"
            weight: 110
            pageType: DccObject.Editor
            page: NetFileChooseEdit {
                dataItem: root
                text: config802_1x && config802_1x.hasOwnProperty(dccObj.name) ? removeTrailingNull(config802_1x[dccObj.name]) : ""
                onTextUpdated: config802_1x[dccObj.name] = text
            }
        }
        DccObject {
            name: "ca-cert"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("CA Cert")
            weight: 120
            visible: keyMgmt === "wpa-eap" && (eapType === "tls" || eapType === "ttls" || eapType === "peap")
            pageType: DccObject.Editor
            page: NetFileChooseEdit {
                dataItem: root
                text: config802_1x && config802_1x.hasOwnProperty(dccObj.name) ? removeTrailingNull(config802_1x[dccObj.name]) : ""
                onTextUpdated: config802_1x[dccObj.name] = text
            }
        }
        DccObject {
            name: "client-cert"
            parentName: root.parentName + "/secretGroup"
            displayName: qsTr("User Cert")
            weight: 130
            visible: keyMgmt === "wpa-eap" && eapType === "tls"
            pageType: DccObject.Editor
            page: NetFileChooseEdit {
                dataItem: root
                placeholderText: qsTr("Required")
                text: config802_1x && config802_1x.hasOwnProperty(dccObj.name) ? removeTrailingNull(config802_1x[dccObj.name]) : ""
                onTextUpdated: config802_1x[dccObj.name] = text
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
                    root.editClicked()
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
                    root.editClicked()
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
