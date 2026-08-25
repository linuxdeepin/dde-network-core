// SPDX-FileCopyrightText: 2011 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dssscreenmanager.h"

#include "dsstestwidget.h"
#include "networkmodule.h"

#include <QGuiApplication>
#include <QScreen>

DssScreenManager::DssScreenManager(QObject *parent)
    : QObject(parent)
    , m_netModule(new dde::network::NetworkPlugin(this))
{
    m_netModule->init();
    initConnection();
    initScreen();
}

DssScreenManager::~DssScreenManager()
{
    qDeleteAll(m_screenWidget);
    m_screenWidget.clear();
}

void DssScreenManager::showWindow()
{
    for (auto it = m_screenWidget.constBegin(); it != m_screenWidget.constEnd(); it++) {
        showWindow(it.key(), it.value());
    }
}

void DssScreenManager::initConnection()
{
    connect(qApp, &QGuiApplication::screenAdded, this, &DssScreenManager::onScreenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &DssScreenManager::onScreenRemoved);
}

void DssScreenManager::initScreen()
{
    QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        DssTestWidget *testWidget = new DssTestWidget(m_netModule);
        m_screenWidget[screen] = testWidget;
    }
}

void DssScreenManager::showWindow(QScreen *screen, DssTestWidget *testWidget)
{
    const QRect screenGeometry = screen->geometry();
    testWidget->resize(330, 800);
    testWidget->move(screenGeometry.x() + (screenGeometry.width() - testWidget->width()) / 2, (screenGeometry.height() - testWidget->height()) / 2);
    testWidget->show();
}

void DssScreenManager::onScreenAdded(QScreen *screen)
{
    DssTestWidget *testWidget = new DssTestWidget(m_netModule);
    m_screenWidget[screen] = testWidget;
    showWindow(screen, testWidget);
}

void DssScreenManager::onScreenRemoved(QScreen *screen)
{
    if (m_screenWidget.contains(screen)) {
        DssTestWidget *testWidget = m_screenWidget[screen];
        m_screenWidget.remove(screen);
        testWidget->deleteLater();
    }
}
