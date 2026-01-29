// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QObject>
#include <QByteArray>

namespace network {
namespace service {

class HttpReply;

class HttpManager : public QObject
{
    Q_OBJECT

public:
    static void init();
    static void unInit();
    explicit HttpManager(QObject *parent = Q_NULLPTR);
    ~HttpManager() override = default;
    // 调用GET方法
    HttpReply *get(const QString &url);
};

/**
 * @brief The HttpReply class
 * Http 回复内容
 */
class HttpReply : public QObject
{
    Q_OBJECT

    friend class HttpManager;

public:
    ~HttpReply() override = default;
    QString errorMessage() const;
    int httpCode() const;
    QString portal() const;

protected:
    explicit HttpReply(QObject *parent = Q_NULLPTR);
    void setHeader(const QString &html);
    void setErrorMessage(const QString &errorMessage);

private:
    QString m_errorMessage;
    int m_httpCode;
    QString m_portalUrl;
};

}
}

#endif
