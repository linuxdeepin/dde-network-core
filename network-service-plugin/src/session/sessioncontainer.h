// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SESSIONCONTAINER_H
#define SESSIONCONTAINER_H

#include <QObject>

namespace network {
namespace sessionservice {

class SessionIPConflict;
class DesktopMonitor;

class SessionContainer : public QObject
{
    Q_OBJECT

public:
    SessionContainer(QObject *parent = Q_NULLPTR);
    ~SessionContainer();
    SessionIPConflict *ipConfilctedChecker() const;

private:
    void initConnection();
    void initEnvornment();
    void checkPortalUrl();
    void enterDesktop();
    void leaveDesktop();
    void openPortalUrl(const QString &url);

private slots:
    void onIPConflictChanged(const QString &devicePath, const QString &ip, bool conflicted);
    void onPortalDetected(const QString &url);
    void onProxyMethodChanged(const QString &method);
    void onDesktopChanged(bool isLogin);

private:
    SessionIPConflict *m_ipConflictHandler;
    DesktopMonitor *m_desktopMonitor;
};

}
}

#endif // NETWORKSERVICE_H
