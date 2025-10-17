// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networksecretdialog.h"
#if (defined QT_DEBUG) && (defined CHECK_ACCESSIBLENAME)
#include "../common/accessibilitycheckerex.h"
#endif

#include <DLog>
#include <DApplication>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTimer>
#include <QTranslator>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("dde-network-secret-dialog");

    DLogManager::registerFileAppender();

    QTranslator translator;
    if (translator.load(QLocale::system(), "network-service-plugin", "_", "/usr/share/deepin-service-manager/network-service/translations")) {
        app.installTranslator(&translator);
        app.loadTranslator();
    }

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("filepath",
                                 "the file of json format data.\n"
                                 "data will be read from STDIN if filepath is not specified.",
                                 "[filepath]");
    parser.process(app);
    const QStringList &posArguments = parser.positionalArguments();
    QJsonDocument jsonDoc;
    QFile file;
    if (posArguments.isEmpty()) {
        qDebug() << "read json data from STDIN";
        if (!file.open(stdin, QFile::ReadOnly)) {
            qWarning() << "read from STDIN failed";
            return -2;
        }
    } else {
        file.setFileName(posArguments.first());
        if (!file.open(QFile::ReadOnly)) {
            qWarning() << "file:" << file.fileName()<< "open failed";
            return -1;
        }
    }
    QByteArray arr = file.readAll();
    jsonDoc = QJsonDocument::fromJson(arr);
    if (jsonDoc.isEmpty()) {
        qWarning() << "invalid json data.";
        return -3;
    }

    file.close();
    // GetSecrets超时时间120000，参考NM中nm-secret-agent.c中函数nm_secret_agent_get_secrets里的设置
    QTimer::singleShot(120000, &app, &DApplication::quit);

    NetworkSecretDialog dialog(jsonDoc);
#if (defined QT_DEBUG) && (defined CHECK_ACCESSIBLENAME)
    AccessibilityCheckerEx checker;
    checker.setOutputFormat(DAccessibilityChecker::FullFormat);
    checker.start();
#endif
    dialog.exec();

    return 0;
    // app.exit(0);
}
