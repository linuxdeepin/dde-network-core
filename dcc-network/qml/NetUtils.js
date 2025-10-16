// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
const VpnTypeEnum = Object.freeze({
                                      "l2tp": 0x01,
                                      "pptp": 0x02,
                                      "vpnc": 0x04,
                                      "openvpn": 0x08,
                                      "strongswan": 0x10,
                                      "openconnect": 0x20,
                                      "sstp": 0x40
                                  })
// ip正则表达式
const ipRegExp = /^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))$/
const ipv6RegExp = /^(((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))))$/
// 子网掩码
const maskRegExp = /(254|252|248|240|224|192|128|0)\.0\.0\.0|255\.(254|252|248|240|224|192|128|0)\.0\.0|255\.255\.(254|252|248|240|224|192|128|0)\.0|255\.255\.255\.(254|252|248|240|224|192|128|0)/
// MAC正则表达式
const macRegExp = /([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})/

// 验证IP地址是否为IPv4
function isIpv4Address(ip) {
    const result = ipRegExp.test(ip)
    console.log("[IPv4-Validation] Validating IPv4:", ip, "Result:", result)
    return result
}

// 验证IP地址是否为IPv6
function isIpv6Address(ip) {
    const result = ipv6RegExp.test(ip)
    console.log("[IPv6-Validation] Validating IPv6:", ip, "Result:", result)
    return result
}

// 验证IP地址（同时支持IPv4和IPv6）
function isValidIpAddress(ip) {
    const ipv4Result = isIpv4Address(ip)
    const ipv6Result = isIpv6Address(ip)
    const finalResult = ipv4Result || ipv6Result
    console.log("[IP-Validation] Validating IP:", ip, "IPv4:", ipv4Result, "IPv6:", ipv6Result, "Final:", finalResult)
    return finalResult
}

function toVpnTypeEnum(vpnKey) {
    const key = vpnKey.substring(31)
    console.log("toVpnTypeEnum", vpnKey, key)
    return VpnTypeEnum.hasOwnProperty(key) ? VpnTypeEnum[key] : 0x01
}

function toVpnKey(vpnType) {
    for (const key in VpnTypeEnum) {
        if (VpnTypeEnum[key] === vpnType) {
            return "org.freedesktop.NetworkManager." + key
        }
    }
    return "org.freedesktop.NetworkManager.l2tp"
}

function removeTrailingNull(str) {
    return str ? str.toString().replace(/\0+$/, '') : ""
}

function numToIp(num) {
    const ips = [0, 0, 0, 0]
    for (var i = 0; i < 4; i++) {
        ips[i] = (num >> (i * 8)) & 255
    }
    return ips.join('.')
}

function ipToNum(ip) {
    console.log("ipToNum----", ip, typeof ip)
    const ips = ip.split('.')
    let cidr = 0
    let ipNum = 0
    if (ips.length !== 4) {
        return 0
    }
    for (var i = 0; i < ips.length; i++) {
        const ipStr = ips[i]
        const num = parseInt(ipStr, 10)
        ipNum |= ((num & 255) << cidr)
        cidr += 8
    }
    return ipNum
}

function prefixToIp(subnetMask) {
    if (subnetMask < 0 || subnetMask > 32) {
        throw new Error("Subnet mask must be between 0 and 32")
    }

    const maskArray = [255, 255, 255, 255]

    for (var i = 0; i < 4; i++) {
        const byteBits = i * 8 + 8 - subnetMask
        if (byteBits > 0) {
            maskArray[i] = (255 << byteBits) & 255
        }
    }

    return maskArray.join('.')
}

function ipToPrefix(decimalSubnet) {
    const octets = decimalSubnet.split('.')
    let cidr = 0

    for (var j = 0; j < octets.length; j++) {
        const octet = octets[j]
        const num = parseInt(octet, 10)
        for (var i = 0; i < 8; i++) {
            if ((num & (1 << (7 - i))) !== 0) {
                cidr++
            } else {
                break
            }
        }
        if (cidr === 32)
            break
    }
    return cidr
}

function macToStr(mac) {
    return Array.prototype.map.call(new Uint8Array(mac), function (x) {
        return ('00' + x.toString(16)).toUpperCase().slice(-2)
    }).join(':')
}

function strToMac(str) {
    if (str.length === 0)
        return new Uint8Array()
    const arr = str.split(":")
    const hexArr = arr.join("")
    return new Uint8Array(hexArr.match(/[\da-f]{2}/gi).map(function (bb) {
        return parseInt(bb, 16)
    })).buffer
}

// 转为ByteArray并以\0结尾
function strToByteArray(data) {
    if (typeof data === 'string') {
        const arr = []
        for (var i = 0; i < data.length; ++i) {
            let charcode = data.charCodeAt(i)
            if (charcode < 0x80) {
                arr.push(charcode)
            } else if (charcode < 0x800) {
                arr.push(0xc0 | (charcode >> 6), 0x80 | (charcode & 0x3f))
            } else if (charcode < 0xd800 || charcode >= 0xe000) {
                arr.push(0xe0 | (charcode >> 12), 0x80 | ((charcode >> 6) & 0x3f), 0x80 | (charcode & 0x3f))
            } else {
                // Handle surrogate pairs (U+10000 to U+10FFFF)
                i++
                charcode = 0x10000 + (((charcode & 0x3ff) << 10) | (data.charCodeAt(i) & 0x3ff))
                arr.push(0xf0 | (charcode >> 18), 0x80 | ((charcode >> 12) & 0x3f), 0x80 | ((charcode >> 6) & 0x3f), 0x80 | (charcode & 0x3f))
            }
        }
        if (arr[arr.length - 1] !== 0) {
            arr.push(0)
        }
        return new Uint8Array(arr).buffer
    } else if (data instanceof ArrayBuffer) {
        const uint8Array = new Uint8Array(data)
        if (uint8Array[uint8Array.length - 1] !== 0) {
            const newUint8Array = new Uint8Array(uint8Array.length + 1)
            newUint8Array.set(uint8Array)
            newUint8Array[newUint8Array.length - 1] = 0
            return newUint8Array.buffer
        } else {
            return data
        }
    }
    return undefined
}

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
