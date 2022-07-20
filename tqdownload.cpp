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
#include <QProgressBar>

#include "tqdownload.h"
#include "ui_download.h"
#include "terror.h"

TqDownload::TqDownload(const std::string &msg, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::TqDownload)
{
    DECL_TRACER("TqDownload::TqDownload(const std::string& msg, QWidget* parent)");

    ui->setupUi(this);
    ui->labelInfo->setText(msg.c_str());
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(0);
}

TqDownload::~TqDownload()
{
    DECL_TRACER("TqDownload::~TqDownload()");

    delete ui;
}

void TqDownload::setProgress(int percent)
{
    DECL_TRACER("TqDownload::setProgress(int percent)");

    int p = percent;

    if (p < 0)
        p = 0;
    else if (p > 100)
        p = 100;

    ui->progressBar->setValue(p);
}

void TqDownload::doResize()
{
    DECL_TRACER("TqDownload::doResize()");

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

        if (name.startsWith("progressBar"))
        {
            QProgressBar *pb = dynamic_cast<QProgressBar *>(obj);
            size = pb->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            pb->resize(size);
            rect = pb->geometry();
            pb->move(scale(rect.left()), scale(rect.top()));
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

int TqDownload::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}
