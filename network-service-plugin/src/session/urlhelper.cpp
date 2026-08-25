// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "urlhelper.h"
#include "desktopmonitor.h"
#include "constants.h"
#include "settingconfig.h"

#include <QProcessEnvironment>
#include <QProcess>
#include <QUrl>
#include <QDebug>

using namespace network::sessionservice;

UrlHelper::UrlHelper(DesktopMonitor *desktopMonitor, QObject *parent)
    : QObject(parent)
    , m_desktopMonitor(desktopMonitor)
{
}

UrlHelper::~UrlHelper()
{
}

void UrlHelper::openUrl(const QString &url)
{
    QString displayEnvironment = m_desktopMonitor->displayEnvironment();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!displayEnvironment.isEmpty())
        env.insert("DISPLAY", displayEnvironment);

    QProcess process;
    process.setProcessEnvironment(env);
    process.start("xdg-open", QStringList() << url);
    process.waitForFinished();
}
