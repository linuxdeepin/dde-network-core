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
#ifndef QUICKPANEL_H
#define QUICKPANEL_H

#include <QIcon>
#include <QVariant>
#include <QWidget>

namespace Dtk {
namespace Widget {
class DLabel;
class DIconButton;
}
}

class QuickPanel : public QWidget
{
    Q_OBJECT

public:
    explicit QuickPanel(QWidget *parent = nullptr);
    const QVariant &userData() const;
    void setUserData(const QVariant &data);
    const QString text() const;
    const QString description() const;

public Q_SLOTS:
    void setIcon(const QIcon &icon);
    void setText(const QString &text);
    void setDescription(const QString &description);
    void setActive(bool active);

Q_SIGNALS:
    void panelClicked();
    void iconClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initUi();
    void initConnect();
    void updateIconPixmap();
    void setHover(bool hover);

private:
    QIcon m_icon;
    QVariant m_userData;

    Dtk::Widget::DIconButton *m_iconButton;
    Dtk::Widget::DLabel *m_text;
    Dtk::Widget::DLabel *m_description;
    QPixmap m_iconPixmap;

    bool m_hover;
    bool m_active;
};

#endif // QUICKPANEL_H
