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
#ifndef SYSPROXYMODULE_H
#define SYSPROXYMODULE_H
#include "interface/pagemodule.h"

#include <dtkwidget_global.h>

DWIDGET_BEGIN_NAMESPACE
class DTextEdit;
DWIDGET_END_NAMESPACE

namespace DCC_NAMESPACE {
class SettingsGroup;
class LineEditWidget;
class ButtonTuple;
class SwitchWidget;
class ComboxWidget;
}

namespace dde {
namespace network {
class ControllItems;
enum class ProxyMethod;
}
}

class SysProxyModule : public DCC_NAMESPACE::PageModule
{
    Q_OBJECT
public:
    explicit SysProxyModule(QObject *parent = nullptr);

    virtual void active() override;
    virtual void deactive() override;

private Q_SLOTS:
    void initManualView(QWidget *w);
    void applySettings();
    void uiMethodChanged(dde::network::ProxyMethod uiMethod);
    void resetData(dde::network::ProxyMethod uiMethod);

private:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QList<DCC_NAMESPACE::ModuleObject *> m_modules;
    const QStringList m_ProxyMethodList;

    DCC_NAMESPACE::SwitchWidget *m_proxySwitch;
    DCC_NAMESPACE::ComboxWidget *m_proxyTypeBox;

    DCC_NAMESPACE::LineEditWidget *m_autoUrl;

    DCC_NAMESPACE::LineEditWidget *m_httpAddr;
    DCC_NAMESPACE::LineEditWidget *m_httpPort;
    DCC_NAMESPACE::LineEditWidget *m_httpsAddr;
    DCC_NAMESPACE::LineEditWidget *m_httpsPort;
    DCC_NAMESPACE::LineEditWidget *m_ftpAddr;
    DCC_NAMESPACE::LineEditWidget *m_ftpPort;
    DCC_NAMESPACE::LineEditWidget *m_socksAddr;
    DCC_NAMESPACE::LineEditWidget *m_socksPort;
    DTK_WIDGET_NAMESPACE::DTextEdit *m_ignoreList;
    DCC_NAMESPACE::ButtonTuple *m_buttonTuple;

    dde::network::ProxyMethod m_uiMethod; // ui修改的ProxyMethod
};

#endif // SYSPROXYMODULE_H
