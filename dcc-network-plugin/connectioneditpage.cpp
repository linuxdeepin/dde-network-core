// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connectioneditpage.h"
#include "settings/wiredsettings.h"
#include "settings/wirelesssettings.h"
#include "settings/dslpppoesettings.h"
#include "window/gsettingwatcher.h"
#include "sections/abstractsection.h"

#include <DDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtDBus/qdbusmetatype.h>

#include <interface/moduleinterface.h>
#include <interface/namespace.h>
#include <widgets/translucentframe.h>
#include <widgets/buttontuple.h>

#include <networkmanagerqt/settings.h>
#include <networkmanagerqt/security8021xsetting.h>
#include <networkmanagerqt/wirelesssecuritysetting.h>
#include <networkmanagerqt/pppoesetting.h>
#include <networkmanagerqt/vpnsetting.h>

using namespace dcc::widgets;
using namespace NetworkManager;
DWIDGET_USE_NAMESPACE

static QString DevicePath = "";

dccV20::FrameProxyInterface *ConnectionEditPage::m_frame = nullptr;

ConnectionEditPage::ConnectionEditPage(ConnectionType connType, const QString &devPath,
        const QString &connUuid, QWidget *parent, bool isHotSpot)
    : ContentWidget(parent)
    , m_settingsLayout(new QVBoxLayout(this))
    , m_connection(nullptr)
    , m_connectionSettings(nullptr)
    , m_settingsWidget(nullptr)
    , m_isNewConnection(false)
    , m_connectionUuid(connUuid)
    , m_mainLayout(new QVBoxLayout(this))
    , m_disconnectBtn(nullptr)
    , m_removeBtn(nullptr)
    , m_buttonTuple(new ButtonTuple(ButtonTuple::Save, this))
    , m_buttonTuple_conn(new ButtonTuple(ButtonTuple::Delete, this))
    , m_subPage(nullptr)
    , m_connType(static_cast<ConnectionSettings::ConnectionType>(connType))
    , m_isHotSpot(isHotSpot)
{
    qDBusRegisterMetaType<NMVariantMapMap>();
    DevicePath = devPath;

    initUI();

    if (m_connectionUuid.isEmpty()) {
        qDebug() << "connection uuid is empty, creating new ConnectionSettings...";
        createConnSettings();
        m_isNewConnection = true;
    } else {
        m_connection = findConnectionByUuid(m_connectionUuid);
        if (!m_connection) {
            qDebug() << "can't find connection by uuid";
            return;
        }
        m_connectionSettings = m_connection->settings();
        m_isNewConnection = false;
        initConnectionSecrets();
    }

    initHeaderButtons();
    initConnection();

    m_removeBtn->installEventFilter(this);
}

ConnectionEditPage::~ConnectionEditPage()
{
    GSettingWatcher::instance()->erase("removeConnection", m_removeBtn);
}

void ConnectionEditPage::initUI()
{
    setAccessibleName("ConnectionEditPage");
    m_settingsLayout->setSpacing(10);

    m_buttonTuple_conn->removeSpacing();
    m_disconnectBtn = m_buttonTuple_conn->leftButton();
    m_removeBtn = m_buttonTuple_conn->rightButton();
    GSettingWatcher::instance()->bind("removeConnection", m_removeBtn);

    m_disconnectBtn->setText(tr("Disconnect", "button"));
    m_removeBtn->setText(tr("Delete", "button"));

    QPushButton *cancelBtn = m_buttonTuple->leftButton();
    QPushButton *acceptBtn = m_buttonTuple->rightButton();
    m_buttonTuple->setAutoFillBackground(true);
    cancelBtn->setText(tr("Cancel", "button"));
    acceptBtn->setText(tr("Save", "button"));
    m_buttonTuple->leftButton()->setEnabled(false);
    m_buttonTuple->rightButton()->setEnabled(false);

    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    m_mainLayout->addWidget(m_buttonTuple_conn);
    m_mainLayout->addLayout(m_settingsLayout);
    m_mainLayout->addStretch();
    m_mainLayout->setSpacing(10);

    QWidget *mainWidget = new TranslucentFrame(this);
    mainWidget->setLayout(m_mainLayout);

    setContent(mainWidget);
    QVBoxLayout * btnTupleLayout = new QVBoxLayout(this);
    btnTupleLayout->setSpacing(0);
    btnTupleLayout->setContentsMargins(10, 10, 10, 10);
    btnTupleLayout->addWidget(m_buttonTuple);
    qobject_cast<QVBoxLayout *>(layout())->addLayout(btnTupleLayout);

    setMinimumWidth(380);
}

const ActiveConnection::State ConnectionEditPage::ConnectedState()
{
    NetworkManager::Device::Ptr device(new NetworkManager::Device(DevicePath));
    if (device->type() == Device::Type::Wifi || device->type() == Device::Type::Ethernet) {
        // 如果是有线网络或者无线网络
        NetworkManager::ActiveConnection::Ptr activeConn = device->activeConnection();
        if (!activeConn.isNull() && (activeConn->uuid() == m_connection->uuid())) {
            return activeConn->state();
        }
    }

    for (auto conn : activeConnections()) {
        if (conn->uuid() == m_connection->uuid()) {
            return conn->state();
        }
    }

    return NetworkManager::ActiveConnection::Unknown;
}

void ConnectionEditPage::initHeaderButtons()
{
    if (m_isNewConnection) {
        m_disconnectBtn->setVisible(false);
        m_removeBtn->setVisible(false);
        m_buttonTuple_conn->setVisible(false);
        return;
    }

    if (m_connection) {
        m_disconnectBtn->setProperty("activeConnectionPath", m_connection->path());
        m_disconnectBtn->setProperty("connectionUuid", m_connection->uuid());
    }

    // 根据连接状态显示按钮文案和状态
    if (ConnectedState() == NetworkManager::ActiveConnection::Activating) {
        m_disconnectBtn->setEnabled(false);
        m_disconnectBtn->setText(tr("Connecting"));
    } else if (ConnectedState() == NetworkManager::ActiveConnection::Activated) {
        m_disconnectBtn->setEnabled(true);
        m_disconnectBtn->setText(tr("Disconnect"));
    } else {
        m_disconnectBtn->setEnabled(false);
        m_disconnectBtn->setText(tr("Disconnect"));
    }
}

void ConnectionEditPage::initSettingsWidget()
{
    if (!m_connectionSettings) {
        return;
    }

    switch (m_connType) {
    case ConnectionSettings::ConnectionType::Wired: {
        m_settingsWidget = new WiredSettings(m_connectionSettings, DevicePath, this);
        break;
    }
    case ConnectionSettings::ConnectionType::Wireless: {
        m_settingsWidget = new WirelessSettings(m_connectionSettings, m_tempParameter, this);
        break;
    }

    case ConnectionSettings::ConnectionType::Pppoe: {
        m_settingsWidget = new DslPppoeSettings(m_connectionSettings, DevicePath, this);
        break;
    }
    default:
        break;
    }

    connect(m_settingsWidget, &AbstractSettings::anyEditClicked, this, [this]{
        m_buttonTuple->leftButton()->setEnabled(true);
        m_buttonTuple->rightButton()->setEnabled(true);
    });
    connect(m_settingsWidget, &AbstractSettings::requestNextPage, this, &ConnectionEditPage::onRequestNextPage);
    connect(m_settingsWidget, &AbstractSettings::requestFrameAutoHide, this, &ConnectionEditPage::requestFrameAutoHide);

    m_settingsLayout->addWidget(m_settingsWidget);
}

const QString ConnectionEditPage::devicePath()
{
    return DevicePath;
}

void ConnectionEditPage::setDevicePath(const QString &path)
{
    DevicePath = path;
}

void ConnectionEditPage::onDeviceRemoved()
{
    if (m_subPage) {
        Q_EMIT m_subPage->back();
    }

    Q_EMIT back();
}

void ConnectionEditPage::initConnection()
{
    connect(m_buttonTuple->rightButton(), &QPushButton::clicked, this, &ConnectionEditPage::saveConnSettings);
    connect(m_buttonTuple->leftButton(), &QPushButton::clicked, this, &ConnectionEditPage::back);
    connect(this, &ConnectionEditPage::saveSettingsDone, this, &ConnectionEditPage::prepareConnection);
    connect(this, &ConnectionEditPage::prepareConnectionDone, this, &ConnectionEditPage::updateConnection);

    if (m_frame)
        connect(this, &ConnectionEditPage::back, [ = ] {
            parentWidget()->setFocus();
            m_frame->popWidget(nullptr);
        });

    connect(m_removeBtn, &QPushButton::clicked, this, [ = ] {
        DDialog dialog(this);
        dialog.setAccessibleName("Form_delete_configuration?");
        dialog.setTitle(tr("Are you sure you want to delete this configuration?"));
        dialog.addButton(tr("Cancel", "button"));
        // 按设计要求，删除操作的危险行为，不管活动色是什么，删除按钮文字改为用红色的
        dialog.addButton(tr("Delete", "button"), false, DDialog::ButtonWarning);
        int ret = dialog.exec();
        if (ret == QDialog::Accepted) {
            m_connection->remove();
            Q_EMIT back();
        }
    });

    connect(m_disconnectBtn, &QPushButton::clicked, this, [ = ]() {
        NetworkManager::Connection::Ptr con = NetworkManager::findConnection(m_connection->path());
        if (!con) {
            qWarning() << "Not possible to deactivate this connection";
        } else {
            for (const NetworkManager::ActiveConnection::Ptr &active : NetworkManager::activeConnections()) {
                if (active->uuid() == con->uuid()) {
                    qDebug() << "deactivate connection: " << active->path();
                    NetworkManager::deactivateConnection(active->path());
                }
            }
        }
        Q_EMIT back();
    });
}

NMVariantMapMap ConnectionEditPage::secretsMapMapBySettingType(Setting::SettingType settingType)
{
    QDBusPendingReply<NMVariantMapMap> reply;
    reply = m_connection->secrets(m_connectionSettings->setting(settingType)->name());

    reply.waitForFinished();
    if (reply.isError() || !reply.isValid()) {
        qDebug() << "get secrets error for connection:" << reply.error();
    }

    return reply.value();
}

template <typename T>
void ConnectionEditPage::setSecretsFromMapMap(Setting::SettingType settingType, NMVariantMapMap secretsMapMap)
{
    QSharedPointer<T> setting = m_connectionSettings->setting(settingType).staticCast<T>();
    setting->secretsFromMap(secretsMapMap.value(setting->name()));
}

void ConnectionEditPage::onRequestNextPage(dcc::ContentWidget *const page)
{
    m_subPage = page;
    Q_EMIT requestNextPage(page);
}

void ConnectionEditPage::initConnectionSecrets()
{
    Setting::SettingType sType;
    NMVariantMapMap sSecretsMapMap;

    switch (m_connType) {
    case ConnectionSettings::ConnectionType::Wired: {
        sType = Setting::SettingType::Security8021x;
        if (!m_connectionSettings->setting(sType).staticCast<Security8021xSetting>()->eapMethods().isEmpty()) {
            sSecretsMapMap = secretsMapMapBySettingType(sType);
            setSecretsFromMapMap<Security8021xSetting>(sType, sSecretsMapMap);
        }
        break;
    }
    case ConnectionSettings::ConnectionType::Wireless: {
        sType = Setting::SettingType::WirelessSecurity;

        WirelessSecuritySetting::KeyMgmt keyMgmt = m_connectionSettings->setting(sType).staticCast<WirelessSecuritySetting>()->keyMgmt();
        if (keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaNone || keyMgmt == WirelessSecuritySetting::KeyMgmt::Unknown)
            break;

        if (keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaEap) {
            sType = Setting::SettingType::Security8021x;
        }
        sSecretsMapMap = secretsMapMapBySettingType(sType);
        setSecretsFromMapMap<WirelessSecuritySetting>(sType, sSecretsMapMap);
        break;
    }

    case ConnectionSettings::ConnectionType::Vpn: {
        sType = Setting::SettingType::Vpn;
        sSecretsMapMap = secretsMapMapBySettingType(sType);
        setSecretsFromMapMap<VpnSetting>(sType, sSecretsMapMap);
        break;
    }

    case ConnectionSettings::ConnectionType::Pppoe: {
        sType = Setting::SettingType::Pppoe;
        sSecretsMapMap = secretsMapMapBySettingType(sType);
        setSecretsFromMapMap<PppoeSetting>(sType, sSecretsMapMap);
        break;
    }

    default:
        break;
    }
}

void ConnectionEditPage::saveConnSettings()
{
    if (!m_settingsWidget->allInputValid()) {
        return;
    }

    m_settingsWidget->saveSettings();
    Q_EMIT saveSettingsDone();
}

void ConnectionEditPage::prepareConnection()
{
    if (!m_connection) {
        qDebug() << "preparing connection...";
        qDBusRegisterMetaType<QByteArrayList>();
        QDBusPendingReply<QDBusObjectPath> reply = addConnection(m_connectionSettings->toMap());
        reply.waitForFinished();
        const QString &connPath = reply.value().path();
        m_connection = findConnection(connPath);
        if (!m_connection) {
            qDebug() << "create connection failed..." << reply.error();
            Q_EMIT back();
            return;
        }
    }

    Q_EMIT prepareConnectionDone();
}

void ConnectionEditPage::updateConnection()
{
    if (!m_isNewConnection) {
        // update function saves the settings on the hard disk
        QDBusPendingReply<> reply;
        reply = m_connection->update(m_connectionSettings->toMap());
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "error occurred while updating the connection" << reply.error();
            Q_EMIT back();
            return;
        }
    }

    if (m_settingsWidget->isAutoConnect()) {
         if (static_cast<int>(m_connType) == static_cast<int>(ConnectionEditPage::WiredConnection)) {
             Q_EMIT activateWiredConnection(m_connection->path(), m_connectionUuid);
         } else {
             if (static_cast<int>(m_connType) == static_cast<int>(ConnectionEditPage::VpnConnection)) {
                 Q_EMIT activateVpnConnection(m_connection->path(), DevicePath);
             } else {
                 if (static_cast<int>(m_connType) == static_cast<int>(ConnectionEditPage::WirelessConnection))
                     Q_EMIT activateWirelessConnection(m_connectionSettings->id(), m_connectionUuid);
                 QDBusPendingReply<> reply = activateConnection(m_connection->path(), DevicePath, QString());
                 reply.waitForFinished();
             }
         }
     }

    Q_EMIT back();
}

void ConnectionEditPage::createConnSettings()
{
    m_connectionSettings = QSharedPointer<ConnectionSettings>(
                               new ConnectionSettings(m_connType));

    // do not handle vpn name here
    QString connName;
    switch (m_connType) {
    case ConnectionSettings::ConnectionType::Wired: {
        connName = tr("Wired Connection %1");
        break;
    }
    case ConnectionSettings::ConnectionType::Wireless: {
        if (m_isHotSpot) {
            connName = tr("Hotspot");
        }
        m_connectionSettings->setting(Setting::Security8021x).staticCast<Security8021xSetting>()->setPasswordFlags(Setting::None);
        break;
    }
    case ConnectionSettings::ConnectionType::Pppoe: {
        connName = tr("PPPoE Connection %1");
        break;
    }
    default:
        break;
    }

    if (!connName.isEmpty()) {
        m_connectionSettings->setId(connName.arg(connectionSuffixNum(connName)));
    }
    m_connectionUuid = m_connectionSettings->createNewUuid();
    while (findConnectionByUuid(m_connectionUuid)) {
        qint64 second = QDateTime::currentDateTime().toSecsSinceEpoch();
        m_connectionUuid.replace(24, QString::number(second).length(), QString::number(second));
    }
    m_connectionSettings->setUuid(m_connectionUuid);
}

int ConnectionEditPage::connectionSuffixNum(const QString &matchConnName)
{
    if (matchConnName.isEmpty()) {
        return 0;
    }

    Connection::List connList = listConnections();
    QStringList connNameList;
    int connSuffixNum = 1;

    for (auto conn : connList) {
        if (conn->settings()->connectionType() == m_connType) {
            connNameList.append(conn->name());
        }
    }

    for (int i = 1; i <= connNameList.size(); ++i) {
        if (!connNameList.contains(matchConnName.arg(i))) {
            connSuffixNum = i;
            break;
        } else if (i == connNameList.size()) {
            connSuffixNum = i + 1;
        }
    }

    return connSuffixNum;
}

void ConnectionEditPage::addHeaderButton(QPushButton *button)
{
    m_mainLayout->insertWidget(m_mainLayout->indexOf(m_buttonTuple_conn) + 1, button);
}

bool ConnectionEditPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Show && obj == m_removeBtn) {
        QWidget *widget = static_cast<QWidget *>(obj);
        bool visible = "Hidden" != GSettingWatcher::instance()->getStatus("removeConnection");
        if (m_isNewConnection)
            widget->setVisible(false);
        else if (visible != widget->isVisible())
            widget->setVisible(visible);
    }
    return QObject::eventFilter(obj, event);
}

void ConnectionEditPage::setFrameProxy(dccV20::FrameProxyInterface *frame)
{
    m_frame = frame;
}

void ConnectionEditPage::setButtonTupleEnable(bool enable)
{
    m_buttonTuple->leftButton()->setEnabled(enable);
    m_buttonTuple->rightButton()->setEnabled(enable);
}

void ConnectionEditPage::setLeftButtonEnable(bool enable)
{
    m_buttonTuple->leftButton()->setEnabled(enable);
}
