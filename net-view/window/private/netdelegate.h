// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef NETDELEGATE_H
#define NETDELEGATE_H

#include "netmanager.h"

#include <DWidget>

#include <QLabel>
#include <QStyledItemDelegate>

class QSortFilterProxyModel;
class QVBoxLayout;
class QLabel;

namespace Dtk {
namespace Widget {
class DSpinner;
class DSwitchButton;
} // namespace Widget
} // namespace Dtk

namespace dde {
namespace network {

class NetIconButton;
class NetItem;
class NetDeviceItem;
class NetWiredDeviceItem;
class NetWirelessDeviceItem;
class NetTipsItem;
class NetAirplaneModeTipsItem;
class NetVPNTipsItem;
class NetWiredItem;
class NetWirelessHiddenItem;
class NetWirelessItem;
enum class NetConnectionStatus;

struct ItemSpacing
{
    int left;
    int top;
    int right;
    int bottom;
    int height;
    QStyleOptionViewItem::ViewItemPosition viewItemPosition;
};

class NetDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NetDelegate(QAbstractItemView *view);
    ~NetDelegate() override;

    void setFlags(NetType::NetManagerFlags flag);

    ItemSpacing getItemSpacing(const QModelIndex &index) const;
    // painting
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    // editing
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void destroyEditor(QWidget *editor, const QModelIndex &index) const override;

public Q_SLOTS:
    void onRequest(NetManager::CmdType cmd, const QString &id, const QVariantMap &param);

Q_SIGNALS:
    void requestExec(NetManager::CmdType cmd, const QString &id, const QVariantMap &param); // 向NetView发请求
    void request(NetManager::CmdType cmd, const QString &id, const QVariantMap &param);     // 向NetWidget发请求
    void requestUpdateLayout();
    void requestShow(const QString &id);

private:
    const QAbstractItemView *m_view;
    const QSortFilterProxyModel *m_model;
    NetType::NetManagerFlags m_flag;
};

class NetWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NetWidget(NetItem *item, QWidget *parent = nullptr);
    ~NetWidget() Q_DECL_OVERRIDE;

    void setCentralWidget(QWidget *widget);
    QWidget *centralWidget() const;
    void addPasswordWidget(QWidget *widget);
    void setNoMousePropagation(bool noMousePropagation);
    virtual void removePasswordWidget();

Q_SIGNALS:
    void requestExec(NetManager::CmdType cmd, const QString &id, const QVariantMap &param);
    void requestShow(const QString &id);
    void requestUpdateLayout();

public Q_SLOTS:
    virtual void exec(NetManager::CmdType cmd, const QString &id, const QVariantMap &param);
    void showPassword(const QString &id, const QVariantMap &param);
    void showError(const QString &id, const QVariantMap &param);
    void onRequestCheckInput(const QVariantMap &param);
    void updateInputValid(const QString &id, const QVariantMap &param);
    void onSubmit(const QVariantMap &param);
    void closeInput();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void sendRequest(NetManager::CmdType cmd, const QString &id, const QVariantMap &param = QVariantMap());

    inline NetItem *item() const { return m_item; }

private:
    NetItem *m_item;
    QVBoxLayout *m_mainLayout;
    bool m_noMousePropagation;
};

class NetDeviceWidget : public NetWidget
{
    Q_OBJECT
public:
    NetDeviceWidget(NetDeviceItem *item, QWidget *parent = nullptr);
    ~NetDeviceWidget() Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void onEnabledChanged(bool enabled);
    void onCheckedChanged(bool checked);
    void onScanClicked();

private:
    Dtk::Widget::DSwitchButton *m_switchBut;
};

class NetWirelessTypeControlWidget : public NetWidget
{
    Q_OBJECT
public:
    NetWirelessTypeControlWidget(NetItem *item, QWidget *parent = nullptr);
    ~NetWirelessTypeControlWidget() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onClicked();
    void updateExpandState(bool isExpanded);

private:
    NetIconButton *m_expandButton;
};

class NetItemWidget : public NetWidget
{
    Q_OBJECT

public:
    NetItemWidget(NetItem *item, QWidget *parent = nullptr);
    void removePasswordWidget() override;
    void setHover(bool isHover);
    void setFlag(NetType::NetManagerFlags flag);

protected:
    virtual bool isConnected() const = 0;
    void removeUrlLink();
    virtual int leftSpacing() const = 0;

    bool eventFilter(QObject *watched, QEvent *event) override;

protected slots:
    void onPortalUrlChanged(const QString &url);

private:
    QString getDisplayText(bool isHover);

private:
    QString m_portalUrl;
    QLabel *m_portalLabel;
    NetType::NetManagerFlags m_flag;
    bool m_isEnter;
};

class NetWirelessWidget : public NetItemWidget
{
    Q_OBJECT
public:
    NetWirelessWidget(NetWirelessItem *item, QWidget *parent = nullptr);
    ~NetWirelessWidget() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void updateIcon();
    void onStatusChanged(NetType::NetConnectionStatus status);
    void onDisconnectClicked();

protected:
    bool isConnected() const Q_DECL_OVERRIDE;
    int leftSpacing() const Q_DECL_OVERRIDE;

private:
    bool m_isWifi6;
    NetIconButton *m_iconBut;
    NetIconButton *m_connBut;
    Dtk::Widget::DSpinner *m_loading;
    dde::network::NetType::NetConnectionStatus m_status;
};

class NetWirelessHiddenWidget : public NetWidget
{
    Q_OBJECT
public:
    NetWirelessHiddenWidget(NetWirelessHiddenItem *item, QWidget *parent = nullptr);
    ~NetWirelessHiddenWidget() Q_DECL_OVERRIDE;
};

class NetTipsWidget : public NetWidget
{
    Q_OBJECT
public:
    NetTipsWidget(NetTipsItem *item, QWidget *parent = nullptr);
    ~NetTipsWidget() Q_DECL_OVERRIDE;

protected:
    bool event(QEvent *event) override;
    void updateHeight();

private:
    QLabel *m_label;
};

class NetAirplaneModeTipsWidget : public NetTipsWidget
{
    Q_OBJECT
public:
    NetAirplaneModeTipsWidget(NetAirplaneModeTipsItem *item, QWidget *parent = nullptr);
    ~NetAirplaneModeTipsWidget() Q_DECL_OVERRIDE;
};

class NetVPNTipsWidget : public NetTipsWidget
{
    Q_OBJECT
public:
    NetVPNTipsWidget(NetVPNTipsItem *item, QWidget *parent = nullptr);
    ~NetVPNTipsWidget() Q_DECL_OVERRIDE;
};

class NetWiredWidget : public NetItemWidget
{
    Q_OBJECT

public:
    NetWiredWidget(NetWiredItem *item, QWidget *parent = nullptr);
    ~NetWiredWidget() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void onStatusChanged(NetType::NetConnectionStatus status);
    void onDisconnectClicked();

protected:
    bool isConnected() const Q_DECL_OVERRIDE;
    int leftSpacing() const Q_DECL_OVERRIDE;

private:
    NetIconButton *m_iconBut;
    NetIconButton *m_connBut;
    Dtk::Widget::DSpinner *m_loading;
    dde::network::NetType::NetConnectionStatus m_status;
};

class NetDisabledWidget : public NetWidget
{
    Q_OBJECT

public:
    explicit NetDisabledWidget(NetItem *item, QWidget *parent = nullptr);
    ~NetDisabledWidget() Q_DECL_OVERRIDE;
};

} // namespace network
} // namespace dde

#endif // NETDELEGATE_H
