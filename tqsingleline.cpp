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

#include <QWidget>
#include <QKeyEvent>

#include "tqsingleline.h"
#include "terror.h"

using std::string;

TQSingleLine::TQSingleLine(QWidget *parent)
    : QLineEdit(parent)
{
    DECL_TRACER("TQSingleLine::TQSingleLine(QWidget *parent)");
}

TQSingleLine::TQSingleLine(QWidget *parent, const string& text)
    : QLineEdit(parent)
{
    DECL_TRACER("TQSingleLine::TQSingleLine(QWidget *parent, const string& text)");

    setText(text.c_str());
}

/*******************************************************************************
 * Signal handling
 ******************************************************************************/

void TQSingleLine::keyPressEvent(QKeyEvent *e)
{
    DECL_TRACER("TQSingleLine::keyPressEvent(QKeyEvent *e)");

    QLineEdit::keyPressEvent(e);
    emit keyPressed(e->key());
}

void TQSingleLine::focusInEvent(QFocusEvent *e)
{
    DECL_TRACER("TQSingleLine::focusInEvent(QFocusEvent *e)");

    QLineEdit::focusInEvent(e);
    emit focusChanged(true);
}

void TQSingleLine::focusOutEvent(QFocusEvent *e)
{
    DECL_TRACER("TQSingleLine::focusOutEvent(QFocusEvent *e)");

    QLineEdit::focusOutEvent(e);
    emit focusChanged(false);
}
