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

#include <QEvent>
#include <QKeyEvent>

#include "tqtextedit.h"
#include "terror.h"

using std::string;

TQTextEdit::TQTextEdit()
{
    DECL_TRACER("TQTextEdit::TQTextEdit()");

    init();
}

TQTextEdit::TQTextEdit(QWidget* parent)
    : QLabel(parent)
{
    DECL_TRACER("TQTextEdit::TQTextEdit(QWidget* parent)");

    init();
}

void TQTextEdit::init()
{
    DECL_TRACER("TQTextEdit::init()");

    setCursor(Qt::IBeamCursor);
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setTextFormat(Qt::PlainText);
}

void TQTextEdit::setText(const std::string& text)
{
    DECL_TRACER("TQTextEdit::setText(const std::string& text)");

    QLabel::setText(QString(text.c_str()));
}

bool TQTextEdit::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *sKey = static_cast<QKeyEvent*>(event);

        if (sKey)
        {
            if (sKey->key() == Qt::Key_Enter || sKey->key() == Qt::Key_Return)
            {
                sigContentChanged(mText);
                return true;
            }
            else if (sKey->key() == Qt::Key_0)
            {
                insert("0");
                return true;
            }
        }
    }

    return false;
}

void TQTextEdit::append(const std::string& txt)
{
    DECL_TRACER("TQTextEdit::append(const std::string& txt)");

    mText += txt;
    QLabel::setText(QString(mText.c_str()));
    mPos = mText.length();
}

void TQTextEdit::insert(const std::string& txt, int pos)
{
    DECL_TRACER("TQTextEdit::insert(const std::string& txt, int pos)");

    if (mText.empty())
    {
        mText = txt;
        QLabel::setText(QString(mText.c_str()));
        mPos = mText.length();
        return;
    }

    if (pos >= 0)
    {
        if ((size_t)pos > mText.length())
            mPos = mText.length();
        else
            mPos = pos;
    }

    if ((size_t)mPos == mText.length())
    {
        append(txt);
        return;
    }

    if (mPos > 0)
    {
        string left = txt.substr(0, mPos);
        string right = txt.substr(mPos);

        mText = left + txt + right;
        mPos += txt.length();
    }
    else
    {
        mText = txt + mText;
        mPos += txt.length();
    }

    QLabel::setText(QString(mText.c_str()));
}
