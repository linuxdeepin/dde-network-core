// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "servicefactory.h"

#include <QDBusConnection>
#include <QDebug>

#include <unistd.h>

static ServiceFactory *serviceFactory = nullptr;

extern "C" int DSMRegister(const char *name, void *data)
{
    serviceFactory = new ServiceFactory(QString(name).endsWith("SystemNetwork"));
    QDBusConnection::RegisterOptions opts = QDBusConnection::ExportAllSlots
            | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;

    QString path = name;
    path = QString("/%1").arg(path.replace(".", "/"));
    auto connection = reinterpret_cast<QDBusConnection *>(data);
    connection->registerObject(path, serviceFactory->serviceObject(), opts);
    return 0;
}

// 该函数用于资源释放
// 非常驻插件必须实现该函数，以防内存泄漏
extern "C" int DSMUnRegister(const char *name, void *data)
{
    (void)name;
    (void)data;
    serviceFactory->deleteLater();
    serviceFactory = nullptr;
    return 0;
}
