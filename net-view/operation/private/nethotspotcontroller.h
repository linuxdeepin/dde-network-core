// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETHOTSPOTCONTROLLER_H
#define NETHOTSPOTCONTROLLER_H

#include <QObject>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace dde {
namespace network {
class HotspotController;
class WirelessDevice;

class NetHotspotController : public QObject
{
    Q_OBJECT
public:
    explicit NetHotspotController(QObject *parent = nullptr);
    bool isEnabled() const;
    bool enabledable() const;
    bool deviceEnabled();
    const QVariantMap &config() const;
    const QStringList &optionalDevice() const;
    const QStringList &optionalDevicePath() const;
    const QStringList &shareDevice() const;

Q_SIGNALS:
    void enabledChanged(const bool);
    void enabledableChanged(const bool);
    void deviceEnabledChanged(const bool);
    void configChanged(const QVariantMap &config);
    void optionalDeviceChanged(const QStringList &optionalDevice);
    void optionalDevicePathChanged(const QStringList &optionalDevicePath);
    void shareDeviceChanged(const QStringList &shareDevice);

private Q_SLOTS:
    void updateEnabled();
    void updateEnabledable();
    void updateData();
    void updateConfig();

private:
    HotspotController *m_hotspotController;
    bool m_isEnabled;
    bool m_enabledable;
    bool m_deviceEnabled;
    QVariantMap m_config;
    QStringList m_shareDevice;
    QStringList m_optionalDevice;
    QStringList m_optionalDevicePath;
    QTimer *m_updatedTimer;
};
} // namespace network
} // namespace dde
#endif // NETHOTSPOTCONTROLLER_H
