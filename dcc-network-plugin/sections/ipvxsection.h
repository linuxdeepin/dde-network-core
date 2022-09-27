// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IPVXSECTION_H
#define IPVXSECTION_H

#include "abstractsection.h"

#include <networkmanagerqt/ipv4setting.h>
#include <networkmanagerqt/ipv6setting.h>
#include <networkmanagerqt/generictypes.h>

namespace dcc {
  namespace widgets {
    class ComboxWidget;
    class LineEditWidget;
    class SpinBoxWidget;
    class SwitchWidget;
  }
}

using namespace NetworkManager;
using namespace dcc::widgets;

class QComboBox;

class IpvxSection : public AbstractSection
{
    Q_OBJECT

public:
    enum Ipvx {Ipv4, Ipv6};

    explicit IpvxSection(Ipv4Setting::Ptr ipv4Setting, QFrame *parent = nullptr);
    explicit IpvxSection(Ipv6Setting::Ptr ipv6Setting, QFrame *parent = nullptr);
    virtual ~IpvxSection() Q_DECL_OVERRIDE;

    bool allInputValid() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;

    bool saveIpv4Settings();
    bool saveIpv6Settings();

    void setIpv4ConfigMethodEnable(Ipv4Setting::ConfigMethod method, const bool enabled);
    void setIpv6ConfigMethodEnable(Ipv6Setting::ConfigMethod method, const bool enabled);
    void setNeverDefaultEnable(const bool neverDefault);

private:
    void initStrMaps();
    void initUI();
    void initForIpv4();
    void initForIpv6();
    void initConnection();

    void onIpv4MethodChanged(Ipv4Setting::ConfigMethod method);
    void onIpv6MethodChanged(Ipv6Setting::ConfigMethod method);

    bool ipv4InputIsValid();
    bool ipv6InputIsValid();
    bool isIpv4Address(const QString &ip);
    bool isIpv6Address(const QString &ip);
    bool isIpv4SubnetMask(const QString &ip);

    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QMap<QString, Ipv4Setting::ConfigMethod> Ipv4ConfigMethodStrMap;
    QMap<QString, Ipv6Setting::ConfigMethod> Ipv6ConfigMethodStrMap;

    QComboBox *m_methodChooser;
    ComboxWidget *m_methodLine;
    LineEditWidget *m_ipAddress;
    LineEditWidget *m_netmaskIpv4;
    SpinBoxWidget *m_prefixIpv6;
    LineEditWidget *m_gateway;
    SwitchWidget *m_neverDefault;

    QList<SettingsItem *> m_itemsList;

    Ipvx m_currentIpvx;
    Setting::Ptr m_ipvxSetting;
};

#endif /* IPVXSECTION_H */
