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

#include <QLabel>
#include <QProgressBar>
#include <QTimer>

#include "tqtwait.h"
#include "terror.h"
#include "tpagemanager.h"
#include "tresize.h"
#include "ui_wait.h"

extern TPageManager *gPageManager;

using std::string;
using std::vector;

TQtWait::TQtWait(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::TQtWait)
{
    DECL_TRACER("TQtWait::TQtWait(QWidget *parent = nullptr)");

    ui->setupUi(this);
    ui->labelText->setText("Please wait ...");
}

TQtWait::TQtWait(QWidget *parent, const string& text)
    : QDialog(parent),
      ui(new Ui::TQtWait),
      mText(text)
{
    DECL_TRACER("TQtWait::TQtWait(QWidget *parent, const string& text)");

    ui->setupUi(this);
    ui->labelText->setText(mText.c_str());
}

TQtWait::~TQtWait()
{
    DECL_TRACER("TQtWait::~TQtWait()");

    if (mTimer)
        mTimer->stop();
}

void TQtWait::setText(const string& text)
{
    DECL_TRACER("TQtWait::setText(const string& text)");

    mText = text;
    ui->labelText->setText(text.c_str());
}

void TQtWait::startTimer()
{
    DECL_TRACER("TQtWait::startTimer()");

    if (!mTimer)
        mTimer = new QTimer(this);

    connect(mTimer, &QTimer::timeout, this, &TQtWait::onTimerEvent);
    mTimer->start(200);
}

void TQtWait::onTimerEvent()
{
    DECL_TRACER("TQtWait::onTimerEvent(QTimerEvent *e)");

    mPosition++;

    if (mPosition > 100)
    {
        mPosition = 0;

        if (!mDir)
            mDir = !mDir;

        ui->progressBarWait->setInvertedAppearance(mDir);
    }

    ui->progressBarWait->setValue(mPosition);
}

void TQtWait::start()
{
    DECL_TRACER("TQtWait::start()");

    show();
    startTimer();
}

void TQtWait::end()
{
    DECL_TRACER("TQtWait::end()");

    if (mTimer)
    {
        mTimer->stop();
        delete mTimer;
        mTimer = nullptr;
    }

    close();
}

void TQtWait::doResize()
{
    DECL_TRACER("TQtWait::doResize()");

    vector<TResize::ELEMENTS_t> elem = {
        { "progressBar", TResize::QPROGRESSBAR },
        { "label", TResize::QLABEL }
    };

    TResize::dlgResize(this, elem, mScaleFactor);
}

int TQtWait::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}

template<typename T>
void TQtWait::scaleObject(T *obj)
{
    DECL_TRACER("TQtWait::scaleObject(T *obj): " + obj->objectName().toStdString());

    QSize size = obj->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    obj->resize(size);
    QRect rect = obj->geometry();
    obj->move(scale(rect.left()), scale(rect.top()));
}
