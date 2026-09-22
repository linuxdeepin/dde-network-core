// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netutils.h"

#include <QMetaType>
#include <QDBusMetaType>
#include <QLocale>
#include <QStringDecoder>

namespace dde {
namespace network {

namespace {

// 语言 → 候选编码(合并 nm_utils.c 的 5 位/2 位语言表,顺序贴近)
QStringList langEncodings(const QLocale &l)
{
    if (l.language() == QLocale::Chinese) {
        switch (l.territory()) {
        case QLocale::HongKong:
        case QLocale::Macau:
        case QLocale::Taiwan:
            return { QStringLiteral("Big5"), QStringLiteral("EUC-TW") };
        default:
            // 简体中文: GB18030 是 GBK/GB2312 超集,一条覆盖
            return { QStringLiteral("GB18030"), QStringLiteral("GB2312") };
        }
    }
    if (l.language() == QLocale::Japanese)
        return { QStringLiteral("EUC-JP"), QStringLiteral("Shift_JIS"), QStringLiteral("ISO-2022-JP") };
    if (l.language() == QLocale::Korean)
        return { QStringLiteral("EUC-KR"), QStringLiteral("ISO-2022-KR"), QStringLiteral("Johab") };
    if (l.language() == QLocale::Russian || l.language() == QLocale::Belarusian
            || l.language() == QLocale::Bulgarian || l.language() == QLocale::Macedonian)
        return { QStringLiteral("KOI8-R"), QStringLiteral("windows-1251"), QStringLiteral("ISO-8859-5") };
    if (l.language() == QLocale::Ukrainian)
        return { QStringLiteral("KOI8-U"), QStringLiteral("KOI8-R"), QStringLiteral("windows-1251") };

    switch (l.language()) {
    case QLocale::Arabic:
        return { QStringLiteral("ISO-8859-6"), QStringLiteral("windows-1256") };
    case QLocale::Czech:
    case QLocale::Croatian:
    case QLocale::Hungarian:
    case QLocale::Polish:
    case QLocale::Romanian:
    case QLocale::Serbian:
    case QLocale::Slovak:
    case QLocale::Slovenian:
        return { QStringLiteral("ISO-8859-2"), QStringLiteral("windows-1250") };
    case QLocale::Greek:
        return { QStringLiteral("ISO-8859-7"), QStringLiteral("windows-1253") };
    case QLocale::Estonian:
    case QLocale::Lithuanian:
    case QLocale::Latvian:
        return { QStringLiteral("ISO-8859-4"), QStringLiteral("windows-1257") };
    case QLocale::Hebrew:
        return { QStringLiteral("ISO-8859-8"), QStringLiteral("windows-1255") };
    case QLocale::Thai:
        return { QStringLiteral("ISO-8859-11"), QStringLiteral("windows-874") };
    case QLocale::Turkish:
        return { QStringLiteral("ISO-8859-9"), QStringLiteral("windows-1254") };
    default:
        break;
    }
    return {};
}

// 候选 = 语言表 + { 系统 charset, iso-8859-1, windows-1251 }(同 _system_encodings_get)
QStringList decodeCandidates(const QLocale &l)
{
    QStringList list = langEncodings(l);
    auto sys = QStringDecoder(QStringDecoder::System);
    if (sys.isValid()) {
        QString n = QString::fromLatin1(sys.name());
        if (!list.contains(n))
            list << n;
    }
    for (const char *e : { "ISO-8859-1", "windows-1251" }) {
        const QString n = QString::fromLatin1(e);
        if (!list.contains(n))
            list << n;
    }
    return list;
}

} // namespace

/**
 * 将 SSID 原始字节解码为显示用 UTF-8 字符串。
 *
 * 实现参考 NetworkManager 上游的 nm_utils_ssid_to_utf8()
 * (network-manager/src/libnm-core-impl/nm-utils.c),采用相同的解码策略:
 *   1) 输入已是合法 UTF-8 → 原样返回(对应 nm_utils 的 g_utf8_validate 分支)
 *   2) 按系统 locale 的语言表尝试候选编码,第一个完整解码成功者生效
 *      (对应 nm_utils 的 _system_encodings_for_lang 语言→编码表,
 *       如 zh_CN → euc-cn/gb2312/gb18030,GB18030 为 GBK 超集)
 *   3) 语言表无命中时回退到默认候选: 系统 charset + iso-8859-1 + windows-1251
 *      (对应 nm_utils 的 _system_encodings_get_default)
 *   4) 全部失败 → 可打印 ASCII,其余字节替换为 '?'
 *      (对应 nm_utils 的 g_convert_with_fallback/g_strcanon 兜底)
 *
 * 与 nm_utils 的区别: 使用 Qt 的 QStringDecoder(内部 ICU)做解码,
 * 不依赖 glib/g_convert,亦不引入 libnm 头文件依赖。
 * 注意: 与 nm_utils_ssid_to_utf8 相同,结果仅用于显示,不可用于字节匹配
 * (匹配请使用 AccessPoints::rawSsid() 原始字节)。
 */
QString ssidToUtf8(const QByteArray &raw)
{
    if (raw.isEmpty())
        return {};

    // 1) 已是 UTF-8 → 直接返回
    QStringDecoder u8(QStringDecoder::Utf8);
    QString out = u8.decode(raw);
    if (!u8.hasError())
        return out;

    // 2) 按系统 locale 的候选表逐个试(先验判断编码)
    const QLocale sys = QLocale::system();
    for (const QString &enc : decodeCandidates(sys)) {
        QStringDecoder d(enc);
        if (!d.isValid())
            continue;
        d.resetState();
        out = d.decode(raw);
        if (!d.hasError())
            return out;
    }

    // 3) 兜底: 可打印 ASCII,其余 '?'
    QString ascii;
    ascii.reserve(raw.size());
    for (char c : raw)
        ascii += (c >= 0x20 && c <= 0x7E) ? QChar(c) : QChar('?');
    return ascii;
}

bool ssidBytesMatch(const QByteArray &connSsid, const QByteArray &rawSsid, const QString &displaySsid)
{
    // 优先按原始字节精确匹配(与 NM 侧字节比较一致),rawSsid 为空(如 DSS 后端)
    // 时回退按 UTF-8 显示名匹配历史连接
    return (!rawSsid.isEmpty() && connSsid == rawSsid) || connSsid == displaySsid.toUtf8();
}

QByteArray ssidForSave(const QByteArray &rawSsid, const QString &displaySsid)
{
    // 保存连接时优先存 AP 原始字节(保证与 NM 字节匹配一致),
    // rawSsid 为空(如隐藏网络)时回退 UTF-8 显示名
    return rawSsid.isEmpty() ? displaySsid.toUtf8() : rawSsid;
}

Connectivity connectivityValue(uint sourceConnectivity)
{
    switch (sourceConnectivity) {
    case 0:     return Connectivity::Unknownconnectivity;
    case 1:     return Connectivity::Noconnectivity;
    case 2:     return Connectivity::Portal;
    case 3:     return Connectivity::Limited;
    case 4:     return Connectivity::Full;
    default:    break;
    }

    return Connectivity::Unknownconnectivity;
}

DeviceStatus convertDeviceStatus(int sourceDeviceStatus)
{
    switch (sourceDeviceStatus) {
    case 0:     return DeviceStatus::Unknown;
    case 10:    return DeviceStatus::Unmanaged;
    case 20:    return DeviceStatus::Unavailable;
    case 30:    return DeviceStatus::Disconnected;
    case 40:    return DeviceStatus::Prepare;
    case 50:    return DeviceStatus::Config;
    case 60:    return DeviceStatus::Needauth;
    case 70:    return DeviceStatus::IpConfig;
    case 80:    return DeviceStatus::IpCheck;
    case 90:    return DeviceStatus::Secondaries;
    case 100:   return DeviceStatus::Activated;
    case 110:   return DeviceStatus::Deactivation;
    case 120:   return DeviceStatus::Failed;
    default:    break;
    }

    return DeviceStatus::Unknown;
}

ConnectionStatus convertConnectionStatus(int sourceConnectionStatus)
{
    switch (sourceConnectionStatus) {
    case 0:     return ConnectionStatus::Unknown;
    case 1:     return ConnectionStatus::Activating;
    case 2:     return ConnectionStatus::Activated;
    case 3:     return ConnectionStatus::Deactivating;
    case 4:     return ConnectionStatus::Deactivated;
    default:    return ConnectionStatus::Unknown;
    }
}

ConnectionStatus convertStateFromNetworkManager(NetworkManager::ActiveConnection::State state)
{
    switch (state) {
    case NetworkManager::ActiveConnection::State::Activated:
        return ConnectionStatus::Activated;
    case NetworkManager::ActiveConnection::State::Activating:
        return ConnectionStatus::Activating;
    case NetworkManager::ActiveConnection::State::Deactivated:
        return ConnectionStatus::Deactivated;
    case NetworkManager::ActiveConnection::State::Deactivating:
        return ConnectionStatus::Deactivating;
    default:
        break;
    }
    return ConnectionStatus::Deactivated;
}

}
}
