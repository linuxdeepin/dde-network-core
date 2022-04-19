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
#ifndef CONTROLLITEMSMODEL_H
#define CONTROLLITEMSMODEL_H

#include <QAbstractItemModel>

#include <DStyledItemDelegate>

class QModelIndex;
struct ControllItemsAction;

namespace dde {
namespace network {
class ControllItems;
class WiredDevice;
}
}
DWIDGET_BEGIN_NAMESPACE
class DSpinner;
DWIDGET_END_NAMESPACE

class ControllItemsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum DisplayRole{
        id,
        ssid,
    };
    explicit ControllItemsModel(QWidget *parent = nullptr);
    virtual ~ControllItemsModel() override;

    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setParentWidget(QWidget *parentWidget);
    void setDisplayRole(DisplayRole role);

Q_SIGNALS:
    void detailClick(dde::network::ControllItems *conn);

public Q_SLOTS:
    void updateDate(QList<dde::network::ControllItems *> conns);
    void addConnection(QList<dde::network::ControllItems *> conns);
    void removeConnection(QList<dde::network::ControllItems *> conns);
    void updateStatus();
    void sortList();

private Q_SLOTS:

    void onDetailTriggered();

private:
    QList<ControllItemsAction *> m_data;
    QWidget *m_parentWidget;
    DisplayRole m_displayRole;
};

#endif // CONTROLLITEMSMODEL_H
