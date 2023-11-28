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

#ifndef __TQTINPUTLINE_H__
#define __TQTINPUTLINE_H__

#include <QDialog>

namespace Ui
{
    class TQtInputLine;
}

class TQtInputLine : public QDialog
{
    Q_OBJECT

    public:
        TQtInputLine(QWidget *parent=nullptr);
        ~TQtInputLine();

        void setText(const std::string& text);
        std::string& getText() { return mText; }
        void setPassword(bool pw);
        void setMessage(const std::string& msg);
        void setScaleFactor(double sf) { mScaleFactor = sf; }
        void doResize();

    public slots:
        void on_lineEdit_textChanged(const QString &arg1);

    private:
        int scale(int value);
        template <typename T>
        void scaleObject(T *obj);

        Ui::TQtInputLine *ui{nullptr};
        double mScaleFactor{1.0};
        std::string mText;
};

#endif
