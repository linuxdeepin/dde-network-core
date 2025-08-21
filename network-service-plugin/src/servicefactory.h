// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef SERVICEFACTORY_H
#define SERVICEFACTORY_H

#include <QObject>

class SystemContainer;
class QDBusConnection;

class ServiceFactory : public QObject
{
    Q_OBJECT

public:
    explicit ServiceFactory(bool isSystem, QDBusConnection *dbusConnection, QObject *parent = nullptr);
    QObject *serviceObject();

private:
    QObject *createServiceObject(bool isSystem);

private:
    QObject *m_serviceObject;
    QDBusConnection *m_dbusConnection;
    bool m_isSystem;
};

#endif // SERVICE_H
