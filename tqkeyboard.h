/*
 * Copyright (C) 2021, 2022 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef TQKEYBOARD_H
#define TQKEYBOARD_H

#include <QDialog>

#include "tsystemsound.h"

namespace Ui
{
    class TQKeyboard;

    typedef enum KEYS_t
    {
        KEY_Clear,
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_Caret,
        KEY_SQ,
        KEY_Backspace,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,
        KEY_AE,
        KEY_OE,
        KEY_UE,
        KEY_SS,
        KEY_Plus,
        KEY_Hash,
        KEY_Komma,
        KEY_Dot,
        KEY_Dash,
        KEY_Tab,
        KEY_Caps,
        KEY_Shift,
        KEY_GtLt,
        KEY_AltGR,
        KEY_Blank
    }KEYS_t;
}

class TQKeyboard : public QDialog, TSystemSound
{
    Q_OBJECT

    public:
        explicit TQKeyboard(const std::string& init, const std::string& prompt, QWidget *parent = nullptr, bool priv=false);
        ~TQKeyboard();

        void doResize();
        void setPrivate(bool mode) { mPrivate = mode; }
        void setScaleFactor(double sf) { mScaleFactor = sf; }
        void setMaxLength(int len) { mMaxLen = len; }
        std::string& getText() { return mText; }
        void setString(const std::string& str);

    private slots:
        void keyA() { setKey(Ui::KEY_A); }
        void keyB() { setKey(Ui::KEY_B); }
        void keyC() { setKey(Ui::KEY_C); }
        void keyD() { setKey(Ui::KEY_D); }
        void keyE() { setKey(Ui::KEY_E); }
        void keyF() { setKey(Ui::KEY_F); }
        void keyG() { setKey(Ui::KEY_G); }
        void keyH() { setKey(Ui::KEY_H); }
        void keyI() { setKey(Ui::KEY_I); }
        void keyJ() { setKey(Ui::KEY_J); }
        void keyK() { setKey(Ui::KEY_K); }
        void keyL() { setKey(Ui::KEY_L); }
        void keyM() { setKey(Ui::KEY_M); }
        void keyN() { setKey(Ui::KEY_N); }
        void keyO() { setKey(Ui::KEY_O); }
        void keyP() { setKey(Ui::KEY_P); }
        void keyQ() { setKey(Ui::KEY_Q); }
        void keyR() { setKey(Ui::KEY_R); }
        void keyS() { setKey(Ui::KEY_S); }
        void keyT() { setKey(Ui::KEY_T); }
        void keyU() { setKey(Ui::KEY_U); }
        void keyV() { setKey(Ui::KEY_V); }
        void keyW() { setKey(Ui::KEY_W); }
        void keyX() { setKey(Ui::KEY_X); }
        void keyY() { setKey(Ui::KEY_Y); }
        void keyZ() { setKey(Ui::KEY_Z); }
        void keyAE() { setKey(Ui::KEY_AE); }
        void keyOE() { setKey(Ui::KEY_OE); }
        void keyUE() { setKey(Ui::KEY_UE); }
        void keySS() { setKey(Ui::KEY_SS); }
        void keyCaret() { setKey(Ui::KEY_Caret); }
        void keyPlus() { setKey(Ui::KEY_Plus); }
        void keyHash() { setKey(Ui::KEY_Hash); }
        void keyDash() { setKey(Ui::KEY_Dash); }
        void keyDot() { setKey(Ui::KEY_Dot); }
        void keyKomma() { setKey(Ui::KEY_Komma); }
        void keyGtLt() { setKey(Ui::KEY_GtLt); }
        void keySQ() { setKey(Ui::KEY_SQ); }
        void keyClear() { setKey(Ui::KEY_Clear); }
        void keyBackspace() { setKey(Ui::KEY_Backspace); }
        void keyTab() { setKey(Ui::KEY_Tab); }
        void keyCaps() { setKey(Ui::KEY_Caps); }
        void keyShift() { setKey(Ui::KEY_Shift); }
        void keyAltGr() { setKey(Ui::KEY_AltGR); }
        void keyBlank() { setKey(Ui::KEY_Blank); }
        void key1() { setKey(Ui::KEY_1); }
        void key2() { setKey(Ui::KEY_2); }
        void key3() { setKey(Ui::KEY_3); }
        void key4() { setKey(Ui::KEY_4); }
        void key5() { setKey(Ui::KEY_5); }
        void key6() { setKey(Ui::KEY_6); }
        void key7() { setKey(Ui::KEY_7); }
        void key8() { setKey(Ui::KEY_8); }
        void key9() { setKey(Ui::KEY_9); }
        void key0() { setKey(Ui::KEY_0); }

    private:
        void setKey(Ui::KEYS_t key);
        int scale(int value);

        Ui::TQKeyboard *ui{nullptr};

        bool mShift{false};         // Key SHIFT is pressed
        bool mCaps{false};          // Key CAPSLOCK is pressed
        bool mGr{false};            // Key AltGR is pressed
        bool mPrivate{false};       // TRUE = private mode. Only * are showed.
        double mScaleFactor{0.0};   // The scale factor if we are on a mobile device
        std::string mText;          // The typed text.
        int mMaxLen{0};             // If >0 then this is the maximum number of chars
};

#endif // TQKEYBOARD_H
