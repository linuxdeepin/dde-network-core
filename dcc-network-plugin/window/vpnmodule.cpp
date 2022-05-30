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
#include "controllitemsmodel.h"
#include "vpnmodule.h"
#include "editpage/connectionvpneditpage.h"
#include "widgets/floatingbutton.h"

#include <widgets/switchwidget.h>
#include <widgets/widgetmodule.h>

#include <DFloatingButton>
#include <DFontSizeManager>
#include <DListView>
#include <DSwitchButton>

#include <QHBoxLayout>
#include <QApplication>
#include <QFileDialog>
#include <DDialog>
#include <QStandardPaths>
#include <QProcess>
#include <QTimer>

#include <networkcontroller.h>
#include <vpncontroller.h>

#include <networkmanagerqt/settings.h>

using namespace dde::network;
DCC_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

QString vpnConfigType(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();

    const QString content = f.readAll();
    f.close();

    if (content.contains("openconnect"))
        return "openconnect";
    if (content.contains("l2tp"))
        return "l2tp";
    if (content.startsWith("[main]"))
        return "vpnc";

    return "openvpn";
}

VPNModule::VPNModule(QObject *parent)
    : ModuleObject("networkVpn", tr("VPN"), tr("VPN"), QIcon::fromTheme("dcc_vpn"), parent)
{
    setChildType(ModuleObject::Page);

    m_modules.append(new WidgetModule<SwitchWidget>("wired_adapter", tr("Wired Network Adapter"), [](SwitchWidget *vpnSwitch) {
        QLabel *lblTitle = new QLabel(tr("VPN Status"));
        DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
        vpnSwitch->setLeftWidget(lblTitle);
        vpnSwitch->switchButton()->setAccessibleName(lblTitle->text());
        vpnSwitch->setChecked(NetworkController::instance()->vpnController()->enabled());
        connect(vpnSwitch, &SwitchWidget::checkedChanged, NetworkController::instance()->vpnController(), &VPNController::setEnabled);
        connect(NetworkController::instance()->vpnController(), &VPNController::enableChanged, vpnSwitch, [vpnSwitch](const bool enabled) {
            vpnSwitch->blockSignals(true);
            vpnSwitch->setChecked(enabled);
            vpnSwitch->blockSignals(false);
        });
        auto updateVisible = [vpnSwitch]() {
            bool visible = !NetworkController::instance()->vpnController()->items().isEmpty();
            vpnSwitch->setVisible(visible);
        };
        updateVisible();
        connect(NetworkController::instance()->vpnController(), &VPNController::itemAdded, vpnSwitch, updateVisible);
        connect(NetworkController::instance()->vpnController(), &VPNController::itemRemoved, vpnSwitch, updateVisible);
    }));
    m_modules.append(new WidgetModule<DListView>("List_wirelesslist", tr("Wired List"), this, &VPNModule::initVPNList));
    ModuleObject *extraCreate = new WidgetModule<FloatingButton>("addWired", tr("Add Network Connection"), [this](FloatingButton *createVpnBtn) {
        createVpnBtn->setIcon(DStyle::StandardPixmap::SP_IncreaseElement);
        createVpnBtn->setMinimumSize(QSize(47, 47));
        createVpnBtn->setToolTip(tr("Create VPN"));
        createVpnBtn->setAccessibleName(tr("Create VPN"));
        connect(createVpnBtn, &DFloatingButton::clicked, this, [this]() {
            editConnection(nullptr);
        });
    });
    extraCreate->setExtra();
    m_modules.append(extraCreate);
    ModuleObject *extraImport = new WidgetModule<FloatingButton>("addWired", tr("Add Network Connection"), [this](FloatingButton *importVpnBtn) {
        importVpnBtn->QAbstractButton::setText("\342\206\223");
        importVpnBtn->setMinimumSize(QSize(47, 47));
        importVpnBtn->setToolTip(tr("Import VPN"));
        importVpnBtn->setAccessibleName(tr("Import VPN"));
        connect(importVpnBtn, &DFloatingButton::clicked, this, &VPNModule::importVPN);
    });
    extraImport->setExtra();
    m_modules.append(extraImport);
    for (auto &it : m_modules) {
        appendChild(it);
    }
}

void VPNModule::initVPNList(DListView *vpnView)
{
    vpnView->setAccessibleName("List_vpnList");
    ControllItemsModel *model = new ControllItemsModel(vpnView);
    auto updateItems = [model, this]() {
        const QList<VPNItem *> conns = NetworkController::instance()->vpnController()->items();
        QList<ControllItems *> items;
        for (VPNItem *it : conns) {
            items.append(it);
            if (!m_newConnectionPath.isEmpty() && it->connection()->path() == m_newConnectionPath) {
                NetworkController::instance()->vpnController()->connectItem(it);
                m_newConnectionPath.clear();
            }
        }
        model->updateDate(items);
    };
    VPNController *vpnController = NetworkController::instance()->vpnController();
    updateItems();
    connect(vpnController, &VPNController::itemAdded, model, updateItems);
    connect(vpnController, &VPNController::itemRemoved, model, updateItems);
    connect(vpnController, &VPNController::itemChanged, model, &ControllItemsModel::updateStatus);
    connect(vpnController, &VPNController::activeConnectionChanged, model, &ControllItemsModel::updateStatus);
    connect(vpnController, &VPNController::enableChanged, model, &ControllItemsModel::updateStatus);

    vpnView->setModel(model);
    vpnView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vpnView->setBackgroundType(DStyledItemDelegate::BackgroundType::ClipCornerBackground);
    vpnView->setSelectionMode(QAbstractItemView::NoSelection);
    vpnView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto adjustHeight = [vpnView]() {
        vpnView->setMinimumHeight(vpnView->model()->rowCount() * 41);
    };
    adjustHeight();
    connect(model, &ControllItemsModel::modelReset, vpnView, adjustHeight);
    connect(model, &ControllItemsModel::detailClick, this, &VPNModule::editConnection);
    connect(vpnView, &DListView::clicked, this, [](const QModelIndex &idx) {
        VPNItem *item = static_cast<VPNItem *>(idx.internalPointer());
        if (!item->connected())
            NetworkController::instance()->vpnController()->connectItem(item);
    });
}

void VPNModule::editConnection(ControllItems *item)
{
    VPNItem *vpn = static_cast<VPNItem *>(item);
    QString connUuid;
    if (vpn) {
        connUuid = vpn->connection()->uuid();
    }
    ConnectionVpnEditPage *m_editPage = new ConnectionVpnEditPage(connUuid, qApp->activeWindow());
    if (vpn) {
        m_editPage->initSettingsWidget();
        m_editPage->setLeftButtonEnable(true);
    } else {
        m_editPage->initSettingsWidgetByType(ConnectionVpnEditPage::VpnType::UNSET);
        m_editPage->setButtonTupleEnable(true);
    }
    connect(m_editPage, &ConnectionVpnEditPage::disconnect, this, [=] {
        VPNController *vpnController = NetworkController::instance()->vpnController();
        vpnController->disconnectItem();
    });
    connect(m_editPage, &ConnectionVpnEditPage::activateVpnConnection, this, [vpn, this](const QString &path, const QString &devicePath) {
        Q_UNUSED(devicePath);
        VPNController *vpnController = NetworkController::instance()->vpnController();
        if (vpn) {
            vpnController->connectItem(vpn);
        } else {
            m_newConnectionPath.clear();
            bool findConnection = false;
            QList<VPNItem *> items = vpnController->items();
            for (VPNItem *item : items) {
                if (item->connection()->path() == path) {
                    vpnController->connectItem(item);
                    findConnection = true;
                    break;
                }
            }
            if (!findConnection)
                m_newConnectionPath = path;
        }
    });
    m_editPage->exec();
    m_editPage->deleteLater();
}

void VPNModule::importVPN()
{
    QFileDialog *m_importFile = new QFileDialog(qApp->activeWindow());
    m_importFile->setAccessibleName("VpnPage_importFile");
    m_importFile->setModal(true);
    m_importFile->setNameFilter("*.conf");
    m_importFile->setAcceptMode(QFileDialog::AcceptOpen);
    QStringList directory = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
    if (!directory.isEmpty()) {
        m_importFile->setDirectory(directory.first());
    }

    connect(m_importFile, &QFileDialog::finished, this, [=](int result) {
        if (result == QFileDialog::Accepted) {
            QString file = m_importFile->selectedFiles().first();
            if (file.isEmpty())
                return;

            const auto args = QStringList{ "connection", "import", "type", vpnConfigType(file), "file", file };

            QProcess p;
            p.start("nmcli", args);
            p.waitForFinished();
            const auto stat = p.exitCode();
            const QString output = p.readAllStandardOutput();
            QString error = p.readAllStandardError();

            qDebug() << stat << ",output:" << output << ",err:" << error;

            if (stat) {
                const auto ratio = qApp->activeWindow()->devicePixelRatioF();
                QPixmap icon = QIcon::fromTheme("dialog-error").pixmap(QSize(48, 48) * ratio);
                icon.setDevicePixelRatio(ratio);

                DDialog dialog(qApp->activeWindow());
                dialog.setTitle(tr("Import Error"));
                dialog.setMessage(tr("File error"));
                dialog.addButton(tr("OK"));
                dialog.setIcon(icon);
                dialog.exec();
                return;
            }

            const QRegularExpression regexp("\\(\\w{8}(-\\w{4}){3}-\\w{12}\\)");
            const auto match = regexp.match(output);

            if (match.hasMatch()) {
                m_editingConnUuid = match.captured();
                m_editingConnUuid.replace("(", "");
                m_editingConnUuid.replace(")", "");
                qDebug() << "editing connection Uuid";
                QTimer::singleShot(10, this, &VPNModule::changeVpnId);
            }
        }
    });
    m_importFile->exec();
    m_importFile->deleteLater();
}

void VPNModule::changeVpnId()
{
    NetworkManager::Connection::List connList = NetworkManager::listConnections();
    QString importName = "";
    for (const auto &conn : connList) {
        if (conn->settings()->connectionType() == NetworkManager::ConnectionSettings::Vpn) {
            if (m_editingConnUuid == conn->uuid()) {
                importName = conn->name();
                break;
            }
        }
    }
    if (importName.isEmpty()) {
        QTimer::singleShot(10, this, &VPNModule::changeVpnId);
        return;
    }

    QString changeName = "";
    bool hasSameName = false;
    for (const auto &conn : connList) {
        const QString vpnName = conn->name();
        const QString vpnUuid = conn->uuid();
        if ((vpnName == importName) && (vpnUuid != m_editingConnUuid)) {
            changeName = importName + "(1)";
            hasSameName = true;
            break;
        }
    }
    if (!hasSameName) {
        return;
    }

    for (int index = 1;; index++) {
        hasSameName = false;
        for (const auto &conn : connList) {
            QString vpnName = conn->name();
            if (vpnName == changeName) {
                changeName = importName + "(%1)";
                changeName = changeName.arg(index);
                hasSameName = true;
                break;
            }
        }
        if (!hasSameName) {
            break;
        }
    }

    NetworkManager::Connection::Ptr uuidConn = NetworkManager::findConnectionByUuid(m_editingConnUuid);
    if (uuidConn) {
        NetworkManager::ConnectionSettings::Ptr connSettings = uuidConn->settings();
        connSettings->setId(changeName);
        // update function saves the settings on the hard disk
        QDBusPendingReply<> reply = uuidConn->update(connSettings->toMap());
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "error occurred while updating the connection" << reply.error();
            return;
        }
        qDebug() << "find Connection By Uuid is success";
        return;
    }
}
