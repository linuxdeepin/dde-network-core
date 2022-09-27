// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bubblemanager.h"
#include "bubble.h"
#include "notificationentity.h"
#include "constants.h"

#include <DDesktopServices>
#include <DWindowManagerHelper>
#include <DPlatformWindowHandle>

#include <QStringList>
#include <QVariantMap>
#include <QTimer>
#include <QDebug>
#include <QScreen>
#include <QDBusContext>
#include <QDateTime>
#include <QGSettings>
#include <QDesktopWidget>

#include <algorithm>

#include <com_deepin_daemon_display.h>
#include <com_deepin_daemon_display_monitor.h>

using DisplayInter = com::deepin::daemon::Display;
using MonitorInter = com::deepin::daemon::display::Monitor;

BubbleManager::BubbleManager(QObject *parent)
    : QObject(parent)
    , m_gestureInter(new GestureInter("com.deepin.daemon.Gesture"
                                      , "/com/deepin/daemon/Gesture"
                                      , QDBusConnection::systemBus()
                                      , this))
    , m_trickTimer(new QTimer(this))
    , m_parentWidget(QApplication::allWidgets().first()->window())
{
    m_trickTimer->setInterval(300);
    m_trickTimer->setSingleShot(true);

    initConnections();
    geometryChanged();
}

BubbleManager::~BubbleManager()
{
    if (!m_bubbleList.isEmpty()) qDeleteAll(m_bubbleList);

    m_oldEntities.clear();
}

void BubbleManager::CloseNotification(uint id)
{
    QString str_id = QString::number(id);
    foreach (auto bubble, m_bubbleList) {
        if (bubble->entity()->replacesId() == str_id) {
            bubble->close();
            m_bubbleList.removeOne(bubble);
            qDebug() << "CloseNotification : id" << str_id;
        }
    }

    foreach (auto notify, m_oldEntities) {
        if (notify->replacesId() == str_id) {
            m_oldEntities.removeOne(notify);
            qDebug() << "CloseNotification : id" << str_id;
        }
    }
}

QStringList BubbleManager::GetCapabilities()
{
    QStringList result;
    result << "action-icons" << "actions" << "body" << "body-hyperlinks" << "body-markup";

    return result;
}

QString BubbleManager::GetServerInformation(QString &name, QString &vender, QString &version)
{
    name = QString("DeepinNotifications");
    vender = QString("Deepin");
    version = QString("2.0");

    return QString("1.2");
}

uint BubbleManager::Notify(const QString &appName, uint replacesId,
                           const QString &appIcon, const QString &summary,
                           const QString &body)
{
    QString strBody = body;
    strBody.replace(QLatin1String("\\\\"), QLatin1String("\\"), Qt::CaseInsensitive);

    EntityPtr notification = std::make_shared<NotificationEntity>(appName, QString(), appIcon,
                                                                  summary, strBody);
    notification->setTime(QString::number(QDateTime::currentMSecsSinceEpoch()));
    notification->setReplacesId(QString::number(replacesId));
    notification->setTimeout("-1");

    bool enablePreview = true;

    notification->setShowPreview(enablePreview);
    notification->setShowInNotifyCenter(false);
    if (!calcReplaceId(notification))
        pushBubble(notification);

    // If replaces_id is 0, the return value is a UINT32 that represent the notification.
    // If replaces_id is not 0, the returned value is the same value as replaces_id.
    return replacesId == 0 ? notification->id() : replacesId;
}

void BubbleManager::pushBubble(EntityPtr notify)
{
    if (notify == nullptr)
        return;

    Bubble *bubble = createBubble(notify);
    if (!bubble)
        return;

    if (m_bubbleList.size() == BubbleEntities + BubbleOverLap) {
        m_oldEntities.push_front(m_bubbleList.last()->entity());
        m_bubbleList.last()->setVisible(false);
        m_bubbleList.last()->deleteLater();
        m_bubbleList.removeLast();
    }

    m_bubbleList.push_front(bubble);
    pushAnimation(bubble);
}

void BubbleManager::popBubble(Bubble *bubble)
{
    // bubble delete itself when aniamtion finished
    refreshBubble();
    popAnimation(bubble);
    m_bubbleList.removeOne(bubble);
}

void BubbleManager::refreshBubble()
{
    if (m_bubbleList.size() < BubbleEntities + BubbleOverLap + 1 && !m_oldEntities.isEmpty()) {
        auto notify = m_oldEntities.takeFirst();
        Bubble *bubble = createBubble(notify, BubbleEntities + BubbleOverLap - 1);
        if (bubble) {
            m_bubbleList.push_back(bubble);
            connect(bubble, &QObject::destroyed, [this, bubble] {
                m_bubbleList.removeAll(bubble);
            });
        }
    }
}

void BubbleManager::pushAnimation(Bubble *bubble)
{
    int index = m_bubbleList.indexOf(bubble);
    if (index == -1)
        return;

    while (index < m_bubbleList.size() - 1) {
        index ++;
        QRect startRect = getLastStableRect(index - 1);
        QRect endRect = getBubbleGeometry(index);
        QPointer<Bubble> item = m_bubbleList.at(index);
        if (item->geometry() != endRect) { //动画中
            startRect = item->geometry();
        }
        if (bubble != nullptr) {
            item->setBubbleIndex(index);
            item->startMove(startRect, endRect);
        }
    }
}

void BubbleManager::popAnimation(Bubble *bubble)
{
    int index = m_bubbleList.indexOf(bubble);
    if (index == -1)
        return;

    QRect startRect = getBubbleGeometry(index);
    QRect endRect = getBubbleGeometry(0);

    if (bubble)
        bubble->startMove(startRect, endRect, true); // delete itself

    while (index < m_bubbleList.size() - 1) {
        index ++;
        startRect = getBubbleGeometry(index);
        endRect = getBubbleGeometry(index - 1);
        QPointer<Bubble> item = m_bubbleList.at(index);
        if (index == BubbleEntities + BubbleOverLap) {
            item->show();
        }
        if (item->geometry() != endRect) { //动画中
            startRect = item->geometry();
        }
        if (bubble != nullptr) {
            item->setBubbleIndex(index);
            item->startMove(startRect, endRect);
        }
    }

    //确定层次关系
    for (int i = m_bubbleList.size() - 1; i >= 0 ; --i) {
        Bubble *b = m_bubbleList[i];
        if (b) {
            b->raise();
        }
    }
}

QRect BubbleManager::getBubbleGeometry(int index)
{
    Q_ASSERT(index >= 0 && index <= BubbleEntities + BubbleOverLap);

    QRect rect;
    if (index >= 0 && index <= BubbleEntities - 1) {
        int y = (m_dockPos == OSD::Top ? m_currentDockRect.bottom() : m_currentDisplayRect.y()); // 多屏时屏幕设置为上下错位，主屏的top可能不是0
        rect.setX(m_currentDisplayRect.x() + (m_currentDisplayRect.width() - OSD::BubbleWidth(OSD::BUBBLEWINDOW)) / 2);
        rect.setY(y + ScreenPadding + index * BubbleMargin + getBubbleHeightBefore(index));
        rect.setWidth(OSD::BubbleWidth(OSD::BUBBLEWINDOW));
        rect.setHeight(OSD::BubbleHeight(OSD::BUBBLEWINDOW));
    } else if (index >= BubbleEntities && index <= BubbleEntities + BubbleOverLap) {
        rect = getBubbleGeometry(index - 1);

        int x = rect.x() + rect.width() / 20;
        int y = rect.y() + rect.height() / 3;
        int width = rect.width() * 18 / 20;
        int height = rect.height() * 19 / 20;

        rect.setX(x);
        rect.setY(y);
        rect.setWidth(width);
        rect.setHeight(height);
    }

    return rect;
}

int BubbleManager::getBubbleHeightBefore(const int index)
{
    int totalHeight = 0;
    for (int i = 0; i < index; i++) {
        if (m_bubbleList[i]) {
            totalHeight += m_bubbleList[i]->height();
        }
    }

    return totalHeight;
}

QRect BubbleManager::getLastStableRect(int index)
{
    QRect rect = getBubbleGeometry(0);
    for (int i = index - 1; i > 0; --i) {
        if (i >= m_bubbleList.size() || m_bubbleList.at(i)->geometry() != getBubbleGeometry(i)) {
            continue;
        }
        rect = getBubbleGeometry(i);
    }

    return rect;
}

void BubbleManager::RemoveRecord(const QString &id)
{
    QFile file(CachePath + id + ".png");
    file.remove();
}

void BubbleManager::ClearRecords()
{
    QDir dir(CachePath);
    dir.removeRecursively();
}

void BubbleManager::Toggle()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();

    geometryChanged();
}

void BubbleManager::Show()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();
    geometryChanged();
}

void BubbleManager::Hide()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();
    geometryChanged();
}

void BubbleManager::bubbleExpired(Bubble *bubble)
{
    popBubble(bubble);
    Q_EMIT NotificationClosed(bubble->entity()->id(), BubbleManager::Expired);
}

void BubbleManager::bubbleDismissed(Bubble *bubble)
{
    popBubble(bubble);
    Q_EMIT NotificationClosed(bubble->entity()->id(), BubbleManager::Dismissed);
}

void BubbleManager::bubbleReplacedByOther(Bubble *bubble)
{
    Q_EMIT NotificationClosed(bubble->entity()->id(), BubbleManager::Unknown);
}

void BubbleManager::bubbleActionInvoked(Bubble *bubble, QString actionId)
{
    popBubble(bubble);
    uint id = bubble->entity()->id();
    uint replacesId = bubble->entity()->replacesId().toUInt();
    Q_EMIT ActionInvoked(replacesId == 0 ? id : replacesId, actionId);
    Q_EMIT NotificationClosed(bubble->entity()->id(), BubbleManager::Closed);
}

void BubbleManager::updateGeometry()
{
    foreach (auto item, m_bubbleList) {
        if (item.isNull())
            continue;

        if (item->parentWidget() != m_parentWidget) {
            bool visible = item->isVisible();
            item->setParent(m_parentWidget);
            item->setVisible(visible);
        }
        item->setGeometry(getBubbleGeometry(item->bubbleIndex()));
        item->updateGeometry();
    }
}

void BubbleManager::initConnections()
{
    connect(qApp, &QApplication::primaryScreenChanged, this, [ = ] {
        updateGeometry();
    });

    connect(qApp->primaryScreen(), &QScreen::geometryChanged, this, [ = ] {
        updateGeometry();
    });
}

void BubbleManager::geometryChanged()
{
    if(m_parentWidget)
        m_currentDisplayRect = m_parentWidget->rect();
    updateGeometry();
}

bool BubbleManager::calcReplaceId(EntityPtr notify)
{
    bool find = false;

    if (notify->replacesId() == NoReplaceId) {
        notify->setId(QString::number(++m_replaceCount));
        notify->setReplacesId(QString::number(m_replaceCount));
    } else {
        for (int i = 0; i < m_bubbleList.size(); ++i) {
            Bubble *bubble = m_bubbleList.at(i);
            if (bubble->entity()->replacesId() == notify->replacesId()
                    && bubble->entity()->appName() == notify->appName()) {
                if (i != 0) {
                    bubble->setEntity(m_bubbleList.at(i)->entity());
                }
                m_bubbleList.at(i)->setEntity(notify);
                find = true;
            }
        }

        for (int i = 0; i < m_oldEntities.size(); ++i) {
            if (m_oldEntities.at(i)->replacesId() == notify->replacesId()
                    && m_oldEntities.at(i)->appName() == notify->appName()) {
                m_oldEntities.removeAt(i);
            }
        }
    }

    return find;
}

bool BubbleManager::eventFilter(QObject *watched, QEvent *e)
{
    if (e->type() == QEvent::Show) {
        QWidget *w = qobject_cast<QWidget *>(watched);
        if (w) {
            m_parentWidget = w->window();
            if (m_parentWidget)
                m_parentWidget->installEventFilter(this);
            geometryChanged();
        }
    } else if (e->type() == QEvent::Resize) {
        geometryChanged();
    }
    return QObject::eventFilter(watched, e);
}

Bubble *BubbleManager::createBubble(EntityPtr notify, int index)
{
    Bubble *bubble = new Bubble(m_parentWidget, notify);
    connect(bubble, &Bubble::expired, this, &BubbleManager::bubbleExpired);
    connect(bubble, &Bubble::dismissed, this, &BubbleManager::bubbleDismissed);
    connect(bubble, &Bubble::actionInvoked, this, &BubbleManager::bubbleActionInvoked);

    if (index != 0) {
        QRect startRect = getBubbleGeometry(BubbleEntities + BubbleOverLap);
        QRect endRect = getBubbleGeometry(BubbleEntities + BubbleOverLap - 1);
        bubble->setBubbleIndex(BubbleEntities + BubbleOverLap - 1);
        bubble->startMove(startRect, endRect);
    } else {
        QRect endRect = getBubbleGeometry(0);
        QRect startRect = endRect;
        startRect.setHeight(1);

        bubble->setProperty("geometry",0);
        bubble->show();

        QPropertyAnimation *ani = new QPropertyAnimation(bubble, "geometry", this);
        ani->setStartValue(startRect);
        ani->setEndValue(endRect);

        int animationTime = int(endRect.height() * 1.0 / 72 * AnimationTime);
        ani->setDuration(animationTime);

        bubble->setBubbleIndex(0);
        ani->start(QPropertyAnimation::DeleteWhenStopped);
    }

    return bubble;
}
