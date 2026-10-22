// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SYSTEMCONTAINER_H
#define SYSTEMCONTAINER_H

#include <QObject>


namespace network {
namespace systemservice {

class SystemIPConflict;
class ConnectivityProcesser;
class NetworkInitialization;

class SystemContainer : public QObject
{
    Q_OBJECT

public:
    SystemContainer(QObject *parent = Q_NULLPTR);
    ~SystemContainer();
    SystemIPConflict *ipConfilctedChecker() const;
    ConnectivityProcesser *connectivityProcesser() const;
    NetworkInitialization *initializator() const;

private:
    // 前两个运行在 NM 线程（带 parent 无法 moveToThread），析构需排队回各自线程，
    // 故不带 parent、由析构函数显式管理
    SystemIPConflict *m_ipConflictHandler;
    ConnectivityProcesser *m_connectivityHelper;
    NetworkInitialization *m_initializator;
};

}
}

#endif // NETWORKSERVICE_H
