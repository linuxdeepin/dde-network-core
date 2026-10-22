// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "servicefactory.h"

#include "utils/constants.h"

#include <QDBusConnection>
#include <QDebug>

#include <unistd.h>

static ServiceFactory *serviceFactory = nullptr;

extern "C" int DSMRegister(const char *name, void *data)
{
    // dsm 可能重复注册同一插件；再 new 一套会丢掉旧对象并让旧对象永不释放
    if (serviceFactory) {
        qCWarning(DSM()) << "network plugin already registered, ignored";
        return 0;
    }

    bool isSystem = geteuid() == 0;
    auto connection = reinterpret_cast<QDBusConnection *>(data);
    serviceFactory = new ServiceFactory(isSystem, connection);
    QDBusConnection::RegisterOptions opts = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
    QString path = name;
    path = QString("/%1").arg(path.replace(".", "/"));
    connection->registerObject(path, serviceFactory->serviceObject(), opts);
    return 0;
}

// 该函数用于资源释放
// 非常驻插件必须实现该函数，以防内存泄漏
extern "C" int DSMUnRegister(const char *name, void *data)
{
    (void)name;
    (void)data;

    // 必须同步析构：dsm 在 DSMUnRegister 之后紧接着 QLibrary::unload()，
    // 不会回到事件循环，deleteLater() 永远不会执行。而 NM 线程常驻不退出，
    // 它下一次 Q_EMIT 就会跳进已卸载的代码。
    ServiceFactory *factory = serviceFactory;
    serviceFactory = nullptr;
    delete factory;
    return 0;
}
