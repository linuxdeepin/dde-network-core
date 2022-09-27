// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkplugin.h"
#include "networkpluginhelper.h"
#include "trayicon.h"
#include "networkdialog.h"

#include <DDBusSender>

#include <QTime>

#include <networkcontroller.h>

#define STATE_KEY "enabled"

NETWORKPLUGIN_USE_NAMESPACE

NetworkPlugin::NetworkPlugin(QObject *parent)
    : QObject(parent)
    , m_networkHelper(Q_NULLPTR)
    , m_networkDialog(Q_NULLPTR)
    , m_clickTime(-10000)
    , m_trayIcon(nullptr)
{
    NetworkController::setIPConflictCheck(true);
    QTranslator *translator = new QTranslator(this);
    translator->load(QString("/usr/share/dock-network-plugin/translations/dock-network-plugin_%1.qm").arg(QLocale::system().name()));
    QCoreApplication::installTranslator(translator);
}

NetworkPlugin::~NetworkPlugin()
{
}

const QString NetworkPlugin::pluginName() const
{
    return "network";
}

const QString NetworkPlugin::pluginDisplayName() const
{
    return tr("Network");
}

void NetworkPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (m_networkHelper)
        return;

    m_networkDialog = new NetworkDialog(this);
    m_networkDialog->setServerName("dde-network-dialog" + QString::number(getuid()) + "dock");
    m_networkHelper.reset(new NetworkPluginHelper(m_networkDialog));
    QDBusConnection::sessionBus().connect("com.deepin.dde.lockFront", "/com/deepin/dde/lockFront", "com.deepin.dde.lockFront", "Visible", this, SLOT(lockFrontVisible(bool)));

    QDBusConnection::sessionBus().connect("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", "org.freedesktop.DBus.Properties",  "PropertiesChanged", this, SLOT(onDockPropertiesChanged(QString, QVariantMap, QStringList)));

    if (!pluginIsDisable())
        loadPlugin();

    m_networkDialog->runServer(true);
}

void NetworkPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(checked)

    if (itemKey == NETWORK_KEY)
        m_networkHelper->invokeMenuItem(menuId);
}

void NetworkPlugin::refreshIcon(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY)
        emit m_networkHelper->viewUpdate();
}

void NetworkPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, STATE_KEY, pluginIsDisable());

    refreshPluginItemsVisible();
}

bool NetworkPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, STATE_KEY, true).toBool();
}

const QString NetworkPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    if (m_networkHelper->needShowControlCenter()) {
        return QString("dbus-send --print-reply "
                       "--dest=com.deepin.dde.ControlCenter "
                       "/com/deepin/dde/ControlCenter "
                       "com.deepin.dde.ControlCenter.ShowModule "
                       "\"string:network\"");
    }

    return QString();
}

const QString NetworkPlugin::itemContextMenu(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY)
        return m_networkHelper->contextMenu(true);

    return QString();
}

QWidget *NetworkPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY) {
        if (m_trayIcon.isNull()) {
            m_trayIcon.reset(new TrayIcon(m_networkHelper.data()));
            connect(this, &NetworkPlugin::signalShowNetworkDialog, m_trayIcon.data(), &TrayIcon::showNetworkDialog);
            connect(m_trayIcon.data(), &TrayIcon::signalShowNetworkDialog, this, &NetworkPlugin::showNetworkDialog);
            connect(m_networkDialog, &NetworkDialog::requestPosition, m_trayIcon.data(), &TrayIcon::showNetworkDialog);
        }
        QTimer::singleShot(100, this, &NetworkPlugin::updatePoint);
        return m_trayIcon.data();
    }

    return Q_NULLPTR;
}

QWidget *NetworkPlugin::itemTipsWidget(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY && !m_networkDialog->isVisible())
        return m_networkHelper->itemTips();

    return Q_NULLPTR;
}

QWidget *NetworkPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    int msec = QTime::currentTime().msecsSinceStartOfDay();
    if (!m_networkDialog->isVisible() && !m_networkHelper->needShowControlCenter() && abs(msec - m_clickTime) > 200) {
        m_clickTime = msec;
        emit signalShowNetworkDialog();
        m_networkDialog->show();
    }
    return Q_NULLPTR;
}

int NetworkPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, 3).toInt();
}

void NetworkPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

void NetworkPlugin::pluginSettingsChanged()
{
    refreshPluginItemsVisible();
}

void NetworkPlugin::loadPlugin()
{
    m_proxyInter->itemAdded(this, NETWORK_KEY);
}

void NetworkPlugin::refreshPluginItemsVisible()
{
    if (pluginIsDisable())
        m_proxyInter->itemRemoved(this, NETWORK_KEY);
    else
        m_proxyInter->itemAdded(this, NETWORK_KEY);
}

void NetworkPlugin::positionChanged(const Dock::Position position)
{
    Q_UNUSED(position);
    updatePoint();
}

void NetworkPlugin::lockFrontVisible(bool visible)
{
    m_networkDialog->runServer(!visible);
    if (!visible) {
        updatePoint();
    }
}

void NetworkPlugin::updatePoint()
{
    emit signalShowNetworkDialog();
}

void NetworkPlugin::showNetworkDialog(QWidget *widget) const
{
    const QWidget *w = qobject_cast<QWidget *>(widget->parentWidget());
    const QWidget *parentWidget = w;
    Dtk::Widget::DArrowRectangle::ArrowDirection position = Dtk::Widget::DArrowRectangle::ArrowDirection::ArrowBottom;
    QPoint point;
    while (w) {
        parentWidget = w;
        w = qobject_cast<QWidget *>(w->parentWidget());
    }

    if (parentWidget) {
        Dock::Position pos = qApp->property(PROP_POSITION).value<Dock::Position>();
        QPoint p = widget->rect().center();
        QRect rect = parentWidget->rect();

        // 任务栏隐藏时不改变网络面板的位置,避免网络面板同步隐藏后不方便操作网络
        if (rect.height() <= 0 || rect.width() <= 0) {
            return;
        }

        switch (pos) {
        case Dock::Position::Top:
            p.ry() += rect.height() / 2;
            position = Dtk::Widget::DArrowRectangle::ArrowDirection::ArrowTop;
            break;
        case Dock::Position::Bottom:
            p.ry() -= rect.height() / 2;
            position = Dtk::Widget::DArrowRectangle::ArrowDirection::ArrowBottom;
            break;
        case Dock::Position::Left:
            p.rx() += rect.width() / 2;
            position = Dtk::Widget::DArrowRectangle::ArrowDirection::ArrowLeft;
            break;
        case Dock::Position::Right:
            p.rx() -= rect.width() / 2;
            position = Dtk::Widget::DArrowRectangle::ArrowDirection::ArrowRight;
            break;
        }
        p = widget->mapToGlobal(p);
        point = p;
    }

    m_networkDialog->setPosition(point.x(), point.y(), position);
}

void NetworkPlugin::onDockPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    qInfo() << Q_FUNC_INFO << ", network dialog is visible: " << m_networkDialog->isVisible();
    if (!m_networkDialog || !m_networkDialog->isVisible()) {
        return;
    }

    if (changedProperties.contains("FrontendWindowRect")) {
        // 延迟100ms处理,避免获取到的坐标有误(dock移动位置有动态效果，不加延时的话可能会获取动画中的位置)
        QTimer::singleShot(100, this, [this] {
            showNetworkDialog(m_trayIcon.data());
            m_networkDialog->updateDialogPosition();
        });
    }
}
