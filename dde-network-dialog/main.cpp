// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "localclient.h"
#include "utils.h"
#include "thememanager.h"
#include "networkcontroller.h"

#include <DApplication>
#include <DPlatformTheme>
#include <DLog>

#include <QWindow>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTranslator>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>

using namespace Dtk::Widget;

int main(int argc, char **argv)
{
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }
    DApplication *app = DApplication::globalApplication(argc, argv);
    app->setOrganizationName("deepin");
    app->setApplicationName("dde-network-dialog");
    app->setQuitOnLastWindowClosed(true);
    app->setAttribute(Qt::AA_UseHighDpiPixmaps);

    qApp->setApplicationDisplayName("NetworkDialog");
    qApp->setApplicationDescription("network dialog");

#ifdef DEBUG_LOG
    DTK_CORE_NAMESPACE::DLogManager::setlogFilePath("/tmp/dde-network-dialog.log");
    DTK_CORE_NAMESPACE::DLogManager::registerConsoleAppender();
    DTK_CORE_NAMESPACE::DLogManager::registerFileAppender();
#endif

    QCommandLineOption showOption(QStringList() << "s", "show config", "config");
    QCommandLineOption waitOption(QStringList() << "w", "wait wep-key"); // 等待模式，密码输入后返回给调用者并退出。否则不退出尝试联网
    QCommandLineOption connectPathOption(QStringList() << "c", "connect wireless ", "path");
    QCommandLineOption devOption(QStringList() << "n", "network device", "device");

    QCommandLineParser parser;
    parser.setApplicationDescription("DDE Network Dialog");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(showOption);
    parser.addOption(waitOption);
    parser.addOption(connectPathOption);
    parser.addOption(devOption);
    parser.process(*qApp);

    if (parser.isSet(showOption)) {
        QString config = parser.value(showOption);
        LocalClient::instance()->showPosition(nullptr, config.toUtf8());
    }
    if (parser.isSet(connectPathOption)) {
        QString dev = parser.value(devOption);
        QString ssid = parser.value(connectPathOption);
        LocalClient::instance()->waitPassword(dev, ssid, parser.isSet(waitOption));
    } else {
        LocalClient::instance()->showWidget();
    }
    return app->exec();
}
