// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONNECTIVITYPROCESSER_H
#define CONNECTIVITYPROCESSER_H

#include "constants.h"

#include <QObject>

class QTimer;

namespace network {
namespace systemservice {

class SystemIPConflict;
class ConnectivityChecker;

class ConnectivityProcesser : public QObject
{
    Q_OBJECT

public:
    explicit ConnectivityProcesser(QObject *parent = nullptr);
    ~ConnectivityProcesser();
    void checkConnectivity();
    network::service::Connectivity connectivity() const;
    QString portalUrl() const;

signals:
    void connectivityChanged(const network::service::Connectivity &);
    void portalDetected(const QString &);

private:
    ConnectivityChecker *createConnectivityChecker(bool enableConnectivity);

private slots:
    void onEnableConnectivityChanged(bool enableConnectivity);

private:
    // 亲和线程可能是 NM 线程，删除一律经 deleteChecker()
    ConnectivityChecker *m_checker = nullptr;
};

}
}

#endif // CONNECTIVITYPROCESSER_H
