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
#ifndef TQTWAIT_H
#define TQTWAIT_H

#include <QDialog>

class QTimer;

namespace Ui
{
    class TQtWait;
}

class TQtWait : public QDialog
{
    Q_OBJECT

    public:
        TQtWait(QWidget *parent = nullptr);
        TQtWait(QWidget *parent = nullptr, const std::string& text = "");
        ~TQtWait();

        void setScaleFactor(double sf) { mScaleFactor = sf; }
        void doResize();
        void setText(const std::string& text);
        void start();
        void end();

    private slots:
        void onTimerEvent();

    private:
        int scale(int value);
        void startTimer();
        template <typename T>
        void scaleObject(T *obj);

        Ui::TQtWait *ui{nullptr};

        double mScaleFactor{0.0};
        std::string mText;
        int mPosition{0};
        bool mDir{false};
        QTimer *mTimer{nullptr};
};

#endif // TQTWAIT_H
