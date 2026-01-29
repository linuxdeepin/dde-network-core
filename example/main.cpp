// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "service/serviceqtdbus.h"
#include "dccplugintestwidget.h"

#include <QApplication>
#include <QTranslator>
#include <DLog>

#include <unistd.h>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Dtk::Core::DLogManager::registerConsoleAppender();
    QTranslator t;
    if (t.load(":/qm/network_cn_qm"))
        app.installTranslator(&t);

    if (argc > 1) {
        if (QString(argv[1]) == "dccPlug") {
            DccPluginTestWidget testPluginWidget;
            testPluginWidget.resize(1024, 720);
            testPluginWidget.show();
            return app.exec();
        }
        if (QString(argv[1]) == "servicemanager") {
            // 新建deepinServiceManager的服务
            QString fileName = QString("%1%2/%3").arg(SERVICE_CONFIG_DIR).arg(geteuid() == 0 ? "system" : "user")
                    .arg(geteuid() == 0 ? "plugin-system-network.json" : "plugin-session-network.json");
            Policy policy(nullptr);
            policy.parseConfig(fileName);
            ServiceQtDBus service;
            service.init(geteuid() == 0 ? QDBusConnection::BusType::SystemBus : QDBusConnection::BusType::SessionBus, &policy);
            return app.exec();
        }
    }

    return app.exec();
}
