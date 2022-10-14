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

#include "tqeditline.h"
#include "tpagemanager.h"
#include "terror.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QAnyStringView>

using std::string;

TQEditLine::TQEditLine(QWidget *widget, bool multiline)
    : mParent(widget),
      mMultiline(multiline)
{
    DECL_TRACER("TQEditLine::TQEditLine(QWidget *widget, bool multiline)");

    QWidget::setParent(widget);
    init();
}

TQEditLine::TQEditLine(string &text, QWidget *widget, bool multiline)
    : mParent(widget),
      mText(text),
      mMultiline(multiline)

{
    DECL_TRACER("TQEditLine::TQEditLine(string &text, QWidget *widget, bool multiline)");

    QWidget::setParent(widget);
    init();
}

TQEditLine::~TQEditLine()
{
    DECL_TRACER("TQEditLine::~TQEditLine()");

//    if (mEdit || mTextArea)
//        QWidget::close();
}

void TQEditLine::init()
{
    DECL_TRACER("TQEditLine::init()");

    QWidget::setAttribute(Qt::WA_DeleteOnClose);
    QWidget::setAttribute(Qt::WA_LayoutOnEntireRect);
    QWidget::setAttribute(Qt::WA_LayoutUsesWidgetRect);

    if (!mLayout)
    {
        mLayout = new QHBoxLayout(this);
        mLayout->setContentsMargins(0, 0, 0, 0);

        if (mMultiline)
            mTextArea = new QTextEdit;
        else
            mEdit = new QLineEdit;

        QPalette pal;

        pal.setColor(QPalette::Window, QColorConstants::Transparent);

        if (!mText.empty())
        {
            if (mMultiline)
                mTextArea->setText(mText.c_str());
            else
                mEdit->setText(mText.c_str());
        }

        if (mMultiline)
        {
            mTextArea->setAutoFillBackground(true);
            mTextArea->setPalette(pal);

            QWidget::connect(mTextArea, &QTextEdit::textChanged, this, &TQEditLine::onTextAreaChanged);
            mLayout->addWidget(mTextArea);
        }
        else
        {
            mEdit->setAutoFillBackground(true);
            mEdit->setPalette(pal);

            QWidget::connect(mEdit, &QLineEdit::textChanged, this, &TQEditLine::onTextChanged);
            mLayout->addWidget(mEdit);
        }

        QWidget::setLayout(mLayout);
    }
}

void TQEditLine::setText(string &text)
{
    DECL_TRACER("TQEditLine::setText(string &text)");

    mText = text;

    if (mMultiline && mTextArea)
        mTextArea->setText(text.c_str());
    else if (!mMultiline && mEdit)
        mEdit->setText(text.c_str());
}

void TQEditLine::setPasswordChar(uint c)
{
    DECL_TRACER("TQEditLine::setPasswordString(const string &str)");

    if (!mMultiline && mEdit && c)
    {
        char style[256];
        snprintf(style, sizeof(style), "lineedit-password-character: %d", c);
        mEdit->setStyleSheet(style);
        mEdit->setEchoMode(QLineEdit::Password);
    }
}

void TQEditLine::setFixedSize(int w, int h)
{
    DECL_TRACER("TQEditLine::setFixedSize(int w, int h)");

    if (mLayout && w > 0 && h > 0)
    {
        QWidget::setFixedSize(w, h);

        if (mMultiline)
            mTextArea->setFixedSize(w - 1 - mPadLeft - mPadRight, h - 1 - mPadTop - mPadBottom);
        else
            mEdit->setFixedSize(w - 1 - mPadLeft - mPadRight, h - 1 - mPadTop - mPadBottom);

        mWidth = w;
        mHeight = h;
    }
}

void TQEditLine::move(QPoint &p)
{
    DECL_TRACER("TQEditLine::move(QPoint &p)");

    mPosX = p.x();
    mPosY = p.y();
    QWidget::move(p);
}

void TQEditLine::move(int x, int y)
{
    DECL_TRACER("TQEditLine::move(int x, int y)");

    if (mLayout && x >= 0 && x >= 0)
    {
        QWidget::move(x, y);
        mPosX = x;
        mPosY = y;
    }
}

void TQEditLine::setFont(QFont &font)
{
    DECL_TRACER("TQEditLine::setFont(QFont &font)");

    if (mEdit)
        mEdit->setFont(font);
}

void TQEditLine::setPalette(QPalette &pal)
{
    DECL_TRACER("TQEditLine::setPalette(QPalette &pal)");

    QWidget::setPalette(pal);
}

void TQEditLine::grabGesture(Qt::GestureType type, Qt::GestureFlags flags)
{
    DECL_TRACER("TQEditLine::grabGesture(Qt::GestureType type, Qt::GestureFlags flags)");

    if (mMultiline && mTextArea)
        mTextArea->grabGesture(type, flags);
    else if (!mMultiline && mEdit)
        mEdit->grabGesture(type, flags);
}

void TQEditLine::setPadding(int left, int top, int right, int bottom)
{
    DECL_TRACER("TQEditLine::setPadding(int left, int top, int right, int bottom)");

    mPadLeft = (left < 0 ? 0 : left);
    mPadTop = (top < 0 ? 0 : top);
    mPadRight = (right < 0 ? 0 : right);
    mPadBottom = (bottom < 0 ? 0 : bottom);

    if (mMultiline && mTextArea)
        mTextArea->setFixedSize(mWidth - 1 - mPadLeft - mPadRight, mHeight - 1 - mPadTop - mPadBottom);
    else if (!mMultiline && mEdit)
        mEdit->setFixedSize(mWidth - 1 - mPadLeft - mPadRight, mHeight - 1 - mPadTop - mPadBottom);

    mLayout->setContentsMargins(mPadLeft, mPadTop, mPadRight, mPadBottom);
}

void TQEditLine::setFrameSize(int s)
{
    DECL_TRACER("TQEditLine::setFrameSize(int s)");

    setPadding(s + mPadLeft, s + mPadTop, s + mPadRight, s + mPadBottom);
}

void TQEditLine::setAutoFillBackground(bool f)
{
    DECL_TRACER("TQEditLine::setAutoFillBackground(bool f)");

    QWidget::setAutoFillBackground(f);
}

void TQEditLine::installEventFilter(QObject *filter)
{
    DECL_TRACER("TQEditLine::installEventFilter(QObject *filter)");

    QWidget::installEventFilter(filter);
}

void TQEditLine::setWordWrapMode(bool mode)
{
    DECL_TRACER("TQEditLine::setWordWrapMode(bool mode)");

    if (!mMultiline || !mTextArea)
        return;

    mTextArea->setWordWrapMode((mode ? QTextOption::WordWrap : QTextOption::NoWrap));
}

WId TQEditLine::winId()
{
    DECL_TRACER("TQEditLine::winId()");

    return QWidget::winId();
}

void TQEditLine::show()
{
    DECL_TRACER("TQEditLine::show()");

    QWidget::show();
}

void TQEditLine::close()
{
    DECL_TRACER("TQEditLine::close()");

    QWidget::close();
    mLayout = nullptr;
    mEdit = nullptr;
}

void TQEditLine::clear()
{
    DECL_TRACER("TQEditLine::clear()");

    if (mMultiline && mTextArea)
        mTextArea->clear();
    else if (!mMultiline && mEdit)
        mEdit->clear();

    mText.clear();
}

/*
 * Here the signal and callback functions follow.
 */

bool TQEditLine::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *sKey = static_cast<QKeyEvent*>(event);

        if (sKey && (sKey->key() == Qt::Key_Enter || sKey->key() == Qt::Key_Return))
        {
            if (mMultiline && mTextArea)
                mText = mTextArea->toPlainText().toStdString();
            else if (!mMultiline && mEdit)
                mText = mEdit->text().toStdString();
            else
                return true;

            if (mChanged && gPageManager)
                gPageManager->inputButtonFinished(mHandle, mText);

            return true;
        }
    }
    else if (event->type() == QEvent::Close ||
             event->type() == QEvent::Leave ||
             event->type() == QEvent::FocusOut ||
             event->type() == QEvent::Hide ||
             event->type() == QEvent::WindowDeactivate)
    {
        if (mMultiline && mTextArea)
            mText = mTextArea->toPlainText().toStdString();
        else if (!mMultiline && mEdit)
            mText = mEdit->text().toStdString();
        else
            return QWidget::event(event);

        if (mChanged && gPageManager)
            gPageManager->inputButtonFinished(mHandle, mText);
    }

    return QWidget::event(event);
}

void TQEditLine::onTextChanged(const QString &text)
{
    DECL_TRACER("TQEditLine::onTextChanged(const QString &text)");

    mText = text.toStdString();
    mChanged = true;
}

void TQEditLine::onTextAreaChanged()
{
    DECL_TRACER("TQEditLine::onTextAreaChanged()");

    mText = mTextArea->toPlainText().toStdString();
    mChanged = true;
}
