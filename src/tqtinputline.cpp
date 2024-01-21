/*
 * Copyright (C) 2023 by Andreas Theofilu <andreas@theosys.at>
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
#include <QLineEdit>

#include "tqtinputline.h"
#include "terror.h"
#include "ui_tqtinputline.h"

using std::string;

TQtInputLine::TQtInputLine(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::TQtInputLine)
{
    DECL_TRACER("TQtInputLine::TQtInputLine(QWidget* parent)");

    ui->setupUi(this);

    ui->labelMessage->setText("Input information");
    ui->lineEdit->setClearButtonEnabled(true);
    ui->lineEdit->setFocus();
#ifdef Q_OS_IOS
    setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, true);
#endif
}

TQtInputLine::~TQtInputLine()
{
    DECL_TRACER("TQtInputLine::~TQtInputLine()");
}

void TQtInputLine::setText(const string& text)
{
    DECL_TRACER("TQtInputLine::setText(const string& text)");

    mText = text;
    ui->lineEdit->setText(text.c_str());
}

void TQtInputLine::setMessage(const string& msg)
{
    DECL_TRACER("TQtInputLine::setMessage(const string& msg)");

    ui->labelMessage->setText(msg.c_str());
}

void TQtInputLine::setPassword(bool pw)
{
    DECL_TRACER("TQtInputLine::setPassword(bool pw)");

    ui->lineEdit->setEchoMode(pw ? QLineEdit::Password : QLineEdit::Normal);
}

void TQtInputLine::doResize()
{
    DECL_TRACER("TQtInputLine::doResize()");

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
        QObject *obj = *iter;
        QString name = obj->objectName();

        if (name.startsWith("toolButton"))
            scaleObject(dynamic_cast<QToolButton *>(obj));
        else if (name.startsWith("pushButton"))
            scaleObject(dynamic_cast<QPushButton *>(obj));
        else if (name.startsWith("label"))
            scaleObject(dynamic_cast<QLabel *>(obj));
        else if (name.startsWith("line"))
            scaleObject(dynamic_cast<QFrame *>(obj));
        else if (name.startsWith("lineEdit"))
            scaleObject(dynamic_cast<QLineEdit *>(obj));
    }
}

void TQtInputLine::on_lineEdit_textChanged(const QString& arg1)
{
    DECL_TRACER("TQtInputLine::on_lineEdit_textChanged(const QString& arg1)");

    if (arg1.compare(mText.c_str()) == 0)
        return;

    mText = arg1.toStdString();
}

template<typename T>
void TQtInputLine::scaleObject(T *obj)
{
    DECL_TRACER("TQtInputLine::scaleObject(T *obj): " + obj->objectName().toStdString());

    QSize size = obj->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    obj->resize(size);
    QRect rect = obj->geometry();
    obj->move(scale(rect.left()), scale(rect.top()));
}

int TQtInputLine::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}
