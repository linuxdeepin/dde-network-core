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

    property string errorKey: ""
    signal editClicked

    // 控件变化
    property bool mppe: false
    property string currentMppeMethod: "require-mppe"
    property bool ipsecEnabled: false
    property string currentEncryption: "secure"

    function setConfig(c) {
        errorKey = ""
        config = c !== undefined ? c : {}
        dataMap = config.hasOwnProperty("data") ? dccData.toMap(config.data) : {}
        secretMap = config.hasOwnProperty("secrets") ? dccData.toMap(config.secrets) : {}

        const mppeList = ["require-mppe", "require-mppe-40", "require-mppe-128"]
        mppe = false
        currentMppeMethod = "require-mppe"
        for (let mppeMethod of mppeList) {
            if (dataMap.hasOwnProperty(mppeMethod) && dataMap[mppeMethod] === "yes") {
                mppe = true
                currentMppeMethod = mppeMethod
            }
        }
        ipsecEnabled = dataMap["ipsec-enabled"] === "yes"
        // 防止密码项没设置对应项显示错误，先给默认值
        const pwdKeys = ["password-flags", "Xauth password-flags", "IPSec secret-flags", "cert-pass-flags"]
        for (let pwdKey of pwdKeys) {
            if (!dataMap.hasOwnProperty(pwdKey)) {
                dataMap[pwdKey] = "0"
            }
        }
        if (!dataMap.hasOwnProperty("method")) {
            dataMap["method"] = "key"
        }
        if (!dataMap.hasOwnProperty("connection-type")) {
            dataMap["connection-type"] = "tls"
        }
        console.log("proxy-type:  ", dataMap["proxy-type"], dataMap["proxy-type"] !== "none")
        if (!dataMap.hasOwnProperty("proxy-type")) {
            dataMap["proxy-type"] = "none"
        }
        console.log("proxy-type:  ", dataMap["proxy-type"], dataMap["proxy-type"] !== "none")
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
        }
        let aa = (vpnType & (NetUtils.VpnTypeEnum["openconnect"])) || ((vpnType & NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "key"))
        console.log("aa:    ", aa, vpnType, dataMap["method"])

        dataMapChanged()
    }
    function getConfig() {
        if (mppe) {
            switch (currentMppeMethod) {
            case "require-mppe":
                dataMap["require-mppe"] = "yes"
                delete dataMap["require-mppe-40"]
                delete dataMap["require-mppe-128"]
                break
            case "require-mppe-40":
                dataMap["require-mppe-40"] = "yes"
                delete dataMap["require-mppe"]
                delete dataMap["require-mppe-128"]
                break
            case "require-mppe-128":
                dataMap["require-mppe-128"] = "yes"
                delete dataMap["require-mppe-40"]
                delete dataMap["require-mppe"]
                break
            }
        } else {
            delete dataMap["require-mppe"]
            delete dataMap["require-mppe-40"]
            delete dataMap["require-mppe-128"]
            delete config["mppe-stateful"]
        }
        // vpn配置全在这，防止不同vpn的配置都保存，这里用key过虑下
        let dataKeys = []
        let secretKeys = []
        const pppKeys = ["require-mppe", "require-mppe-40", "require-mppe-128", "mppe-stateful", "refuse-eap", "refuse-pap", "refuse-chap", "refuse-mschap", "refuse-mschapv2", "nobsdcomp", "nodeflate", "no-vj-comp", "nopcomp", "noaccomp", "lcp-echo-failure", "lcp-echo-interval"]
        const ipsecKeys = ["ipsec-enabled", "ipsec-group-name", "ipsec-gateway-id", "ipsec-psk", "ipsec-ike", "ipsec-esp"]
        const vpncAdvKeys = ["Domain", "Vendor", "Application Version", "Enable Single DES", "Enable no encryption", "NAT Traversal Mode", "IKE DH Group", "Perfect Forward Secrecy", "Local Port", "DPD idle timeout (our side)"]
        switch (vpnType) {
        case NetUtils.VpnTypeEnum["l2tp"]:
            dataKeys = ["gateway", "user", "password-flags", "domain"]
            dataKeys = dataKeys.concat(pppKeys)
            dataKeys = dataKeys.concat(ipsecKeys)
            if (dataMap["password-flags"] === "0") {
                secretKeys = ["password"]
            }
            break
        case NetUtils.VpnTypeEnum["pptp"]:
            dataKeys = ["gateway", "user", "password-flags", "domain"]
            dataKeys = dataKeys.concat(pppKeys)
            if (dataMap["password-flags"] === "0") {
                secretKeys = ["password"]
            }
            break
        case NetUtils.VpnTypeEnum["vpnc"]:
            dataKeys = ["IPSec gateway", "Xauth username", "Xauth password-flags", "xauth-password-type", "IPSec ID", "IPSec secret-flags", "ipsec-secret-type", "IKE Authmode", "CA-File"]
            dataKeys = dataKeys.concat(vpncAdvKeys)
            if (dataMap["Xauth password-flags"] === "0") {
                secretKeys.push("Xauth password")
            }
            if (dataMap["IPSec secret-flags"] === "0") {
                secretKeys.push("IPSec secret")
            }
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
            switch (dataMap["Xauth password-flags"]) {
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
            switch (dataMap["IPSec secret-flags"]) {
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
            break
        case NetUtils.VpnTypeEnum["openconnect"]:
            dataKeys = ["gateway", "cacert", "proxy", "enable_csd_trojan", "csd_wrapper", "usercert", "userkey", "pem_passphrase_fsid", "cookie-flags"]
            dataMap["cookie-flags"] = "2"
            break
        case NetUtils.VpnTypeEnum["strongswan"]:
            dataKeys = ["address", "certificate", "method", "usercert", "userkey", "user", "virtual", "encap", "ipcomp", "proposal", "ike", "esp"]
            secretKeys = ["password"]
            break
        case NetUtils.VpnTypeEnum["openvpn"]:
            dataKeys = ["remote", "connection-type", "port", "reneg-seconds", "comp-lzo", "proto-tcp", "dev-type", "tunnel-mtu", "fragment-size", "mssfix", "remote-random", "tls-remote", "remote-cert-tls", "ta", "ta-dir"]
            switch (dataMap["connection-type"]) {
            case "tls":
                dataKeys = dataKeys.concat(["ca", "cert", "key", "cert-pass-flags"])
                if (dataMap["cert-pass-flags"] === "0") {
                    secretKeys.push("cert-pass")
                }
                break
            case "password":
                dataKeys = dataKeys.concat(["ca", "username", "password-flags"])
                if (dataMap["password-flags"] === "0") {
                    secretKeys.push("password")
                }
                break
            case "password-tls":
                dataKeys = dataKeys.concat(["ca", "username", "password-flags", "cert", "key", "cert-pass-flags"])
                if (dataMap["password-flags"] === "0") {
                    secretKeys.push("password")
                }
                if (dataMap["cert-pass-flags"] === "0") {
                    secretKeys.push("cert-pass")
                }
                break
            case "static-key":
                dataKeys = dataKeys.concat(["static-key", "static-key-direction", "remote-ip", "local-ip"])
                break
            }
            if (dataMap["cipher"] !== "default") {
                dataKeys.push("cipher")
            }
            if (dataMap["auth"] !== "default") {
                dataKeys.push("auth")
            }

            switch (dataMap["proxy-type"]) {
            case "http":
                dataKeys = dataKeys.concat(["proxy-type", "proxy-server", "proxy-port", "proxy-retry", "http-proxy-username", "http-proxy-password-flags"])
                dataMap["http-proxy-password-flags"] = "0"
                secretKeys.push("http-proxy-password")
                break
            case "socks":
                dataKeys = dataKeys.concat(["proxy-type", "proxy-server", "proxy-port", "proxy-retry"])
                break
            case "none":
                break
            }
            break
        }
        let tmpDataMap = {}
        for (var key of dataKeys) {
            if (dataMap.hasOwnProperty(key)) {
                tmpDataMap[key] = dataMap[key]
            }
        }
        let tmpSecretMap = {}
        for (key of secretKeys) {
            if (secretMap.hasOwnProperty(key)) {
                tmpSecretMap[key] = secretMap[key]
            }
        }

        config.data = dccData.toStringMap(tmpDataMap)
        config.secrets = dccData.toStringMap(tmpSecretMap)
        return config
    }
    function checkInput() {
        errorKey = ""
        let checkKeys = []
        switch (vpnType) {
        case NetUtils.VpnTypeEnum["pptp"]:
        case NetUtils.VpnTypeEnum["l2tp"]:
            // ipv6 不支持，这里过滤掉，不然后面host 会反向解析浪费时间。
            if (!dataMap.hasOwnProperty("gateway") || dataMap["gateway"].length === 0 || NetUtils.ipv6RegExp.test(dataMap["gateway"])) {
                errorKey = "gateway"
                return false
            }
            checkKeys = [[dataMap, "gateway"], [dataMap, "user"]]
            console.log("password-flags", dataMap["password-flags"])
            if (!dataMap.hasOwnProperty("password-flags") || dataMap["password-flags"] === "0") {
                checkKeys.push([secretMap, "password"])
            }
            console.log("length", checkKeys.length)
            break
        case NetUtils.VpnTypeEnum["vpnc"]:
            checkKeys = [[dataMap, "IPSec gateway"], [dataMap, "Xauth username"]]
            if (!dataMap.hasOwnProperty("Xauth password-flags") || dataMap["Xauth password-flags"] === "0") {
                checkKeys.push([secretMap, "Xauth password"])
            }
            checkKeys.push([dataMap, "IPSec ID"])
            if (!dataMap.hasOwnProperty("IPSec secret-flags") || dataMap["IPSec secret-flags"] === "0") {
                checkKeys.push([secretMap, "IPSec secret"])
            }
            if (dataMap["IKE Authmode"] === "hybrid") {
                checkKeys.push([dataMap, "CA-File"])
            }
            break
        case NetUtils.VpnTypeEnum["openconnect"]:
            checkKeys = [[dataMap, "gateway"], [dataMap, "usercert"], [dataMap, "userkey"]]
            break
        case NetUtils.VpnTypeEnum["strongswan"]:
            checkKeys = [[dataMap, "address"]]
            break
        case NetUtils.VpnTypeEnum["openvpn"]:
            checkKeys = [[dataMap, "remote"]]
            const openvpnTlsKeys = [[dataMap, "cert"], [dataMap, "key"]]
            const openvpnPwdKeys = [[dataMap, "username"]]
            switch (dataMap["connection-type"]) {
            case "tls":
                checkKeys.push([dataMap, "ca"])
                checkKeys = checkKeys.concat(openvpnTlsKeys)
                if (dataMap["cert-pass-flags"] === "0") {
                    checkKeys.push([secretMap, "cert-pass"])
                }
                break
            case "password":
                checkKeys.push([dataMap, "ca"])
                checkKeys = checkKeys.concat(openvpnPwdKeys)
                if (dataMap["password-flags"] === "0") {
                    checkKeys.push([secretMap, "password"])
                }
                break
            case "password-tls":
                checkKeys.push([dataMap, "ca"])
                checkKeys = checkKeys.concat(openvpnPwdKeys)
                checkKeys = checkKeys.concat(openvpnTlsKeys)
                if (dataMap["password-flags"] === "0") {
                    checkKeys.push([secretMap, "password"])
                }
                if (dataMap["cert-pass-flags"] === "0") {
                    checkKeys.push([secretMap, "cert-pass"])
                }
                break
            case "static-key":
                checkKeys = checkKeys.concat([[dataMap, "static-key"], [dataMap, "remote-ip"], [dataMap, "local-ip"]])
                break
            }
            switch (dataMap["proxy-type"]) {
            case "http":
                checkKeys = checkKeys.concat([[dataMap, "proxy-server"], [dataMap, "http-proxy-username"], [secretMap, "http-proxy-password"]])
                break
            case "socks":
                checkKeys.push([dataMap, "proxy-server"])
                break
            case "none":
                break
            }
            break
        }
        for (var key of checkKeys) {
            console.log("err:", key[1], key[0][key[1]])
            if (!key[0].hasOwnProperty(key[1]) || key[0][key[1]].length === 0) {
                errorKey = key[1]
                return false
            }
        }
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
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (dataMap[dccObj.name] !== text) {
                        dataMap[dccObj.name] = text
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
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (dataMap[dccObj.name] !== text) {
                        dataMap[dccObj.name] = text
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
        id: gatewayLineEdit
        D.LineEdit {
            placeholderText: qsTr("Required")
            text: dataMap.hasOwnProperty(dccObj.name) ? dataMap[dccObj.name] : ""
            onTextChanged: {
                if (showAlert) {
                    errorKey = ""
                }
                if (dataMap[dccObj.name] !== text) {
                    dataMap[dccObj.name] = text
                    root.editClicked()
                }
            }
            showAlert: errorKey === dccObj.name
            alertDuration: 2000
            alertText: qsTr("Invalid gateway")
            onShowAlertChanged: {
                if (showAlert) {
                    dccObj.trigger()
                    forceActiveFocus()
                }
            }
        }
    }
    Component {
        id: requiredLineEdit
        D.LineEdit {
            placeholderText: qsTr("Required")
            text: dataMap.hasOwnProperty(dccObj.name) ? dataMap[dccObj.name] : ""
            onTextChanged: {
                if (showAlert) {
                    errorKey = ""
                }
                if (dataMap[dccObj.name] !== text) {
                    dataMap[dccObj.name] = text
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
    Component {
        id: lineEdit
        D.LineEdit {
            text: dataMap.hasOwnProperty(dccObj.name) ? dataMap[dccObj.name] : ""
            onTextChanged: {
                if (showAlert) {
                    errorKey = ""
                }
                if (dataMap[dccObj.name] !== text) {
                    dataMap[dccObj.name] = text
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
    Component {
        id: switchItem
        D.Switch {
            checked: dataMap.hasOwnProperty(dccObj.name) && dataMap[dccObj.name] === "yes"
            onClicked: {
                if (checked) {
                    dataMap[dccObj.name] = "yes"
                } else {
                    delete dataMap[dccObj.name]
                }
                dataMapChanged()
                root.editClicked()
            }
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
            page: gatewayLineEdit
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
            page: gatewayLineEdit
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
                    root.editClicked()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "usercert"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("User Cert")
            weight: 90
            visible: (vpnType & (NetUtils.VpnTypeEnum["openconnect"])) || ((vpnType & NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "key" || dataMap["method"] === "agent"))
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    id: usercertEdit
                    placeholderText: vpnType === NetUtils.VpnTypeEnum["strongswan"] ? "" : qsTr("Required")
                    text: dataMap.hasOwnProperty(dccObj.name) ? NetUtils.removeTrailingNull(dataMap[dccObj.name]) : ""
                    onTextChanged: {
                        if (showAlert) {
                            errorKey = ""
                        }
                        if (dataMap[dccObj.name] !== text) {
                            dataMap[dccObj.name] = text
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
                NetButton {
                    text: "..."
                    onClicked: {
                        fileDialog.createObject(this, {
                                                    "selectEdit": usercertEdit
                                                }).open()
                    }
                }
            }
        }
        DccObject {
            name: "userkey"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Private Key")
            weight: 100
            visible: (vpnType & (NetUtils.VpnTypeEnum["openconnect"])) || ((vpnType & NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "key"))
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    id: userkeyEdit
                    placeholderText: vpnType === NetUtils.VpnTypeEnum["strongswan"] ? "" : qsTr("Required")
                    text: dataMap.hasOwnProperty(dccObj.name) ? NetUtils.removeTrailingNull(dataMap[dccObj.name]) : ""
                    onTextChanged: {
                        if (showAlert) {
                            errorKey = ""
                        }
                        if (dataMap[dccObj.name] !== text) {
                            dataMap[dccObj.name] = text
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
                NetButton {
                    text: "..."
                    onClicked: {
                        fileDialog.createObject(this, {
                                                    "selectEdit": userkeyEdit
                                                }).open()
                    }
                }
            }
        }
        DccObject {
            name: "user"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Username")
            weight: 110
            visible: (vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])) || ((vpnType & NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "eap" || dataMap["method"] === "psk"))
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "pem_passphrase_fsid"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Use FSID for Key Passphrase")
            weight: 120
            visible: vpnType & (NetUtils.VpnTypeEnum["openconnect"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "remote"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Gateway")
            weight: 130
            visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"])
            pageType: DccObject.Editor
            page: gatewayLineEdit
        }
        DccObject {
            name: "connection-type"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Auth Type")
            weight: 140
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
            weight: 150
            visible: vpnType & (NetUtils.VpnTypeEnum["openvpn"]) && (dataMap["connection-type"] === "tls" || dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "username"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Username")
            weight: 160
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "password-flags"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Pwd Options")
            weight: 170
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
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                    root.editClicked()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "password"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Password")
            weight: 180
            visible: ((vpnType & NetUtils.VpnTypeEnum["strongswan"]) && (dataMap["method"] === "eap" || dataMap["method"] === "psk")) || (dataMap["password-flags"] === "0" && ((vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])) || (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password" || dataMap["connection-type"] === "password-tls")))
            pageType: DccObject.Editor
            page: D.PasswordEdit {
                placeholderText: qsTr("Required")
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (secretMap[dccObj.name] !== text) {
                        secretMap[dccObj.name] = text
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
            name: "cert"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("User Cert")
            weight: 190
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: fileLineEdit
        }
        DccObject {
            name: "key"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Private Key")
            weight: 200
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: fileLineEdit
        }
        DccObject {
            name: "cert-pass-flags"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Pwd Options")
            weight: 210
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
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
            }
        }
        DccObject {
            name: "cert-pass"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Private Pwd")
            weight: 220
            visible: dataMap["cert-pass-flags"] === "0" && (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "password-tls" || dataMap["connection-type"] === "tls")
            pageType: DccObject.Editor
            page: NetPasswordEdit {
                dataItem: root
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextUpdated: secretMap[dccObj.name] = text
            }
        }
        DccObject {
            name: "static-key"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Static Key")
            weight: 230
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "has_static-key-direction"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Customize Key Direction")
            weight: 240
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
            weight: 250
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
            weight: 260
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "local-ip"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Local IP")
            weight: 270
            visible: (vpnType & (NetUtils.VpnTypeEnum["openvpn"])) && (dataMap["connection-type"] === "static-key")
            pageType: DccObject.Editor
            page: requiredFileLineEdit
        }
        DccObject {
            name: "domain"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("NT Domain")
            weight: 280
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"] | NetUtils.VpnTypeEnum["pptp"])
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "virtual"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Request an Inner IP Address")
            weight: 290
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "encap"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Enforce UDP Encapsulation")
            weight: 300
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "ipcomp"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Use IP Compression")
            weight: 310
            visible: vpnType & (NetUtils.VpnTypeEnum["strongswan"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "proposal"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Enable Custom Cipher Proposals")
            weight: 320
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
            weight: 330
            visible: (vpnType & (NetUtils.VpnTypeEnum["strongswan"])) && dataMap["proposal"] === "yes"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        DccObject {
            name: "esp"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("ESP")
            weight: 340
            visible: (vpnType & (NetUtils.VpnTypeEnum["strongswan"])) && dataMap["proposal"] === "yes"
            pageType: DccObject.Editor
            page: requiredLineEdit
        }
        // vpnc
        DccObject {
            name: "IPSec gateway"
            parentName: root.parentName + "/vpnGroup"
            displayName: qsTr("Gateway")
            weight: 350
            visible: vpnType & (NetUtils.VpnTypeEnum["vpnc"])
            pageType: DccObject.Editor
            page: gatewayLineEdit
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
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                    root.editClicked()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
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
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (secretMap[dccObj.name] !== text) {
                        secretMap[dccObj.name] = text
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
                currentIndex: dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    dataMapChanged()
                    root.editClicked()
                }
                Component.onCompleted: currentIndex = dataMap.hasOwnProperty(dccObj.name) ? indexOfValue(dataMap[dccObj.name]) : 0
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
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (secretMap[dccObj.name] !== text) {
                        secretMap[dccObj.name] = text
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
                    root.editClicked()
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
            name: "require-mppe"
            parentName: root.parentName + "/vpnPPPMPPEGroup"
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
                onActivated: {
                    currentMppeMethod = currentValue
                    root.editClicked()
                }
                Component.onCompleted: currentIndex = indexOfValue(currentMppeMethod)
            }
        }
        DccObject {
            name: "mppe-stateful"
            parentName: root.parentName + "/vpnPPPMPPEGroup"
            displayName: qsTr("Stateful MPPE")
            weight: 30
            visible: mppe
            pageType: DccObject.Editor
            page: switchItem
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
            name: "refuse-eap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse EAP Authentication")
            weight: 40
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-pap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse PAP Authentication")
            weight: 50
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-chap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse CHAP Authentication")
            weight: 60
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-mschap"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse MSCHAP Authentication")
            weight: 70
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "refuse-mschapv2"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("Refuse MSCHAPv2 Authentication")
            weight: 80
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nobsdcomp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No BSD Data Compression")
            weight: 90
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nodeflate"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No Deflate Data Compression")
            weight: 100
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "no-vj-comp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No TCP Header Compression")
            weight: 110
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "nopcomp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No Protocol Field Compression")
            weight: 120
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"])
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "noaccomp"
            parentName: root.parentName + "/vpnPPPGroup"
            displayName: qsTr("No Address/Control Compression")
            weight: 130
            visible: vpnType & (NetUtils.VpnTypeEnum["l2tp"])
            pageType: DccObject.Editor
            page: switchItem
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
                        delete dataMap["lcp-echo-failure"]
                        delete dataMap["lcp-echo-interval"]
                    }
                    root.editClicked()
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
            name: "ipsec-enabled"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Enable IPsec")
            weight: 10
            pageType: DccObject.Editor
            page: switchItem
        }
        DccObject {
            name: "ipsec-group-name"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Group Name")
            weight: 20
            visible: dataMap["ipsec-enabled"] === "yes"
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "ipsec-gateway-id"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Group ID")
            weight: 30
            visible: dataMap["ipsec-enabled"] === "yes"
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "ipsec-psk"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Pre-Shared Key")
            weight: 40
            visible: dataMap["ipsec-enabled"] === "yes"
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "ipsec-ike"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Phase1 Algorithms")
            weight: 50
            visible: dataMap["ipsec-enabled"] === "yes"
            pageType: DccObject.Editor
            page: lineEdit
        }
        DccObject {
            name: "ipsec-esp"
            parentName: root.parentName + "/vpnIPsecGroup"
            displayName: qsTr("Phase2 Algorithms")
            weight: 60
            visible: dataMap["ipsec-enabled"] === "yes"
            pageType: DccObject.Editor
            page: lineEdit
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
                    root.editClicked()
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
                onValueChanged: {
                    if (dataMap[dccObj.name] !== value) {
                        dataMap[dccObj.name] = value
                        root.editClicked()
                    }
                }
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
                    root.editClicked()
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
                onValueChanged: {
                    if (dataMap[dccObj.name] !== value) {
                        dataMap[dccObj.name] = value
                        root.editClicked()
                    }
                }
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
                        dataMap[dccObj.name] = "yes"
                    } else {
                        delete dataMap[dccObj.name]
                    }
                    dataMapChanged()
                    root.editClicked()
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
            name: "dev-type"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("Use TAP Device")
            weight: 70
            pageType: DccObject.Editor
            page: switchItem
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
                    root.editClicked()
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
                onValueChanged: {
                    if (dataMap[dccObj.name] !== value) {
                        dataMap[dccObj.name] = value
                        root.editClicked()
                    }
                }
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
                    root.editClicked()
                }
            }
        }
        DccObject {
            name: "fragment-size"
            parentName: root.parentName + "/vpnAdvancedGroup"
            displayName: qsTr("UDP Fragment Size")
            weight: 110
            visible: dataMap.hasOwnProperty("fragment-size")
            pageType: DccObject.Editor
            page: D.SpinBox {
                value: dataMap.hasOwnProperty(dccObj.name) ? parseInt(dataMap[dccObj.name], 10) : 1300
                from: 0
                to: 65535
                onValueChanged: {
                    if (dataMap[dccObj.name] !== value) {
                        dataMap[dccObj.name] = value
                        root.editClicked()
                    }
                }
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
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    root.editClicked()
                }
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
                onActivated: {
                    currentEncryption = currentValue
                    root.editClicked()
                }
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
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    root.editClicked()
                }
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
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    root.editClicked()
                }
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
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    root.editClicked()
                }
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
                onValueChanged: {
                    if (dataMap[dccObj.name] !== value) {
                        dataMap[dccObj.name] = value
                        root.editClicked()
                    }
                }
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
                    root.editClicked()
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
                onActivated: {
                    dataMap["cipher"] = currentValue
                    root.editClicked()
                }
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
                onActivated: {
                    dataMap["auth"] = currentValue
                    root.editClicked()
                }
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
                    root.editClicked()
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
                onValueChanged: {
                    if (dataMap[dccObj.name] !== value) {
                        dataMap[dccObj.name] = value
                        root.editClicked()
                    }
                }
                Component.onCompleted: {
                    console.log("onCompleted-type:  ", dataMap["proxy-type"], dataMap["proxy-type"] !== "none", dccObj.visible)
                }
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
            page: D.PasswordEdit {
                placeholderText: qsTr("Required")
                text: secretMap.hasOwnProperty(dccObj.name) ? secretMap[dccObj.name] : ""
                onTextChanged: {
                    if (showAlert) {
                        errorKey = ""
                    }
                    if (secretMap[dccObj.name] !== text) {
                        secretMap[dccObj.name] = text
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
            page: lineEdit
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
                onActivated: {
                    if (currentValue === "default") {
                        delete dataMap[dccObj.name]
                    } else {
                        dataMap[dccObj.name] = currentValue
                    }
                    root.editClicked()
                }
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
            page: fileLineEdit
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
                    root.editClicked()
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
                onActivated: {
                    dataMap[dccObj.name] = currentValue
                    root.editClicked()
                }
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
