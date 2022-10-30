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

#ifndef __TQTEXTEDIT_H__
#define __TQTEXTEDIT_H__

#include <string>
#include <QWidget>
#include <QLabel>

//QT_BEGIN_NAMESPACE
//class QLabel;
//QT_END_NAMESPACE

class TQTextEdit : public QLabel
{
    public:
        TQTextEdit();
        TQTextEdit(QWidget *parent);

        void setText(const std::string& text);
        std::string& getText() { return mText; }

        virtual void sigContentChanged(const std::string& text) = 0;

    protected:
        bool event(QEvent *event) override;

    private:
        void init();
        void append(const std::string& txt);
        void insert(const std::string& txt, int pos=-1);

        std::string mText;
        int mPos{0};            // The current cursor position
};

#endif
