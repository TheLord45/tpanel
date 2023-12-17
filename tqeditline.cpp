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

#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QPainter>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAnyStringView>
#endif

#include "tqeditline.h"
#include "tqsingleline.h"
#include "tqmultiline.h"
#include "terror.h"

using std::string;

TQEditLine::TQEditLine(QWidget *widget, bool multiline)
    : QWidget(widget),
      mMultiline(multiline)
{
    DECL_TRACER("TQEditLine::TQEditLine(QWidget *widget, bool multiline)");

    init();
}

TQEditLine::TQEditLine(string &text, QWidget *widget, bool multiline)
    : QWidget(widget),
      mText(text),
      mMultiline(multiline)

{
    DECL_TRACER("TQEditLine::TQEditLine(string &text, QWidget *widget, bool multiline)");

    init();
}

TQEditLine::~TQEditLine()
{
    DECL_TRACER("TQEditLine::~TQEditLine()");

    if (mMultiline && mTextArea)
    {
        disconnect(mTextArea, &TQMultiLine::textChanged, this, &TQEditLine::onTextAreaChanged);
        disconnect(mTextArea, &TQMultiLine::focusChanged, this, &TQEditLine::onFocusChanged);
        disconnect(mTextArea, &TQMultiLine::keyPressed, this, &TQEditLine::onKeyPressed);
    }
    else if (!mMultiline && mEdit)
    {
        disconnect(mEdit, &TQSingleLine::textChanged, this, &TQEditLine::onTextChanged);
        disconnect(mEdit, &TQSingleLine::cursorPositionChanged, this, &TQEditLine::onCursorPositionChangedS);
        disconnect(mEdit, &TQSingleLine::editingFinished, this, &TQEditLine::onEditingFinished);
        disconnect(mEdit, &TQSingleLine::focusChanged, this, &TQEditLine::onFocusChanged);
        disconnect(mEdit, &TQSingleLine::keyPressed, this, &TQEditLine::onKeyPressed);
    }

    if (mLayout)
        delete mLayout;
}

void TQEditLine::init()
{
    DECL_TRACER("TQEditLine::init()");

    if (!mLayout)
    {
        mLayout = new QHBoxLayout(this);
        mLayout->setSpacing(0);
        mLayout->setContentsMargins(0, 0, 0, 0);

        if (mMultiline)
            mTextArea = new TQMultiLine;
        else
            mEdit = new TQSingleLine;

        QPalette pal(palette());

        pal.setColor(QPalette::Window, Qt::transparent);
        pal.setColor(QPalette::WindowText, Qt::black);
        pal.setColor(QPalette::Text, Qt::black);

        if (!mText.empty())
        {
            if (mMultiline)
                mTextArea->setText(mText.c_str());
            else
                mEdit->setText(mText.c_str());
        }

        if (mMultiline)
        {
            mTextArea->setPalette(pal);

            QWidget::connect(mTextArea, &TQMultiLine::textChanged, this, &TQEditLine::onTextAreaChanged);
            QWidget::connect(mTextArea, &TQMultiLine::focusChanged, this, &TQEditLine::onFocusChanged);
            QWidget::connect(mTextArea, &TQMultiLine::keyPressed, this, &TQEditLine::onKeyPressed);
            mLayout->addWidget(mTextArea);
        }
        else
        {
            mEdit->setPalette(pal);

            QWidget::connect(mEdit, &TQSingleLine::textChanged, this, &TQEditLine::onTextChanged);
            QWidget::connect(mEdit, &TQSingleLine::cursorPositionChanged, this, &TQEditLine::onCursorPositionChangedS);
            QWidget::connect(mEdit, &TQSingleLine::editingFinished, this, &TQEditLine::onEditingFinished);
            QWidget::connect(mEdit, &TQSingleLine::focusChanged, this, &TQEditLine::onFocusChanged);
            QWidget::connect(mEdit, &TQSingleLine::keyPressed, this, &TQEditLine::onKeyPressed);
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

    mChanged = false;
}

void TQEditLine::setObjectName(const string& name)
{
    DECL_TRACER("TQEditLine::setObjectName(const string& name)");

    if (name.empty())
        return;

    QWidget::setObjectName(name.c_str());
    QString editName("Edit#");
    editName.append(name.c_str());

    if (mMultiline && mTextArea)
        mTextArea->setObjectName(editName);
    else if (!mMultiline && mEdit)
        mEdit->setObjectName(editName);

    if (mLayout)
        mLayout->setObjectName(QString("Layout#%1").arg(name.c_str()));
}

void TQEditLine::setPasswordChar(uint c)
{
    DECL_TRACER("TQEditLine::setPasswordChar(uint c)");

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

void TQEditLine::setFont(QFont &font)
{
    DECL_TRACER("TQEditLine::setFont(QFont &font)");

    if (!mMultiline && mEdit)
        mEdit->setFont(font);
    else if (mMultiline && mTextArea)
        mTextArea->setFont(font);
}

void TQEditLine::setPalette(QPalette &pal)
{
    DECL_TRACER("TQEditLine::setPalette(QPalette &pal)");

    QWidget::setPalette(pal);
/*
    if (mMultiline && mTextArea)
        mTextArea->setPalette(pal);
    else if (!mMultiline && mEdit)
        mEdit->setPalette(pal); */
}

void TQEditLine::setTextColor(QColor col)
{
    DECL_TRACER("TQEditLine::setTextColor(QColor col)");

    QPalette pal;

    if (!mMultiline && mEdit)
        pal = mEdit->palette();
    else if (mMultiline && mTextArea)
        pal = mTextArea->palette();
    else
        return;

    pal.setColor(QPalette::WindowText, col);
    pal.setColor(QPalette::Text, col);

    if (!mMultiline)
        mEdit->setPalette(pal);
    else
        mTextArea->setPalette(pal);
}

void TQEditLine::setBackgroundPixmap(QPixmap& pixmap)
{
    DECL_TRACER("TQEditLine::setBackgroundPixmap(QPixmap& pixmap)");

    if (pixmap.isNull())
        return;

    mBackground = pixmap;
    update();
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

void TQEditLine::setWordWrapMode(bool mode)
{
    DECL_TRACER("TQEditLine::setWordWrapMode(bool mode)");

    if (!mMultiline || !mTextArea)
        return;

    mTextArea->setWordWrapMode((mode ? QTextOption::WordWrap : QTextOption::NoWrap));
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

void TQEditLine::setInputMask(const std::string& mask)
{
    DECL_TRACER("TQEditLine::setInputMask(const std::string& mask)");

    if (!mMultiline && mEdit)
        mEdit->setInputMask(mask.c_str());
}

void TQEditLine::setNumericInput()
{
    DECL_TRACER("TQEditLine::setNumericInput()");

    if (!mMultiline && mEdit)
        mEdit->setInputMethodHints(mEdit->inputMethodHints() | Qt::ImhDigitsOnly);
}

#ifndef __ANDROID__
void TQEditLine::setClearButtonEnabled(bool state)
{
    DECL_TRACER("TQEditLine::setClearButtonEnabled(bool state)");

    if (!mMultiline && mEdit)
        mEdit->setClearButtonEnabled(state);
}

void TQEditLine::setCursor(const QCursor& qc)
{
    DECL_TRACER("TQEditLine::setCursor(const QCursor& qc)");

    if (mMultiline && mTextArea)
        mTextArea->setCursor(qc);
    else if (!mMultiline && mEdit)
        mEdit->setCursor(qc);
}
#endif

/*
 * Here the signal and callback functions follow.
 */
void TQEditLine::onKeyPressed(int key)
{
    DECL_TRACER("TQEditLine::onKeyPressed(int key)");

    if (key == Qt::Key_Enter || key == Qt::Key_Return)
    {
        string txt;

        if (mMultiline && mTextArea)
            txt = mTextArea->toPlainText().toStdString();
        else if (!mMultiline && mEdit)
            txt = mEdit->text().toStdString();

        if (mChanged || txt != mText)
        {
            emit inputChanged(mHandle, mText);
            mChanged = false;
        }
    }

    QApplication::processEvents();
}

void TQEditLine::hideEvent(QHideEvent *event)
{
    DECL_TRACER("TQEditLine::hideEvent(QHideEvent *event)");

    Q_UNUSED(event);
    _end();
}

void TQEditLine::leaveEvent(QEvent *event)
{
    DECL_TRACER("TQEditLine::leaveEvent(QEvent *event)");

    Q_UNUSED(event);
    _end();
}

void TQEditLine::closeEvent(QCloseEvent *event)
{
    DECL_TRACER("TQEditLine::closeEvent(QCloseEvent *event)");

    Q_UNUSED(event);
    _end();
}

void TQEditLine::onFocusChanged(bool in)
{
    DECL_TRACER("TQEditLine::onFocusChanged(bool in)");

    emit focusChanged(mHandle, in);
}

void TQEditLine::_end()
{
    DECL_TRACER("TQEditLine::_end()");

    string text;

    if (mMultiline && mTextArea)
        text = mTextArea->toPlainText().toStdString();
    else if (!mMultiline && mEdit)
        text = mEdit->text().toStdString();

    if (mChanged || text != mText)
    {
        emit inputChanged(mHandle, mText);
        mChanged = false;
    }
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

void TQEditLine::onCursorPositionChangedS(int oldPos, int newPos)
{
    DECL_TRACER("TQEditLine::onCursorPositionChangedS(int oldPos, int newPos)");

    emit cursorPositionChanged(mHandle, oldPos, newPos);
}

void TQEditLine::onEditingFinished()
{
    DECL_TRACER("TQEditLine::onEditingFinished()");

    _end();
}

void TQEditLine::paintEvent(QPaintEvent* event)
{
    DECL_TRACER("TQEditLine::paintEvent(QPaintEvent* event)");

    if (mBackground.isNull())
        return;

    QPainter p(this);
    p.drawPixmap(0, 0, mBackground);
}
