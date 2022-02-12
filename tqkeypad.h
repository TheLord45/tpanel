/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef TQKEYPAD_H
#define TQKEYPAD_H

#include <QDialog>

#include "tsystemsound.h"

namespace Ui
{
    class TQKeypad;

    typedef enum KEYSP_t
    {
        KEYP_0,
        KEYP_1,
        KEYP_2,
        KEYP_3,
        KEYP_4,
        KEYP_5,
        KEYP_6,
        KEYP_7,
        KEYP_8,
        KEYP_9,
        KEYP_Minus,
        KEYP_Plus,
        KEYP_Clear,
        KEYP_Dot,
        KEYP_Komma,
        KEYP_DoubleDot
    }KEYSP_t;
}

class TQKeypad : public QDialog, TSystemSound
{
    Q_OBJECT

    public:
        explicit TQKeypad(const std::string& init, const std::string& prompt, QWidget *parent = nullptr, bool priv=false);
        ~TQKeypad();

        void doResize();
        void setPrivate(bool mode) { mPrivate = mode; }
        void setScaleFactor(double sf) { mScaleFactor = sf; }
        std::string& getText() { return mText; }

    private slots:
        void key0() { setKey(Ui::KEYP_0); }
        void key1() { setKey(Ui::KEYP_1); }
        void key2() { setKey(Ui::KEYP_2); }
        void key3() { setKey(Ui::KEYP_3); }
        void key4() { setKey(Ui::KEYP_4); }
        void key5() { setKey(Ui::KEYP_5); }
        void key6() { setKey(Ui::KEYP_6); }
        void key7() { setKey(Ui::KEYP_7); }
        void key8() { setKey(Ui::KEYP_8); }
        void key9() { setKey(Ui::KEYP_9); }
        void keyMinus() { setKey(Ui::KEYP_Minus); }
        void keyPlus() { setKey(Ui::KEYP_Plus); }
        void keyDot() { setKey(Ui::KEYP_Dot); }
        void keyKomma() { setKey(Ui::KEYP_Komma); }
        void keyDoubleDot() { setKey(Ui::KEYP_DoubleDot); }
        void keyClear() { setKey(Ui::KEYP_Clear); }

    private:
        void setKey(Ui::KEYSP_t key);
        int scale(int value);

        Ui::TQKeypad *ui{nullptr};

        bool mPrivate{false};       // TRUE = private mode. Only * is showed.
        double mScaleFactor{0.0};   // The scale factor if we are on a mobile device
        std::string mText;          // The typed text.
        std::string mSound;         // Path and name to a sound file played on every key press.
};

#endif // TQKEYPAD_H
