// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef BROWSERASSIST_H
#define BROWSERASSIST_H

#include <QObject>

class QProcess;
class QTimer;

namespace network {
namespace sessionservice {

class UrlOpenerHelper : public QObject
{
    Q_OBJECT

public:
    static void openUrl(const QString &url);

protected:
    explicit UrlOpenerHelper(QObject *parent = nullptr);
    ~UrlOpenerHelper();

private:
    void openUrlAddress(const QString &url);
    QString getDisplayEnvironment() const;
    void init();
    bool isPrepare() const;

private slots:
    void onCheckTimeout();
    void onServiceRegistered(const QString &service);

private:
    QStringList m_cacheUrls;
    QProcess *m_process;
    QTimer *m_timer;
    QString m_displayEnvironment;
    bool m_startManagerIsPrepare;
    bool m_checkComplete;
};

}
}

#endif // SERVICE_H
