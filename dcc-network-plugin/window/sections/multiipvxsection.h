/*
 * Copyright (C) 2011 ~ 2021 Deepin Technology Co., Ltd.
 *
 * Author:     donghualin <donghualin@gmail.com>
 *
 * Maintainer: donghualin <donghualin@gmail.com>
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

#ifndef MULTIIPVXSECTION_H
#define MULTIIPVXSECTION_H

#include "abstractsection.h"

#include <networkmanagerqt/ipv4setting.h>
#include <networkmanagerqt/ipv6setting.h>
#include <networkmanagerqt/generictypes.h>
#include <networkmanagerqt/ipaddress.h>

namespace dcc {
namespace network {
class SpinBoxWidget;
}
}
DCC_BEGIN_NAMESPACE
class ComboxWidget;
class LineEditWidget;
class SwitchWidget;
class SettingsHead;
DCC_END_NAMESPACE

DWIDGET_BEGIN_NAMESPACE
class DIconButton;
class DLabel;
DWIDGET_END_NAMESPACE

class QComboBox;
class ActionButton;
class IPInputSection;
class QPaintEvent;

class MultiIpvxSection : public AbstractSection
{
    Q_OBJECT

public:
    explicit MultiIpvxSection(NetworkManager::Setting::Ptr ipvSetting, QFrame *parent = nullptr);
    virtual ~MultiIpvxSection() Q_DECL_OVERRIDE;

    bool allInputValid() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;

private:
    void refreshItems();
    void addIPV4Config();
    void addIPV6Config();
    void setDefaultValue();
    QList<IPInputSection *> createIpInputSections();
    void setIpInputSection(IPInputSection *ipSection, IPInputSection *itemBefore = nullptr);

protected slots:
    void onDeleteItem(IPInputSection *item);
    void onAddItem(IPInputSection *item);
    void onIPV4OptionChanged();
    void onIPV6OptionChanged();
    void onButtonShow(bool edit);

private:
    QList<IPInputSection *> m_ipSections;

    NetworkManager::Setting::Ptr m_ipvxSetting;
    QFrame *m_mainFrame;
    QComboBox *m_methodChooser;
    DCC_NAMESPACE::ComboxWidget *m_methodLine;
    QMap<QString, NetworkManager::Ipv4Setting::ConfigMethod> Ipv4ConfigMethodStrMap;
    QMap<QString, NetworkManager::Ipv6Setting::ConfigMethod> Ipv6ConfigMethodStrMap;
    DCC_NAMESPACE::SettingsHead *m_headerEditWidget;
    DCC_NAMESPACE::SettingsHead *m_headerWidget;
    bool m_isEditMode;
};

class IPInputSection : public DCC_NAMESPACE::SettingsItem
{
    Q_OBJECT

public:
    explicit IPInputSection(NetworkManager::IpAddress ipAddress, QFrame *parent = Q_NULLPTR);
    ~IPInputSection() override;

    void setTtile(const QString &title);
    void setAddItemVisible(bool visible);
    void setDeleteItemVisible(bool visible);
    virtual bool allInputValid(const QList<NetworkManager::IpAddress> &ipAddresses) = 0;
    virtual NetworkManager::IpAddress ipAddress() { return NetworkManager::IpAddress(); }

Q_SIGNALS:
    void editClicked();
    void requestDelete(IPInputSection *);
    void requestAdd(IPInputSection *);

protected:
    void initUi();
    void initConnection();
    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    DCC_NAMESPACE::LineEditWidget *m_lineIpAddress;
    DCC_NAMESPACE::LineEditWidget *m_gateway;
    QVBoxLayout *m_mainLayout;
    NetworkManager::IpAddress m_ipAddress;

private:
    QWidget *m_headerWidget;
    DTK_WIDGET_NAMESPACE::DLabel *m_titleLabel;

    DTK_WIDGET_NAMESPACE::DIconButton *m_newIpButton;
    DTK_WIDGET_NAMESPACE::DIconButton *m_deleteButton;
};

class IPV4InputSection : public IPInputSection
{
    Q_OBJECT

public:
    explicit IPV4InputSection(NetworkManager::IpAddress ipAddress, QFrame *parent = Q_NULLPTR);
    ~IPV4InputSection() override;
    bool allInputValid(const QList<NetworkManager::IpAddress> &ipAddresses) override;

private:
    void initUi();
    void initConnection();
    bool isIpv4Address(const QString &ip);
    bool isIpv4SubnetMask(const QString &ip);
    NetworkManager::IpAddress ipAddress() override;

private:
    QMap<QString, NetworkManager::Ipv4Setting::ConfigMethod> Ipv4ConfigMethodStrMap;
    DCC_NAMESPACE::LineEditWidget *m_netmaskIpv4;
};

class IPV6InputSection : public IPInputSection
{
    Q_OBJECT

public:
    explicit IPV6InputSection(NetworkManager::IpAddress ipAddress, QFrame *parent = Q_NULLPTR);
    ~IPV6InputSection() override;
    bool allInputValid(const QList<NetworkManager::IpAddress> &ipAddresses) override;
    NetworkManager::IpAddress ipAddress() override;

protected:
    void initUi();
    bool isIpv6Address(const QString &ip);

private:
    dcc::network::SpinBoxWidget *m_prefixIpv6;
};

#endif /* MULTIIPVXSECTION_H */
