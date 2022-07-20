/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <QLabel>
#include <QUrl>
#include <QtQuickWidgets/QQuickWidget>

#include "tqtbusy.h"
#include "ui_busy.h"
#include "terror.h"

TQBusy::TQBusy(const std::string& msg, QWidget* parent)
    : QDialog(parent),
      ui(new Ui::TQBusy)
{
    DECL_TRACER("TQBusy::TQBusy(const std::string& init, const std::string& prompt, QWidget* parent, bool priv)");

    ui->setupUi(this);
    ui->quickWidget->setSource(QUrl("qrc:qrc/BusyIndicator.qml"));
    ui->label_Download->setText(msg.c_str());
}

TQBusy::~TQBusy()
{
    DECL_TRACER("TQBusy::~TQBusy()");

    delete ui;
}

void TQBusy::doResize()
{
    DECL_TRACER("TQBusy::doResize()");

    if (mScaleFactor == 0.0 || mScaleFactor == 1.0)
        return;

    QRect rect = this->geometry();
    QSize size = this->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    this->resize(size);
    this->move(scale(rect.left()), scale(rect.top()));
    QWidget *parent = this->parentWidget();

    if (parent)
    {
        rect = parent->geometry();
        this->move(rect.center() - this->rect().center());
    }

    // Iterate through childs and resize them
    QObjectList childs = children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QObject *obj = *iter;
        QString name = obj->objectName();

        if (name.startsWith("quick"))
        {
            QQuickWidget *bt = dynamic_cast<QQuickWidget *>(obj);
            size = bt->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            bt->resize(size);
            rect = bt->geometry();
            bt->move(scale(rect.left()), scale(rect.top()));
        }
        else if (name.startsWith("label"))   // It's a label
        {
            QLabel *lb = dynamic_cast<QLabel *>(obj);
            size = lb->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            lb->resize(size);
            rect = lb->geometry();
            lb->move(scale(rect.left()), scale(rect.top()));
        }
    }
}

int TQBusy::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}
