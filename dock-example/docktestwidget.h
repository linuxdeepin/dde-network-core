// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCKTESTWIDGET_H
#define DOCKTESTWIDGET_H

#include "dockpopupwindow.h"
#include "pluginproxyinterface.h"

#include <QWidget>
#include <QLabel>
#include <QPointer>
#include <QHBoxLayout>

namespace dde {
namespace network {
class NetworkPlugin;
}
}

class DockTestWidget : public QWidget, public PluginProxyInterface
{
    Q_OBJECT

public:
    explicit DockTestWidget(QWidget *parent = nullptr);
    ~DockTestWidget() override;

private:
    void initDock();
    bool eventFilter(QObject *object, QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;

protected:
    void showPopupWindow(QWidget *content, bool model = false);
    void showHoverTips();
    const QPoint popupMarkPoint() const;
    const QPoint topLeftPoint() const;
    void popupWindowAccept();
    void showPopupApplet(QWidget *applet);
    void hidePopup();
    QWidget *trayTipsWidget();

protected:
    void itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey);
    void itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey);
    void itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey);
    void requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide);
    void requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey);

    void requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible);
    void saveValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &value);
    const QVariant getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant& fallback = QVariant());
    void removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList);

Q_SIGNALS:
    void requestWindowAutoHide(const bool autoHide);

private:
    QWidget *createNetworkStatusWidget();
    QWidget *createNetworkPopupApplet();

private:
    QLabel *m_networkLabel;
    dde::network::NetworkPlugin *m_networkModule;

    bool m_popupShown;
    QPointer<QWidget> m_lastPopupWidget;

    static QPointer<DockPopupWindow> PopupWindow;
};

#endif // DOCKTESTWIDGET_H
