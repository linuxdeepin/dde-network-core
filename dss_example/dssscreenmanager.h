// SPDX-FileCopyrightText: 2011 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSSSCREENMANAGER_H
#define DSSSCREENMANAGER_H

#include <QMap>
#include <QObject>

class DssTestWidget;
class QScreen;

namespace dde {
namespace network {
class NetworkPlugin;
} // namespace network
} // namespace dde

class DssScreenManager : public QObject
{
    Q_OBJECT

public:
    explicit DssScreenManager(QObject *parent = nullptr);
    ~DssScreenManager();
    void showWindow();

private:
    void initConnection();
    void initScreen();
    void showWindow(QScreen *screen, DssTestWidget *testWidget);

protected:
    void onScreenAdded(QScreen *screen);
    void onScreenRemoved(QScreen *screen);

private:
    QMap<QScreen *, DssTestWidget *> m_screenWidget;
    dde::network::NetworkPlugin *m_netModule;
};

#endif // DSSSCREENMANAGER_H
