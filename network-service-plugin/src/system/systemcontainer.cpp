// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "systemcontainer.h"
#include "systemipconfilct.h"
#include "connectivityprocesser.h"
#include "networkinitialization.h"
#include "nmaccess.h"

using namespace network::systemservice;

SystemContainer::SystemContainer(QObject *parent)
    : QObject (parent)
    // SystemIPConflict / NetworkInitialization 会被 moveToThread 到 NM 线程，
    // 带 parent 的对象无法迁移，因此不带 parent、由析构函数显式管理。
    , m_ipConflictHandler(new SystemIPConflict())
    , m_connectivityHelper(new ConnectivityProcesser(this))
    , m_initializator(new NetworkInitialization())
{
}

SystemContainer::~SystemContainer()
{
    // SystemIPConflict / NetworkInitialization 已 moveToThread 到 NM 线程
    //（进程级常驻，永不退出）。若在插件线程直接 delete 会跨线程销毁对象；
    // NM 线程队列中可能仍有派发给它们的 D-Bus 回调 / 定时器事件
    // → use-after-free。删除必须排到 NM 线程执行。
    SystemIPConflict *ipc = m_ipConflictHandler;
    NetworkInitialization *init = m_initializator;
    m_ipConflictHandler = nullptr;
    m_initializator = nullptr;

    if (isNetworkManagerThread()) {
        delete ipc;
        delete init;
        return;
    }

    if (ipc || init) {
        QMetaObject::invokeMethod(networkManagerContext(), [ipc, init] {
            delete ipc;
            delete init;
        }, Qt::BlockingQueuedConnection);
    }
}

SystemIPConflict *SystemContainer::ipConfilctedChecker() const
{
    return m_ipConflictHandler;
}

ConnectivityProcesser *SystemContainer::connectivityProcesser() const
{
    return m_connectivityHelper;
}

NetworkInitialization *SystemContainer::initializator() const
{
    return m_initializator;
}
