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

#ifndef __TQTBUSY_H__
#define __TQTBUSY_H__

#include <QDialog>

#include <string>

namespace Ui
{
    class TQBusy;

    class TQtBusy : public QDialog
    {
        Q_OBJECT

        public:
            explicit TQtBusy(const std::string& msg, QWidget *parent = nullptr);
            ~TQtBusy();

            void doResize();
            void setScaleFactor(double sf) { mScaleFactor = sf; }

        private:
            Ui::TQBusy *ui{nullptr};

            int scale(int value);

            double mScaleFactor{0.0};
    };
}

#endif
