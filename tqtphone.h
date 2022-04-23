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

#ifndef __TQTPHONE_H__
#define __TQTPHONE_H__

#include <QDialog>

namespace Ui
{
    class TQtPhone;
}

class TQtPhone : public QDialog
{
    Q_OBJECT

    public:
        explicit TQtPhone(QWidget *parent = nullptr);
        ~TQtPhone();

        void setScaleFactor(double sf) { mScaleFactor = sf; }
        void doResize();

    private slots:
        void on_pushButton_0_clicked();
        void on_pushButton_1_clicked();
        void on_pushButton_2_clicked();
        void on_pushButton_3_clicked();
        void on_pushButton_4_clicked();
        void on_pushButton_5_clicked();
        void on_pushButton_6_clicked();
        void on_pushButton_7_clicked();
        void on_pushButton_8_clicked();
        void on_pushButton_9_clicked();
        void on_pushButton_Star_clicked();
        void on_pushButton_Hash_clicked();
        void on_pushButton_Clear_clicked();
        void on_pushButton_Exit_clicked();
        void on_toolButton_Call_Clicked();

    private:
        int scale(int value);
        template <typename T>
        void scaleObject(T *obj);

        Ui::TQtPhone *ui{nullptr};
        double mScaleFactor{1.0};
        QString mNumber;
};

#endif
