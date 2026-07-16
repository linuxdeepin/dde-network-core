// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef NETCOMMONBUTTON_H
#define NETCOMMONBUTTON_H

#include <QAbstractButton>

namespace dde {
namespace network {

class NetCommonButton : public QAbstractButton
{
public:
    explicit NetCommonButton(QWidget *parent = nullptr, const QString &text = "");
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace network
} // namespace dde

#endif //NETCOMMONBUTTON_H
