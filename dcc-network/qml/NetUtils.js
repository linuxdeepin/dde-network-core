// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
.pragma library

const VpnTypeEnum = Object.freeze({
                                      "l2tp": 0x01,
                                      "pptp": 0x02,
                                      "vpnc": 0x04,
                                      "openvpn": 0x08,
                                      "strongswan": 0x10,
                                      "openconnect": 0x20,
                                      "sstp": 0x40
                                  })
function toVpnTypeEnum(vpnKey) {
    let key = vpnKey.substring(31)
    console.log("toVpnTypeEnum", vpnKey, key)
    return VpnTypeEnum.hasOwnProperty(key) ? VpnTypeEnum[key] : 0x01
}

function toVpnKey(vpnType) {
    for (let key in VpnTypeEnum) {
        if (VpnTypeEnum[key] === vpnType) {
            return "org.freedesktop.NetworkManager." + key
        }
    }
    return "org.freedesktop.NetworkManager.l2tp"
}

function removeTrailingNull(str) {
    return str ? str.toString().replace(/\0+$/, '') : ""
}
