/*
 * Copyright (C) 2022, 2023 by Andreas Theofilu <andreas@theosys.at>
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

#include <QPainter>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>

#include "tqtextedit.h"
#include "terror.h"

using std::string;

TQTextEdit::TQTextEdit(QWidget* parent)
    : QWidget(parent)
{
    DECL_TRACER("TQTextEdit::TQTextEdit(QWidget* parent)");

    init();
}

TQTextEdit::TQTextEdit(const QString& text, QWidget* parent)
    : QWidget(parent),
      mText(text)
{
    DECL_TRACER("TQTextEdit::TQTextEdit(const QString& text, QWidget* parent)");
}

void TQTextEdit::init()
{
    DECL_TRACER("TQTextEdit::init()");

    setCursor(Qt::IBeamCursor);
    setAutoFillBackground(true);
    update();
//    setTextInteractionFlags(Qt::TextEditorInteraction);
//    setTextFormat(Qt::PlainText);
}

void TQTextEdit::setText(const QString& text)
{
    DECL_TRACER("TQTextEdit::setText(const std::string& text)");

    if (mText != text)
    {
        mText = text;
        update();
    }
}

bool TQTextEdit::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *sKey = static_cast<QKeyEvent*>(event);

        if (sKey)
        {
            switch(sKey->key())
            {
                case Qt::Key_Enter:
                case Qt::Key_Return:
                    emit contentChanged(mText);
                break;

                case Qt::Key_0: insert("0"); break;
                case Qt::Key_1: insert("1"); break;
                case Qt::Key_2: insert("2"); break;
                case Qt::Key_3: insert("3"); break;
                case Qt::Key_4: insert("4"); break;
                case Qt::Key_5: insert("5"); break;
                case Qt::Key_6: insert("6"); break;
                case Qt::Key_7: insert("7"); break;
                case Qt::Key_8: insert("8"); break;
                case Qt::Key_9: insert("9"); break;
                case Qt::Key_A: insert(mShift || mCapsLock ? "A" : "a"); break;
                case Qt::Key_B: insert(mShift || mCapsLock ? "B" : "b"); break;
                case Qt::Key_C: insert(mShift || mCapsLock ? "C" : "c"); break;
                case Qt::Key_D: insert(mShift || mCapsLock ? "D" : "d"); break;
                case Qt::Key_E: insert(mShift || mCapsLock ? "E" : "e"); break;
                case Qt::Key_F: insert(mShift || mCapsLock ? "F" : "f"); break;
                case Qt::Key_G: insert(mShift || mCapsLock ? "G" : "g"); break;
                case Qt::Key_H: insert(mShift || mCapsLock ? "H" : "h"); break;
                case Qt::Key_I: insert(mShift || mCapsLock ? "I" : "i"); break;
                case Qt::Key_J: insert(mShift || mCapsLock ? "J" : "j"); break;
                case Qt::Key_K: insert(mShift || mCapsLock ? "K" : "k"); break;
                case Qt::Key_L: insert(mShift || mCapsLock ? "L" : "l"); break;
                case Qt::Key_M: insert(mShift || mCapsLock ? "M" : "m"); break;
                case Qt::Key_N: insert(mShift || mCapsLock ? "N" : "n"); break;
                case Qt::Key_O: insert(mShift || mCapsLock ? "O" : "o"); break;
                case Qt::Key_P: insert(mShift || mCapsLock ? "P" : "p"); break;
                case Qt::Key_Q: insert(mShift || mCapsLock ? "Q" : "q"); break;
                case Qt::Key_R: insert(mShift || mCapsLock ? "R" : "r"); break;
                case Qt::Key_S: insert(mShift || mCapsLock ? "S" : "s"); break;
                case Qt::Key_T: insert(mShift || mCapsLock ? "T" : "t"); break;
                case Qt::Key_U: insert(mShift || mCapsLock ? "U" : "u"); break;
                case Qt::Key_V: insert(mShift || mCapsLock ? "V" : "v"); break;
                case Qt::Key_W: insert(mShift || mCapsLock ? "W" : "w"); break;
                case Qt::Key_X: insert(mShift || mCapsLock ? "X" : "x"); break;
                case Qt::Key_Y: insert(mShift || mCapsLock ? "Y" : "y"); break;
                case Qt::Key_Z: insert(mShift || mCapsLock ? "Z" : "z"); break;

                case Qt::Key_Exclam:        insert("!"); break;
                case Qt::Key_Space:         insert(" "); break;
                case Qt::Key_Ampersand:     insert("&"); break;
                case Qt::Key_Slash:         insert("/"); break;
                case Qt::Key_Backslash:     insert("\\"); break;
                case Qt::Key_QuoteDbl:      insert("\""); break;
                case Qt::Key_Percent:       insert("%"); break;
                case Qt::Key_Dollar:        insert("$"); break;
                case Qt::Key_paragraph:     insert("§"); break;
                case Qt::Key_BraceLeft:     insert("{"); break;
                case Qt::Key_BraceRight:    insert("}"); break;
                case Qt::Key_BracketLeft:   insert("["); break;
                case Qt::Key_BracketRight:  insert("]"); break;
                case Qt::Key_ParenLeft:     insert("("); break;
                case Qt::Key_ParenRight:    insert(")"); break;
                case Qt::Key_Equal:         insert("="); break;
                case Qt::Key_Question:      insert("?"); break;
                case Qt::Key_degree:        insert("°"); break;
                case Qt::Key_Colon:         insert(":"); break;
                case Qt::Key_Comma:         insert(","); break;
                case Qt::Key_Underscore:    insert("_"); break;
                case Qt::Key_hyphen:        insert("-"); break;
                case Qt::Key_Semicolon:     insert(";"); break;
                case Qt::Key_Bar:           insert("|"); break;
                case Qt::Key_Greater:       insert(">"); break;
                case Qt::Key_Less:          insert("<"); break;
                case Qt::Key_AsciiCircum:   insert("^"); break;
                case Qt::Key_Plus:          insert("+"); break;
                case Qt::Key_Period:        insert("."); break;
                case Qt::Key_QuoteLeft:     insert("`"); break;
                case Qt::Key_Apostrophe:    insert("'"); break;
                case Qt::Key_Asterisk:      insert("*"); break;
                case Qt::Key_AsciiTilde:    insert("~"); break;
                case Qt::Key_At:            insert("@"); break;
                case Qt::Key_mu:            insert("µ"); break;

                default:
                    return false;
            }

            return true;
        }
    }

    return false;
}

void TQTextEdit::append(const QString& txt)
{
    DECL_TRACER("TQTextEdit::append(const QString& txt)");

    mText += txt;
//    QLabel::setText(mText);
    mPos = mText.length();
    update();
}

void TQTextEdit::insert(const QString& txt, int pos)
{
    DECL_TRACER("TQTextEdit::insert(const QString& txt, int pos)");

    if (mText.isEmpty())
    {
        mText = txt;
//        QLabel::setText(mText);
        mPos = mText.length();
        update();
        return;
    }

    if (pos >= 0)
    {
        if ((qsizetype)pos > mText.length())
            mPos = mText.length();
        else
            mPos = pos;
    }

    if ((qsizetype)mPos == mText.length())
    {
        append(txt);
        return;
    }

    if (mPos > 0)
    {
        QString left = txt.mid(0, mPos);
        QString right = txt.mid(mPos);

        mText = left + txt + right;
        mPos += txt.length();
    }
    else
    {
        mText = txt + mText;
        mPos += txt.length();
    }

    update();
//    QLabel::setText(mText);
}

void TQTextEdit::setAlignment(Qt::Alignment al)
{
    DECL_TRACER("TQTextEdit::setAlignment(Qt::Alignment al)");

    mAlignment = al;
    updateCoordinates();
}

void TQTextEdit::setPadding(int left, int top, int right, int bottom)
{
    DECL_TRACER("TQTextEdit::setPadding(int left, int top, int right, int bottom)");

    mPadLeft = (left >= 0 ? left : 0);
    mPadTop = (top >= 0 ? left : 0);
    mPadRight = (right >= 0 ? left : 0);
    mPadBottom = (bottom >= 0 ? left : 0);
    updateCoordinates();
}

void TQTextEdit::updateCoordinates()
{
    DECL_TRACER("TQTextEdit::updateCoordinates()");

    mFontPointSize = font().pointSize() / 2;
    mTextLength = fontMetrics().horizontalAdvance(mText);
    mTextHeight = fontMetrics().height();

    if (mAlignment & Qt::AlignLeft)
        mPosX = mPadLeft;
    else if (mAlignment & Qt::AlignHCenter)
        mPosX = width() / 2 - mTextLength / 2;
    else if (mAlignment & Qt::AlignRight)
        mPosX = width() - mPadRight - mTextLength;

    if (mAlignment & Qt::AlignTop)
        mPosY = mPadTop + mTextHeight / 2;
    else if (mAlignment & Qt::AlignVCenter)
        mPosY = height() / 2;
    else if (mAlignment & Qt::AlignBottom)
        mPosY = height() - mPadBottom - mTextHeight / 2;

    MSG_DEBUG("Font point size: " << mFontPointSize << ", text length: " << mTextLength << ", text height: " << mTextHeight);
    MSG_DEBUG("Pos X: " << mPosX << ", Pos Y: " << mPosY);
//    QApplication::processEvents();
    update();
}

void TQTextEdit::setBackgroundPixmap(const QPixmap& pixmap)
{
    DECL_TRACER("TQTextEdit::setBackgroundPixmap(const QPixmap& pixmap)");

    if (pixmap.isNull())
        return;

    mBackground = pixmap;
//    setPixmap(mBackground);
//    QApplication::processEvents();
    update();
}

void TQTextEdit::resizeEvent(QResizeEvent *evt)
{
    DECL_TRACER("TQTextEdit::resizeEvent(QResizeEvent *evt)");

    updateCoordinates();
    QWidget::resizeEvent(evt);
}

void TQTextEdit::paintEvent(QPaintEvent*)
{
    DECL_TRACER("TQTextEdit::paintEvent(QPaintEvent*)");

    QPainter p(this);

    if (!mBackground.isNull())
        p.drawPixmap(0, 0, mBackground);

    p.drawText(mPosX, mPosY + mFontPointSize, mText);
}
