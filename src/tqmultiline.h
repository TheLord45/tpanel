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
#ifndef TQMULTILINE_H
#define TQMULTILINE_H

#include <QTextEdit>

class TQMultiLine : public QTextEdit
{
    Q_OBJECT

    public:
        TQMultiLine(QWidget *parent=nullptr);
        TQMultiLine(QWidget *parent, const std::string& text);

    signals:
        void focusChanged(bool in);
        void keyPressed(int key);

    protected:
        void keyPressEvent(QKeyEvent *e) override;
        void focusInEvent(QFocusEvent *e) override;
        void focusOutEvent(QFocusEvent *e) override;
};

#endif // TQMULTILINE_H
