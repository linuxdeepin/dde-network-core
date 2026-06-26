// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INTERNETCHECKER_H
#define INTERNETCHECKER_H

#include <QObject>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/Device>

class QTimer;

namespace network {
namespace systemservice {

class InternetChecker : public QObject
{
    Q_OBJECT

public:
    explicit InternetChecker(QObject *parent = nullptr);
    ~InternetChecker() override;
    void switchInternetAccess(bool checkPrimaryConnection = false);

signals:
    void switchSuccess();
    void switchFailed();

private slots:
    void onPrimaryConnectionChanged(const QString &uni);
    void onPrimaryConnectionTimeout();

private:
    void resetAllNeverDefault() const;
    bool setConnectionNeverDefault(const NetworkManager::Connection::Ptr &conn, const NetworkManager::Device::Ptr &device, bool neverDefault) const;
    void setPrimaryDeviceNeverDefault(bool neverDefault) const;
    bool checkInternetAccessible() const;
    bool checkInternetAccessible(int timeoutSec, bool &timedOut) const;
    bool checkInternetAccessibleWithRetry(int maxRetry) const;
    void changeDeviceNeverDefault(const NetworkManager::Device::Ptr &device, bool neverDefault) const;

    int m_tryIndex;
    NetworkManager::Device::List m_tryDevices;
    QTimer *m_switchTimer;
    bool m_isSwitching;
};

}
}

#endif // INTERNETCHECKER_H
