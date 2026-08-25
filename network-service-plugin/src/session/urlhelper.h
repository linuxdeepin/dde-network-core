// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef URLHELPER_H
#define URLHELPER_H

#include <QObject>

#include <NetworkManagerQt/VpnConnection>

class QTimer;

namespace network {
namespace sessionservice {

class DesktopMonitor;

class UrlHelper : public QObject
{
    Q_OBJECT

public:
    explicit UrlHelper(DesktopMonitor *desktopMonitor, QObject *parent = nullptr);
    ~UrlHelper() override;
    void openUrl(const QString &url);

private:
    DesktopMonitor *m_desktopMonitor;
};

}
}

#endif // SERVICE_H
