// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QPointer>
#include <QWindow>

namespace dde {
namespace network {

class XdgActivationTokenV1;

class XdgActivation : public QObject
{
    Q_OBJECT

public:
    explicit XdgActivation(QObject *parent = nullptr);
    ~XdgActivation() override;

    bool isActive() const;
    void requestToken(QWindow *window = nullptr, const QString &appId = {});

Q_SIGNALS:
    void tokenReady(const QString &token);

private:
    QPointer<XdgActivationTokenV1> m_provider;
};

} // namespace network
} // namespace dde
