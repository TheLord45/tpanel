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

#ifndef __TSYSTEMBUTTON_H__
#define __TSYSTEMBUTTON_H__

#include <string>
#include <vector>
#include "tbutton.h"

#define BANK_1      1       // Keyboard keys bank1: lower case letters
#define BANK_2      2       // Keyboard keys bank2: upper case letters (shift pressed)
#define BANK_3      3       // Keyboard keys bank3: Special characters (AltGr pressed)

class TSystemButton
{
    public:
        typedef enum SYS_BUUTON_TYPE
        {
            KEY_UNDEFINED,
            KEY_KEY,            // A keyboard button e.g. printable symbol
            KEY_INPUT_SINGLE,   // Single input line
            KEY_INPUT_MULTI,    // Multi line input object
            KEY_BUTTON          // A normal system button (increase/decrease volume, ...)
        }SYS_BUTTON_TYPE;

        typedef struct SYS_BUTTONS
        {
            int channel{0};         // The channel number (cp)
            int port{0};            // The port number (ad)
            SYS_BUTTON_TYPE type;   // The type of the button
        }SYS_BUTTONS;

        TSystemButton();
        ~TSystemButton();

        void addSysButton(Button::TButton *bt);
        Button::TButton *getSysButton(int channel, int port);
        void setBank(int bank);     // The bank (instance) of keyboard buttons
        int getActualBank() { return mBank; }
    
    protected:
        SYS_BUTTON_TYPE getSystemButtonType(int channel, int port);
        Button::TButton *getSystemInputLine();
        Button::TButton *getSystemKey(int channel, uint handle=0);
        void buttonPress(int channel, uint handle, bool pressed);      // Callback for buttons

    private:
        void setKeysToBank(int bank, uint handle);   // The handle is the key who should be highlighted
        void handleDedicatedKeys(int channel, bool pressed);
        void sendDedicatedKey(int channel, const std::string str, bool pressed);
        std::string typeToString(SYS_BUUTON_TYPE type);

        int mBank{1};                       // The activated bank (1 - 3)
        int mStateKeyActive{0};             // If the system button is a key button, it can have a state between 0 and 5
        std::string mInputText;             // Contains the text typed over system keyboard
        std::vector<Button::TButton *> mButtons;    // Vector array holding all system buttons
        bool mCapsLock{false};              // TRUE = Key CAPS LOCK was pressed and is active
        bool mBank3{false};                 // TRUE = 3rd bank was activated for one key press
        bool mShift{false};                 // TRUE = Key shift was pressed for one key press
        bool isKeyboard{false};             // TRUE = A keyboard was detected, FALSE = A keypad was detected
};

#endif
