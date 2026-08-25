// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docktestwidget.h"
#include "dockpopupwindow.h"
#include "networkplugin.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPixmap>
#include <DGuiApplicationHelper>
#include <QMenu>

QPointer<DockPopupWindow> DockTestWidget::PopupWindow = nullptr;

DockTestWidget::DockTestWidget(QWidget *parent)
    : QWidget(parent)
    , m_networkLabel(nullptr)
    , m_networkModule(new dde::network::NetworkPlugin(this))
    , m_popupShown(false)
{
    m_networkModule->init(this);
    initDock();
}

DockTestWidget::~DockTestWidget() = default;

void DockTestWidget::initDock()
{
    // 创建一个模拟NetworkPlugin itemWidget的网络状态显示
    QWidget *networkWidget = createNetworkStatusWidget();
    
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->addWidget(networkWidget);
    layout->addStretch();
    
    networkWidget->installEventFilter(this);
    this->installEventFilter(this);

    QPalette palette = this->palette();
    if (Dtk::Gui::DGuiApplicationHelper::instance()->themeType() == Dtk::Gui::DGuiApplicationHelper::ColorType::LightType) {
        palette.setBrush(QPalette::ColorRole::Window, Qt::white);
    } else {
        palette.setBrush(QPalette::ColorRole::Window, Qt::black);
    }
    setPalette(palette);

    connect(Dtk::Gui::DGuiApplicationHelper::instance(), &Dtk::Gui::DGuiApplicationHelper::themeTypeChanged, this, [ this ](Dtk::Gui::DGuiApplicationHelper::ColorType themeType) {
        QPalette palette = this->palette();
        if (themeType == Dtk::Gui::DGuiApplicationHelper::ColorType::LightType) {
            palette.setBrush(QPalette::ColorRole::Window, Qt::white);
        } else {
            palette.setBrush(QPalette::ColorRole::Window, Qt::black);
        }
        setPalette(palette);
    });

    if (PopupWindow.isNull()) {
        auto *popup = new DockPopupWindow(nullptr);
        popup->setObjectName("systemtraypopup");
        PopupWindow = popup;
        connect(qApp, &QApplication::aboutToQuit, PopupWindow, &DockPopupWindow::deleteLater);
    }
}

bool DockTestWidget::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        showHoverTips();
        return true;
    }
    if (event->type() == QEvent::Leave) {
        // auto hide if popup is not model window
        if (m_popupShown && !PopupWindow->model())
            hidePopup();
        return true;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        // auto hide if popup is not model window
        if (m_popupShown && !PopupWindow->model())
            hidePopup();
        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *e = static_cast<QMouseEvent *>(event);
        if (e->button() == Qt::LeftButton) {
            if (m_networkModule->itemPopupApplet(NETWORK_KEY)) {
                m_networkModule->itemPopupApplet(NETWORK_KEY)->setAccessibleName(m_networkModule->pluginName());
            }
            // auto hide if popup is not model window
            showPopupApplet(m_networkModule->itemPopupApplet(NETWORK_KEY));
        } else if (e->button() == Qt::RightButton) {
            const QString menuJson = m_networkModule->itemContextMenu(NETWORK_KEY);
            if (menuJson.isEmpty())
                return true;
            qInfo() << menuJson;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
            if (jsonDocument.isNull())
                return true;
            QJsonObject jsonMenu = jsonDocument.object();
            QMenu menu;
            QJsonArray jsonMenuItems = jsonMenu.value("items").toArray();
            for (auto item : jsonMenuItems) {
                QJsonObject itemObj = item.toObject();
                auto *action = new QAction(itemObj.value("itemText").toString());
                action->setCheckable(itemObj.value("isCheckable").toBool());
                action->setChecked(itemObj.value("checked").toBool());
                action->setData(itemObj.value("itemId").toString());
                action->setEnabled(itemObj.value("isActive").toBool());
                menu.addAction(action);
            }
            menu.exec(QCursor::pos());
        }
        return true;
    }

    return QWidget::eventFilter(object, event);
}

void DockTestWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    // 简单的悬停提示
    QMessageBox::information(this, "网络状态", "网络连接正常\nIP: 192.168.1.100");
}

void DockTestWidget::showPopupWindow(QWidget *content, bool model)
{
    m_popupShown = true;
    m_lastPopupWidget = content;
    if (model)
        emit requestWindowAutoHide(false);
    DockPopupWindow *popup = PopupWindow.data();
    QWidget *lastContent = popup->getContent();
    if (lastContent)
        lastContent->setVisible(false);

    popup->resize(content->sizeHint());
    popup->setContent(content);
    QPoint p = popupMarkPoint();
    if (!popup->isVisible())
        QMetaObject::invokeMethod(popup, "show", Qt::QueuedConnection, Q_ARG(QPoint, p), Q_ARG(bool, model));
    else
        popup->show(p, model);
    connect(popup, &DockPopupWindow::accept, this, &DockTestWidget::popupWindowAccept, Qt::UniqueConnection);
}

void DockTestWidget::showHoverTips()
{
    // another model popup window already exists
    if (PopupWindow->model())
        return;

    QWidget *const content = trayTipsWidget();
    if (!content)
        return;

    showPopupWindow(content);
}

const QPoint DockTestWidget::popupMarkPoint() const
{
    QPoint p(topLeftPoint());
    const QRect r = rect();
    p += QPoint(r.width() / 2, 0 - r.height() / 2);
    return p;
}

const QPoint DockTestWidget::topLeftPoint() const
{
    QPoint p;
    const QWidget *w = this;
    do {
        p += w->pos();
        w = qobject_cast<QWidget *>(w->parent());
    } while (w);
    return p;
}

void DockTestWidget::popupWindowAccept()
{
    if (!PopupWindow->isVisible())
        return;

    disconnect(PopupWindow.data(), &DockPopupWindow::accept, this, &DockTestWidget::popupWindowAccept);
    hidePopup();
}

void DockTestWidget::showPopupApplet(QWidget *applet)
{
    if (!applet)
        return;

    // another model popup window already exists
    if (PopupWindow->model()) {
        applet->setVisible(false);
        return;
    }

    showPopupWindow(applet, true);
}

void DockTestWidget::hidePopup()
{
    m_popupShown = false;
    PopupWindow->hide();
    emit PopupWindow->accept();
    emit requestWindowAutoHide(true);
}

QWidget *DockTestWidget::trayTipsWidget()
{
    QLabel *tip = new QLabel("网络状态: 已连接\nIP: 192.168.1.100\n速度: 100Mbps");
    tip->setFixedSize(200, 80);
    return tip;
}

void DockTestWidget::itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey)
{

}

void DockTestWidget::itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey)
{

}

void DockTestWidget::itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey)
{

}

void DockTestWidget::requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide)
{

}

void DockTestWidget::requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey)
{

}

void DockTestWidget::requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible)
{
    if (visible) {
        showPopupApplet(itemInter->itemPopupApplet(itemKey));
    } else {
        hidePopup();
    }
}

void DockTestWidget::saveValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &value)
{

}

const QVariant DockTestWidget::getValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &fallback)
{
    if (key == "enabled")
        return true;

    return QVariant();
}

void DockTestWidget::removeValue(PluginsItemInterface * const itemInter, const QStringList &keyList)
{

}

QWidget *DockTestWidget::createNetworkStatusWidget()
{
    // 创建一个模拟NetworkPlugin itemWidget的网络状态显示
    QWidget *networkWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(networkWidget);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(5);
    
    layout->addWidget(m_networkModule->itemWidget(NETWORK_KEY));

    return networkWidget;
}

QWidget *DockTestWidget::createNetworkPopupApplet()
{
    // 创建一个模拟NetworkPlugin弹出面板的widget
    QWidget *applet = new QWidget();
    applet->setFixedSize(300, 200);
    
    QVBoxLayout *layout = new QVBoxLayout(applet);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    
    // 标题
    QLabel *titleLabel = new QLabel("网络状态");
    titleLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; }");
    
    // 网络信息
    QLabel *infoLabel = new QLabel("连接状态: 已连接\n网络类型: 有线网络\nIP地址: 192.168.1.100\n速度: 100Mbps");
    infoLabel->setStyleSheet("QLabel { font-size: 12px; }");
    
    // 控制按钮
    QPushButton *settingsButton = new QPushButton("网络设置");
    settingsButton->setStyleSheet("QPushButton { background-color: #2ca7f8; color: white; border: none; padding: 5px; border-radius: 3px; }");
    
    layout->addWidget(titleLabel);
    layout->addWidget(infoLabel);
    layout->addStretch();
    layout->addWidget(settingsButton);
    
    applet->setStyleSheet("QWidget { background-color: white; border: 1px solid #ccc; border-radius: 5px; }");
    
    return applet;
}
