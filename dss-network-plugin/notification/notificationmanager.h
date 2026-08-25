// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QString>

class QObject;
class BubbleManager;
class NotificationManager
{
public:
    static uint Notify(const QString &icon, const QString &body);
    static void InstallEventFilter(QObject *obj);

protected:
    static BubbleManager *BubbleManagerInstance();
};

#endif // NOTIFICATIONMANAGER_H
