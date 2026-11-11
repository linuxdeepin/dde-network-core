// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "settingconfig.h"

#include <DConfig>

#include <QMutexLocker>
#include <QVariant>

static Dtk::Core::DConfig *dConfig = nullptr;

// 当没有进行配置的时候, 则访问我们官网
static const QStringList CheckUrls{
    "https://www.uniontech.com",
};

// dconfig 里两个检测间隔、两个超时的单位都是秒。
// 读取失败、低于下限或超出上限时一律回落到默认值：这些值最终会变成 QTimer 间隔
// 或 curl 超时，0 会退化成忙循环，过大的值在乘 1000 转毫秒时会溢出。
static constexpr int MinTimeSettingSec = 5;
static constexpr int MaxTimeSettingSec = 20 * 24 * 3600; // 20 天，乘 1000 后仍在 int 范围内, 取一个在范围内相对合理的值
static constexpr int DefaultConnectivityCheckIntervalSec = 150;
static constexpr int DefaultConnectivityIntervalWhenLimitSec = 30;
static constexpr int DefaultHttpRequestTimeoutSec = 10;
static constexpr int DefaultHttpConnectTimeoutSec = 8;

static int sanitizedSeconds(const QVariant &value, int fallbackSec)
{
    bool ok = false;
    const int seconds = value.toInt(&ok);
    const bool inRange = (seconds >= MinTimeSettingSec && seconds <= MaxTimeSettingSec);
    return (ok && inRange) ? seconds : fallbackSec;
}

SettingConfig *SettingConfig::instance()
{
    static SettingConfig inst;
    return &inst;
}

bool SettingConfig::reconnectIfIpConflicted() const
{
    QMutexLocker locker(&m_mutex);
    return m_reconnectIfIpConflicted;
}

bool SettingConfig::enableConnectivity() const
{
    QMutexLocker locker(&m_mutex);
    return m_enableConnectivity;
}

int SettingConfig::connectivityIntervalWhenLimit() const
{
    QMutexLocker locker(&m_mutex);
    return m_connectivityIntervalWhenLimit;
}

int SettingConfig::connectivityCheckInterval() const
{
    QMutexLocker locker(&m_mutex);
    return m_connectivityCheckInterval;
}

QStringList SettingConfig::networkCheckerUrls() const
{
    QMutexLocker locker(&m_mutex);
    return m_networkUrls;
}

bool SettingConfig::supportAutoOpenPortal() const
{
    QMutexLocker locker(&m_mutex);
    return m_protalProcessMode.toLower().contains("open");
}

bool SettingConfig::supportPortalPromp() const
{
    QMutexLocker locker(&m_mutex);
    return m_protalProcessMode.toLower().contains("promp");
}

bool SettingConfig::disableNetwork() const
{
    QMutexLocker locker(&m_mutex);
    return m_disabledNetwork;
}

bool SettingConfig::enableAccountNetwork() const
{
    QMutexLocker locker(&m_mutex);
    return m_enableAccountNetwork;
}

bool SettingConfig::disableFailureNotify() const
{
    QMutexLocker locker(&m_mutex);
    return m_disableFailureNotify;
}

bool SettingConfig::disableAllNotify() const
{
    QMutexLocker locker(&m_mutex);
    return m_disableAllNotify;
}

int SettingConfig::resetWifiOSDEnableTimeout() const
{
    QMutexLocker locker(&m_mutex);
    return m_resetWifiOSDEnableTimeout;
}

int SettingConfig::httpRequestTimeout() const
{
    QMutexLocker locker(&m_mutex);
    return m_httpRequestTimeout;
}

int SettingConfig::httpConnectTimeout() const
{
    QMutexLocker locker(&m_mutex);
    return m_httpConnectTimeout;
}

bool SettingConfig::needCheckNetwork() const
{
    QMutexLocker locker(&m_mutex);
    return m_needCheckNetwork;
}

int SettingConfig::reapplyFlags() const
{
    QMutexLocker locker(&m_mutex);
    return m_reapplyFlags;
}

void SettingConfig::onValueChanged(const QString &key)
{
    // 本函数在 DConfig 所属线程执行，访问器可能被 NM 线程/探测线程并发读取，
    // 因此改值持锁、发送信号放在锁外（槽里可能再次读取本对象，持锁发送会自锁）。
    bool emitEnableConnectivity = false;
    bool enableConnectivityValue = false;
    bool emitCheckInterval = false;
    int checkIntervalValue = 0;
    bool emitLimitInterval = false;
    int limitIntervalValue = 0;
    bool emitCheckUrls = false;
    QStringList checkUrlsValue;
    bool emitFailureNotify = false;
    bool failureNotifyValue = false;
    bool emitOsdTimeout = false;
    int osdTimeoutValue = 0;

    {
        QMutexLocker locker(&m_mutex);
        if (key == "reconnectIfIpConflicted") {
            m_reconnectIfIpConflicted = dConfig->value(key).toBool();
        } else if (key == "enableConnectivity") {
            m_enableConnectivity = dConfig->value(key).toBool();
            emitEnableConnectivity = true;
            enableConnectivityValue = m_enableConnectivity;
        } else if (key == QString("ConnectivityIntervalWhenLimit")) {
            m_connectivityIntervalWhenLimit = sanitizedSeconds(dConfig->value("ConnectivityIntervalWhenLimit"), DefaultConnectivityIntervalWhenLimitSec);
            emitLimitInterval = true;
            limitIntervalValue = m_connectivityIntervalWhenLimit;
        } else if (key == QString("ConnectivityCheckInterval")) {
            // 与构造初始化保持一致：成员统一存“秒”，*1000 只在消费点做一次
            m_connectivityCheckInterval = sanitizedSeconds(dConfig->value("ConnectivityCheckInterval"), DefaultConnectivityCheckIntervalSec);
            emitCheckInterval = true;
            checkIntervalValue = m_connectivityCheckInterval;
        } else if (key == QString("NetworkCheckerUrls")) {
            m_networkUrls = dConfig->value("NetworkCheckerUrls").toStringList();
            if (m_networkUrls.isEmpty())
                m_networkUrls = CheckUrls;
            emitCheckUrls = true;
            checkUrlsValue = m_networkUrls;
        } else if (key == QString("disableFailureNotify")) {
            m_disableFailureNotify = dConfig->value("disableFailureNotify").toBool();
            emitFailureNotify = true;
            failureNotifyValue = m_disableFailureNotify;
        } else if (key == QString("disableAllNotify")) {
            m_disableAllNotify = dConfig->value("disableAllNotify", false).toBool();
        } else if (key == QString("resetWifiOSDEnableTimeout")) {
            m_resetWifiOSDEnableTimeout = dConfig->value("resetWifiOSDEnableTimeout").toInt();
            emitOsdTimeout = true;
            osdTimeoutValue = m_resetWifiOSDEnableTimeout;
        } else if (key == QString("httpRequestTimeout")) {
            m_httpRequestTimeout = sanitizedSeconds(dConfig->value("httpRequestTimeout"), DefaultHttpRequestTimeoutSec);
        } else if (key == QString("httpConnectTimeout")) {
            m_httpConnectTimeout = sanitizedSeconds(dConfig->value("httpConnectTimeout"), DefaultHttpConnectTimeoutSec);
        } else if (key == QString("portalProcessMode")) {
            m_protalProcessMode = dConfig->value("portalProcessMode").toString();
        } else if (key == QString("needCheckNetwork")) {
            m_needCheckNetwork = dConfig->value("needCheckNetwork").toBool();
        } else if (key == QString("reapplyFlags")) {
            int val = dConfig->value("reapplyFlags").toInt();
            m_reapplyFlags = (val >= 0 && val <= 2) ? val : 2;
        }
    }

    if (emitEnableConnectivity)
        emit enableConnectivityChanged(enableConnectivityValue);
    if (emitCheckInterval)
        emit connectivityCheckIntervalChanged(checkIntervalValue);
    if (emitLimitInterval)
        emit connectivityIntervalWhenLimitChanged(limitIntervalValue);
    if (emitCheckUrls)
        emit checkUrlsChanged(checkUrlsValue);
    if (emitFailureNotify)
        emit disableFailureNotifyChanged(failureNotifyValue);
    if (emitOsdTimeout)
        emit resetWifiOSDEnableTimeoutChanged(osdTimeoutValue);
}

SettingConfig::SettingConfig(QObject *parent)
    : QObject(parent)
    , m_reconnectIfIpConflicted(false)
    , m_enableConnectivity(true)
    , m_connectivityIntervalWhenLimit(DefaultConnectivityIntervalWhenLimitSec)
    , m_connectivityCheckInterval(DefaultConnectivityCheckIntervalSec)
    , m_protalProcessMode("promp")
    , m_disabledNetwork(false)
    , m_enableAccountNetwork(false)
    , m_disableFailureNotify(false)
    , m_disableAllNotify(false)
    , m_resetWifiOSDEnableTimeout(300)
    , m_httpRequestTimeout(DefaultHttpRequestTimeoutSec)
    , m_httpConnectTimeout(DefaultHttpConnectTimeoutSec)
    , m_needCheckNetwork(true)
    , m_reapplyFlags(2)
{
    if (!dConfig)
        dConfig = Dtk::Core::DConfig::create("org.deepin.dde.network", "org.deepin.dde.network");

    // 连接不依赖 isValid()：dconfig 晚于插件就绪时，单例不应永久停留在默认值
    if (dConfig)
        connect(dConfig, &Dtk::Core::DConfig::valueChanged, this, &SettingConfig::onValueChanged);

    if (dConfig && dConfig->isValid()) {
        QStringList keys = dConfig->keyList();
        if (keys.contains("reconnectIfIpConflicted"))
            m_reconnectIfIpConflicted = dConfig->value("reconnectIfIpConflicted").toBool();

        if (keys.contains("enableConnectivity"))
            m_enableConnectivity = dConfig->value("enableConnectivity").toBool();

        if (keys.contains("ConnectivityIntervalWhenLimit"))
            m_connectivityIntervalWhenLimit = sanitizedSeconds(dConfig->value("ConnectivityIntervalWhenLimit"), DefaultConnectivityIntervalWhenLimitSec);

        if (keys.contains("ConnectivityCheckInterval"))
            m_connectivityCheckInterval = sanitizedSeconds(dConfig->value("ConnectivityCheckInterval"), DefaultConnectivityCheckIntervalSec);

        if (keys.contains("NetworkCheckerUrls"))
            m_networkUrls = dConfig->value("NetworkCheckerUrls").toStringList();

        if (keys.contains("portalProcessMode"))
            m_protalProcessMode = dConfig->value("portalProcessMode").toString();

        if (keys.contains("disabledNetwork"))
            m_disabledNetwork = dConfig->value("disabledNetwork").toBool();

        if (keys.contains("enableAccountNetwork"))
            m_enableAccountNetwork = dConfig->value("enableAccountNetwork").toBool();

        if (keys.contains("resetWifiOSDEnableTimeout"))
            m_resetWifiOSDEnableTimeout = dConfig->value("resetWifiOSDEnableTimeout").toInt();

        if (keys.contains("httpRequestTimeout"))
            m_httpRequestTimeout = sanitizedSeconds(dConfig->value("httpRequestTimeout"), DefaultHttpRequestTimeoutSec);

        if (keys.contains("httpConnectTimeout"))
            m_httpConnectTimeout = sanitizedSeconds(dConfig->value("httpConnectTimeout"), DefaultHttpConnectTimeoutSec);

        if (keys.contains("needCheckNetwork"))
            m_needCheckNetwork = dConfig->value("needCheckNetwork").toBool();

        if (keys.contains("disableAllNotify"))
            m_disableAllNotify = dConfig->value("disableAllNotify", false).toBool();

        m_disableFailureNotify = dConfig->value("disableFailureNotify", false).toBool();

        if (keys.contains("reapplyFlags")) {
            int val = dConfig->value("reapplyFlags").toInt();
            if (val >= 0 && val <= 2)
                m_reapplyFlags = val;
            else
                m_reapplyFlags = 2;
        }
    }
    if (m_networkUrls.isEmpty())
        m_networkUrls = CheckUrls;
}
