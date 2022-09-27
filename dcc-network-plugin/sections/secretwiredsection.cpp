// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "secretwiredsection.h"

#include "widgets/switchwidget.h"

using namespace dcc::widgets;

SecretWiredSection::SecretWiredSection(Security8021xSetting::Ptr sSetting, QFrame *parent)
    : Secret8021xSection(sSetting, parent)
    , m_secretEnable(new SwitchWidget(this))
    , m_enableWatcher(new Secret8021xEnableWatcher(this))
{
    setAccessibleName("SecretWiredSection");
    m_secretEnable->setTitle(tr("Security Required"));
    m_secretEnable->setChecked(!sSetting->toMap().isEmpty());

    m_enableWatcher->setSecretEnable(m_secretEnable->checked());

    connect(m_secretEnable, &SwitchWidget::checkedChanged, this, &SecretWiredSection::editClicked);
    connect(m_secretEnable, &SwitchWidget::checkedChanged, m_enableWatcher, &Secret8021xEnableWatcher::setSecretEnable);
    appendItem(m_secretEnable);
    QList<Security8021xSetting::EapMethod> eapMethodsWantedList;
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodTls);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodMd5);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodFast);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodTtls);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodPeap);

    init(m_enableWatcher, eapMethodsWantedList);
}

SecretWiredSection::~SecretWiredSection()
{
}

bool SecretWiredSection::allInputValid()
{
    return Secret8021xSection::allInputValid();
}

void SecretWiredSection::saveSettings()
{
    Secret8021xSection::saveSettings();
}
