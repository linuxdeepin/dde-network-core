// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SETTINGCONFIG_H
#define SETTINGCONFIG_H

#include <QObject>

class SettingConfig : public QObject
{
    Q_OBJECT

public:
    static SettingConfig *instance();
    bool reconnectIfIpConflicted() const;
    bool enableConnectivity() const;
    int connectivityIntervalWhenLimit() const;
    int connectivityCheckInterval() const;
    QStringList networkCheckerUrls() const; // 网络检测地址，用于检测网络连通性
    bool enableOpenPortal() const;               // 是否检测网络认证信息
    bool disableNetwork() const;            // 是否禁用无线网络和蓝牙
    bool enableAccountNetwork() const;      // 是否开启用户私有网络(工银瑞信定制)
    bool disableFailureNotify() const;      // 当网络连接失败后,true:不弹出消息,false:弹出消息
    int resetWifiOSDEnableTimeout() const;  // 重新显示网络连接OSD超时
    int httpRequestTimeout() const;                 // HTTP请求超时时间（秒）
    int httpConnectTimeout() const;                 // HTTP连接超时时间（秒）

    bool supportAutoOpenPortal() const;             // 是否支持自动打开网页
    bool supportPortalPromp() const;                // 是否支持在任务栏或控制中心给出提示
    bool checkDeviceConnection() const;             // 是否检测设备连接
    bool enableLocalNotify() const;

signals:
    void enableConnectivityChanged(bool);
    void connectivityCheckIntervalChanged(int);
    void checkUrlsChanged(QStringList);
    void disableFailureNotifyChanged(bool);
    void resetWifiOSDEnableTimeoutChanged(int);

private slots:
    void onValueChanged(const QString &key);

private:
    explicit SettingConfig(QObject *parent = nullptr);

private:
    bool m_reconnectIfIpConflicted;
    bool m_enableConnectivity;
    int m_connectivityIntervalWhenLimit;
    int m_connectivityCheckInterval;
    QStringList m_networkUrls;
    QString m_protalProcessMode;
    bool m_disabledNetwork;
    bool m_enableAccountNetwork;
    bool m_disableFailureNotify;
    int m_resetWifiOSDEnableTimeout;
    int m_httpRequestTimeout;
    int m_httpConnectTimeout;
};

#endif // SERVICE_H
