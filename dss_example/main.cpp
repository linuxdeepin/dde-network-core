// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dssscreenmanager.h"
#include "dsstestwidget.h"
#include "networkcontroller.h"

#include <DGuiApplicationHelper>

#include <QApplication>

using namespace std;
using namespace dde::network;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString applicationName("example-lock");
    if (argc > 1) {
        applicationName = QString("%1-%2").arg("example").arg(argv[1]);
    }

    app.setApplicationName(applicationName);

    DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
    DPalette pa = DGuiApplicationHelper::instance()->applicationPalette();
    pa.setColor(QPalette::Normal, DPalette::WindowText, QColor("#FFFFFF"));
    pa.setColor(QPalette::Normal, DPalette::Text, QColor("#FFFFFF"));
    pa.setColor(QPalette::Normal, DPalette::AlternateBase, QColor(0, 0, 0, 76));
    pa.setColor(QPalette::Normal, DPalette::Button, QColor(255, 255, 255, 76));
    pa.setColor(QPalette::Normal, DPalette::Light, QColor(255, 255, 255, 76));
    pa.setColor(QPalette::Normal, DPalette::Dark, QColor(255, 255, 255, 76));
    pa.setColor(QPalette::Normal, DPalette::ButtonText, QColor("#FFFFFF"));
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::WindowText, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Text, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::AlternateBase, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Button, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Light, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Dark, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::ButtonText, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::instance()->setApplicationPalette(pa);

    DssScreenManager dssManager;
    dssManager.showWindow();

    return app.exec();
}
