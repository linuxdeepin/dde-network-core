/*
* Copyright (C) 2021 ~ 2023 Deepin Technology Co., Ltd.
*
* Author:     caixiangrong <caixiangrong@uniontech.com>
*
* Maintainer: caixiangrong <caixiangrong@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef APPPROXYMODULE_H
#define APPPROXYMODULE_H
#include "interface/moduleobject.h"

#include <dtkwidget_global.h>

DWIDGET_BEGIN_NAMESPACE
class DTextEdit;
DWIDGET_END_NAMESPACE
DCC_BEGIN_NAMESPACE
class SettingsGroup;
class LineEditWidget;
class ButtonTuple;
class SwitchWidget;
class ComboxWidget;
DCC_END_NAMESPACE

namespace dde {
namespace network {
class ControllItems;
enum class ProxyMethod;
}
}

class AppProxyModule : public DCC_NAMESPACE::ModuleObject
{
    Q_OBJECT
public:
    explicit AppProxyModule(QObject *parent = nullptr);

    virtual QWidget *extraButton() override;
//    virtual void active() override;
    virtual void deactive() override;
Q_SIGNALS:
    void resetData();

private Q_SLOTS:
    void onCheckValue();
//    void initManualView(QWidget *w);
//    void applySettings();
//    void uiMethodChanged(dde::network::ProxyMethod uiMethod);
//    void resetData(dde::network::ProxyMethod uiMethod);

private:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;
    bool isIPV4(const QString &ipv4);

private:
    DCC_NAMESPACE::ComboxWidget *m_proxyType;
    DCC_NAMESPACE::LineEditWidget *m_addr;
    DCC_NAMESPACE::LineEditWidget *m_port;
    DCC_NAMESPACE::LineEditWidget *m_username;
    DCC_NAMESPACE::LineEditWidget *m_password;
    //    QComboBox *m_comboBox;
    DCC_NAMESPACE::ButtonTuple *m_btns;
};

#endif // APPPROXYMODULE_H
