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

#include <QPushButton>
#include <QToolButton>
#include <QLabel>

#include "tqtphone.h"
#include "terror.h"
#include "ui_tqtphone.h"

TQtPhone::TQtPhone(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::TQtPhone)
{
    DECL_TRACER("TQtPhone::TQtPhone(QWidget* parent)");

    ui->setupUi(this);

    ui->label_Number->setText("");
    ui->label_Status->setText("");
    ui->toolButton_Call->setText("");
    ui->toolButton_Call->setIcon(QIcon(":images/pickup.png"));

    connect(ui->pushButton_0, SIGNAL(pressed()), this, SLOT(on_pushButton_0_clicked()));
    connect(ui->pushButton_1, SIGNAL(pressed()), this, SLOT(on_pushButton_1_clicked()));
    connect(ui->pushButton_2, SIGNAL(pressed()), this, SLOT(on_pushButton_2_clicked()));
    connect(ui->pushButton_3, SIGNAL(pressed()), this, SLOT(on_pushButton_3_clicked()));
    connect(ui->pushButton_4, SIGNAL(pressed()), this, SLOT(on_pushButton_4_clicked()));
    connect(ui->pushButton_5, SIGNAL(pressed()), this, SLOT(on_pushButton_5_clicked()));
    connect(ui->pushButton_6, SIGNAL(pressed()), this, SLOT(on_pushButton_6_clicked()));
    connect(ui->pushButton_7, SIGNAL(pressed()), this, SLOT(on_pushButton_7_clicked()));
    connect(ui->pushButton_8, SIGNAL(pressed()), this, SLOT(on_pushButton_8_clicked()));
    connect(ui->pushButton_9, SIGNAL(pressed()), this, SLOT(on_pushButton_9_clicked()));
    connect(ui->pushButton_Star, SIGNAL(pressed()), this, SLOT(on_pushButton_Star_clicked()));
    connect(ui->pushButton_Hash, SIGNAL(pressed()), this, SLOT(on_pushButton_Hash_clicked()));
    connect(ui->pushButton_Clear, SIGNAL(pressed()), this, SLOT(on_pushButton_Clear_clicked()));
    connect(ui->toolButton_Call, SIGNAL(pressed()), this, SLOT(on_toolButton_Call_Clicked()));

    connect(ui->pushButton_Exit, SIGNAL(pressed()), this, SLOT(on_pushButton_Exit_clicked()));
}

TQtPhone::~TQtPhone()
{
    DECL_TRACER("TQtPhone::~TQtPhone()");

    delete ui;
}

void TQtPhone::on_pushButton_0_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_0_clicked()");

    mNumber += "0";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_1_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_1_clicked()");

    mNumber += "1";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_2_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_2_clicked()");

    mNumber += "2";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_3_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_3_clicked()");

    mNumber += "3";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_4_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_4_clicked()");

    mNumber += "4";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_5_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_5_clicked()");

    mNumber += "5";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_6_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_6_clicked()");

    mNumber += "6";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_7_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_7_clicked()");

    mNumber += "7";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_8_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_8_clicked()");

    mNumber += "8";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_9_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_9_clicked()");

    mNumber += "9";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_Hash_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_Hash_clicked()");

    mNumber += "#";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_Star_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_Star_clicked()");

    mNumber += "*";
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_Clear_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_Clear_clicked()");

    mNumber.clear();
    ui->label_Number->setText(mNumber);
}

void TQtPhone::on_pushButton_Exit_clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_Exit_clicked()");

    // FIXME: Enter code to close the dialog
    close();
}

void TQtPhone::on_toolButton_Call_Clicked()
{
    DECL_TRACER("TQtPhone::on_pushButton_0_clicked()");

    // FIXME: Add code to call a number
}

void TQtPhone::doResize()
{
    DECL_TRACER("TQtPhone::doResize()");

    // The main dialog window
    QSize size = this->size();
    QRect rect = this->geometry();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    this->resize(size);
    this->move(scale(rect.left()), scale(rect.top()));
    QWidget *parent = this->parentWidget();

    if (parent)
    {
        rect = parent->geometry();
        this->move(rect.center() - this->rect().center());
    }

    // Layout
    // Iterate through childs and resize them
    QObjectList childs = children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QString name = iter.i->t()->objectName();
        QObject *obj = iter.i->t();

        if (name.startsWith("toolButton"))
            scaleObject(dynamic_cast<QToolButton *>(obj));
        else if (name.startsWith("pushButton"))
            scaleObject(dynamic_cast<QPushButton *>(obj));
        else if (name.startsWith("label"))
            scaleObject(dynamic_cast<QLabel *>(obj));
        else if (name.startsWith("line"))
        {
            QLine *ln = dynamic_cast<QLine *>(obj);
            int x1, y1, x2, y2;
            x1 = scale(ln->x1());
            y1 = scale(ln->y1());
            x2 = scale(ln->x2());
            y2 = scale(ln->y2());
            ln->setLine(x1, y1, x2, y2);
        }
    }
}

template<typename T>
void TQtPhone::scaleObject(T *obj)
{
    DECL_TRACER("TQtPhone::scaleObject(T *obj)");

    QSize size = obj->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    obj->resize(size);
    QRect rect = obj->geometry();
    obj->move(scale(rect.left()), scale(rect.top()));
}

int TQtPhone::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}
