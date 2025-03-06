// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dsstestwidget.h"

#include <QApplication>
#include <QScreen>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    DssTestWidget testPluginWidget;
    QScreen *deskdop = QApplication::primaryScreen();
    testPluginWidget.move((deskdop->size().width() - testPluginWidget.width()) / 2, (deskdop->size().height() - testPluginWidget.height()) / 2);

    testPluginWidget.show();

    return app.exec();
}
