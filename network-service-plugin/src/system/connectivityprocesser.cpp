// SPDX-FileCopyrightText: 2018-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connectivityprocesser.h"
#include "connectivitychecker.h"
#include "settingconfig.h"

#include <QDebug>
#include <QThread>

using namespace network::systemservice;

namespace {

// ConnectivityChecker 的亲和线程可能不是本对象的线程（NMConnectionvityChecker
// 运行在 NM 线程），删除必须回到它自己的线程执行。
void deleteChecker(ConnectivityChecker *checker)
{
    if (!checker) {
        return;
    }
    if (checker->thread() == QThread::currentThread()) {
        delete checker;
        return;
    }
    QMetaObject::invokeMethod(checker, [checker] { delete checker; }, Qt::BlockingQueuedConnection);
}

} // namespace
ConnectivityProcesser::ConnectivityProcesser(QObject *parent)
    : QObject(parent)
{
    SettingConfig *config = SettingConfig::instance();
    connect(config, &SettingConfig::enableConnectivityChanged, this, &ConnectivityProcesser::onEnableConnectivityChanged);
    m_checker = createConnectivityChecker(config->enableConnectivity());
}

ConnectivityProcesser::~ConnectivityProcesser()
{
    deleteChecker(m_checker);
    m_checker = nullptr;
}

void ConnectivityProcesser::checkConnectivity()
{
    m_checker->checkConnectivity();
}

network::service::Connectivity ConnectivityProcesser::connectivity() const
{
    return m_checker->connectivity();
}

QString ConnectivityProcesser::portalUrl() const
{
    return m_checker->portalUrl();
}

ConnectivityChecker *ConnectivityProcesser::createConnectivityChecker(bool enableConnectivity)
{
    ConnectivityChecker *checker = nullptr;
    if (enableConnectivity) {
        qCDebug(DSM) << "uses local connectivity checker";
        // LocalConnectionvityChecker 持有跨线程对象，生命周期由 m_checker 独占
        LocalConnectionvityChecker *localChecker = new LocalConnectionvityChecker();
        connect(localChecker, &LocalConnectionvityChecker::portalDetected, this, &ConnectivityProcesser::portalDetected);
        checker = localChecker;
    } else {
        qCDebug(DSM) << "uses nm connectivity checker";
        // NMConnectionvityChecker 会被 moveToThread 到 NM 线程，不能带 parent
        checker = new NMConnectionvityChecker();
    }

    connect(checker, &ConnectivityChecker::connectivityChanged, this, &ConnectivityProcesser::connectivityChanged);

    return checker;
}

void ConnectivityProcesser::onEnableConnectivityChanged(bool enableConnectivity)
{
    deleteChecker(m_checker);
    m_checker = createConnectivityChecker(enableConnectivity);
}
