// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOCALCLIENT_H
#define LOCALCLIENT_H

#include "dockpopupwindow.h"

#include <QLocalSocket>
#include <QProcess>

#include <DSingleton>

class DockPopupWindow;
class NetworkPanel;

class QTimer;
class QTranslator;

class LocalClient : public QObject
    , public Dtk::Core::DSingleton<LocalClient>
{
    Q_OBJECT

    friend class Dtk::Core::DSingleton<LocalClient>;

public:
    enum WaitClient {
        No,    // 无客户端等待
        Other, // 有其他客户端在等待密码
        Self,  // 本程序在等待密码
    };

protected:
    explicit LocalClient(QObject *parent = nullptr);

public:
    ~LocalClient();

public:
    bool ConnectToServer();
    void waitPassword(const QString &dev, const QString &ssid, bool wait);

    void showWidget();
    void initWidget();

    bool changePassword(QString key, QString password, bool input);
    bool eventFilter(QObject *watched, QEvent *event) override;

public:
    void showPosition(QLocalSocket *socket, const QByteArray &data);
    void receive(QLocalSocket *socket, const QByteArray &data);
    void onClick(QLocalSocket *socket, const QByteArray &data);
    void connectNetwork(QLocalSocket *socket, const QByteArray &data);
    void receivePassword(QLocalSocket *socket, const QByteArray &data);
    void updateTranslator(QString locale);
    inline WaitClient waitClientType() const { return m_wait; }
    inline QString ssidWaitingForPassword() const { return m_ssid; }
    void close(QLocalSocket *socket, const QByteArray &data);
    void releaseKeyboard();

private:
    void showPopupWindow(bool forceShowDialog = false);

private Q_SLOTS:
    void connectedHandler();
    void disConnectedHandler();
    void readyReadHandler();

private:
    QLocalSocket *m_clinet;
    QString m_dev;
    QString m_ssid; // 等待密码输入模式时才有值，由命令转入 @see waitPassword
    WaitClient m_wait; // 等待模式
    QTimer *m_timer;
    QTimer *m_exitTimer;
    QByteArray m_lastData;

    DockPopupWindow *m_popopWindow;
    NetworkPanel *m_panel;

    QTranslator *m_translator;
    QString m_locale;
    bool m_popopNeedShow;
    RunReason m_runReason;
};

#endif // LOCALCLIENT_H
