// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notificationmanager.h"

#include "bubblemanager.h"

BubbleManager *NotificationManager::BubbleManagerInstance()
{
    static auto *s_bubbleManager = new BubbleManager();
    return s_bubbleManager;
}
uint NotificationManager::Notify(const QString &icon, const QString &body)
{
    static uint replacesId = 0;
    replacesId = BubbleManagerInstance()->Notify("dde-control-center", replacesId, icon, "", body);
    return replacesId;
}

void NotificationManager::InstallEventFilter(QObject *obj)
{
    obj->installEventFilter(NotificationManager::BubbleManagerInstance());
}
