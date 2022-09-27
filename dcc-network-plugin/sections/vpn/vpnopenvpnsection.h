// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNOPENVPNSECTION_H
#define VPNOPENVPNSECTION_H

#include "../abstractsection.h"

#include <networkmanagerqt/vpnsetting.h>

namespace dcc {
  namespace widgets {
    class LineEditWidget;
    class ComboxWidget;
    class FileChooseWidget;
  }
}

using namespace NetworkManager;

class VpnOpenVPNSection : public AbstractSection
{
    Q_OBJECT

public:
    explicit VpnOpenVPNSection(VpnSetting::Ptr vpnSetting, QFrame *parent = nullptr);
    virtual ~VpnOpenVPNSection() Q_DECL_OVERRIDE;

    bool allInputValid() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;

    inline QString authType() const { return m_currentAuthType; }

Q_SIGNALS:
    void authTypeChanged(const QString &type) const;

private:
    void initStrMaps();
    void initUI();
    void initItems(const QString &type);
    void initTLSItems();
    void initPasswordItems();
    void initStaticKeyItems();
    void initConnection();
    void onAuthTypeChanged(const QString &type);
    void setItemsVisible(const QString &itemsType, const bool visible);
    bool tlsItemsInputValid();
    bool passwordItemsInputValid();
    bool staticKeyItemsInputValid();
    void saveTlsItems();
    void savePasswordItems();
    void saveStaticKeyItems();
    bool isIpv4Address(const QString &ip);
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QList<QPair<QString, QString>> AuthTypeStrMap;
    QList<QPair<QString, Setting::SecretFlagType>> PasswordFlagsStrMap;

    VpnSetting::Ptr m_vpnSetting;
    NMStringMap m_dataMap;
    NMStringMap m_secretMap;

    LineEditWidget *m_gateway;
    ComboxWidget *m_authTypeChooser;
    FileChooseWidget *m_caFile;

    QMap<QString, QList<SettingsItem *>> m_settingItemsMap;
    QString m_currentAuthType;
    QString m_currentKeyDirection;
    Setting::SecretFlagType m_currentPasswordType;
    Setting::SecretFlagType m_currentCertPasswordType;
};

#include "../declare_metatype_for_networkmanager.h"

#endif /* VPNOPENVPNSECTION_H */
