// SPDX-FileCopyrightText: 2025-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NetworkEnabledConfig_H
#define NetworkEnabledConfig_H
#include <QMap>
#include <QMutex>

namespace network {
namespace systemservice {
class NetworkEnabledConfig
{
public:
    NetworkEnabledConfig();
    ~NetworkEnabledConfig();

    bool vpnEnabled() const;
    void setVpnEnabled(bool enabled);
    bool deviceEnabled(const QString &dev);
    void setDeviceEnabled(const QString &dev, bool enabled);
    void removeDeviceEnabled(const QString &dev);

    void loadConfig();
    QString saveConfig();
    void setConnectionInfo(const QString &dev, const QString &uuid);
    QString connectionUuid(const QString &dev) const;

private:
    // 访问点分布在 NM 线程（写）与插件线程（NetworkDBus::VpnEnabled 读），
    // 保护本进程自己的容器
    mutable QMutex m_mutex;
    QMap<QString, QVariant> m_map;
};
} // namespace systemservice} // namespace network#endif // NetworkEnabledConfig_H