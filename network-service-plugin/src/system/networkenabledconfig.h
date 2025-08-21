// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NetworkEnabledConfig_H
#define NetworkEnabledConfig_H
#include <QMap>

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

private:
    QMap<QString, QVariant> m_map;
};
} // namespace systemservice
} // namespace network
#endif // NetworkEnabledConfig_H
