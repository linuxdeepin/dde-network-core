// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docktestwidget.h"

#include <QApplication>
#include <QScreen>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DockTestWidget testPluginWidget;

    QScreen *mainScreen = qApp->primaryScreen();
    if (mainScreen) {
        testPluginWidget.move(100, mainScreen->size().height() - 100);
    }

    testPluginWidget.show();

    return app.exec();
}
