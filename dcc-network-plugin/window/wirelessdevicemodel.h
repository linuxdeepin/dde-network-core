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
#ifndef WirelessDeviceModel_H
#define WirelessDeviceModel_H

#include <QAbstractItemModel>

#include <DStyledItemDelegate>

class QModelIndex;
struct ItemAction;

namespace dde {
namespace network {
class AccessPoints;
class WirelessDevice;
}
}
DWIDGET_BEGIN_NAMESPACE
class DSpinner;
DWIDGET_END_NAMESPACE

class WirelessDeviceModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit WirelessDeviceModel(const dde::network::WirelessDevice *dev, QWidget *parent = nullptr);
    virtual ~WirelessDeviceModel() override;

    QModelIndex index(const dde::network::AccessPoints *ap);
    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

Q_SIGNALS:
    void detailClick(dde::network::AccessPoints *ap);
//    void requestSetDevAlias(const dde::network::AccessPoints *ap, const QString &devAlias);

public Q_SLOTS:
    void updateApStatus();
    void sortAPList();

private Q_SLOTS:
    void addAccessPoints(QList<dde::network::AccessPoints *> aps);
    void removeAccessPoints(QList<dde::network::AccessPoints *> aps);

    void onDetailTriggered();

private:
    QList<ItemAction *> m_data;
    const dde::network::WirelessDevice *m_device;
    QWidget *m_parent;
    ItemAction *m_hiddenItem;
};

#endif // WirelessDeviceModel_H
