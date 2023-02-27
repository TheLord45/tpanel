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

#include <functional>

#include "tsystembutton.h"
#include "tpagemanager.h"
#include "tbutton.h"
#include "terror.h"
#include "tresources.h"

#define MAX_BANK        3

#define DISPLAY_LINE    0

// Key definitions
#define KB_DISPLAY_INPUT    5
#define KB_DISPLAY_INFO     6

#define KB_KEYBOARD_KEY     201

#define KB_SPACE            202
#define KB_ENTER            203
#define KB_BACKSPACE        208
#define KB_CLEAR            210
#define KB_CANCEL           211
#define KB_SUBMIT           212
#define KB_CAPS_LOCK        221
#define KB_BANK3            222
#define KB_SHIFT            226

using std::string;
using std::vector;
using std::bind;
using namespace Button;

TSystemButton::SYS_BUTTONS _sysButtons[] = {
    {   0, 4, TSystemButton::KEY_INPUT_SINGLE },   // A single line textarea getting typed keys for keypad
    {   0, 5, TSystemButton::KEY_INPUT_MULTI },    // A multiline textarea getting typed keys for keyboard
    {   0, 6, TSystemButton::KEY_BUTTON },         // A button showing the prompt text comming from the call to the keyboard
    { 201, 0, TSystemButton::KEY_KEY },            // A keyboard key
    { 202, 0, TSystemButton::KEY_BUTTON },         // Space button
    { 203, 0, TSystemButton::KEY_BUTTON },         // Enter/Return button
    { 208, 0, TSystemButton::KEY_BUTTON },         // Backspace button
    { 210, 0, TSystemButton::KEY_BUTTON },         // Clear button
    { 211, 0, TSystemButton::KEY_BUTTON },         // The keyboard cancel button
    { 212, 0, TSystemButton::KEY_BUTTON },         // The keyboard submit button
    { 221, 0, TSystemButton::KEY_BUTTON },         // bank 2; Caps lock button
    { 222, 0, TSystemButton::KEY_BUTTON },         // Bank 3; lock button
    { 226, 0, TSystemButton::KEY_BUTTON },         // Bank 2; shift button
    // Virtual keyboard keys
    { 501, 0, TSystemButton::KEY_BUTTON },         // ESC
    { 502, 0, TSystemButton::KEY_BUTTON },         // !\n1
    { 503, 0, TSystemButton::KEY_BUTTON },         // @\n2
    { 504, 0, TSystemButton::KEY_BUTTON },         // #\n3
    { 505, 0, TSystemButton::KEY_BUTTON },         // $\n4
    { 506, 0, TSystemButton::KEY_BUTTON },         // %\n5
    { 507, 0, TSystemButton::KEY_BUTTON },         // ^\n6
    { 508, 0, TSystemButton::KEY_BUTTON },         // &\n7
    { 509, 0, TSystemButton::KEY_BUTTON },         // *\n8
    { 510, 0, TSystemButton::KEY_BUTTON },         // (\n9
    { 511, 0, TSystemButton::KEY_BUTTON },         // )\n0
    { 512, 0, TSystemButton::KEY_BUTTON },         // _\n-
    { 513, 0, TSystemButton::KEY_BUTTON },         // +\n=
    { 514, 0, TSystemButton::KEY_BUTTON },         // Backspace
    { 515, 0, TSystemButton::KEY_BUTTON },         // TAB
    { 516, 0, TSystemButton::KEY_BUTTON },         // Q
    { 517, 0, TSystemButton::KEY_BUTTON },         // W
    { 518, 0, TSystemButton::KEY_BUTTON },         // E
    { 519, 0, TSystemButton::KEY_BUTTON },         // R
    { 520, 0, TSystemButton::KEY_BUTTON },         // T
    { 521, 0, TSystemButton::KEY_BUTTON },         // Y
    { 522, 0, TSystemButton::KEY_BUTTON },         // U
    { 523, 0, TSystemButton::KEY_BUTTON },         // I
    { 524, 0, TSystemButton::KEY_BUTTON },         // O
    { 525, 0, TSystemButton::KEY_BUTTON },         // P
    { 526, 0, TSystemButton::KEY_BUTTON },         // {\n[
    { 527, 0, TSystemButton::KEY_BUTTON },         // }\n]
    { 528, 0, TSystemButton::KEY_BUTTON },         // Enter
    { 529, 0, TSystemButton::KEY_BUTTON },         // Ctrl left
    { 530, 0, TSystemButton::KEY_BUTTON },         // A
    { 531, 0, TSystemButton::KEY_BUTTON },         // S
    { 532, 0, TSystemButton::KEY_BUTTON },         // D
    { 533, 0, TSystemButton::KEY_BUTTON },         // F
    { 534, 0, TSystemButton::KEY_BUTTON },         // G
    { 535, 0, TSystemButton::KEY_BUTTON },         // H
    { 536, 0, TSystemButton::KEY_BUTTON },         // J
    { 537, 0, TSystemButton::KEY_BUTTON },         // K
    { 538, 0, TSystemButton::KEY_BUTTON },         // L
    { 539, 0, TSystemButton::KEY_BUTTON },         // :\n;
    { 540, 0, TSystemButton::KEY_BUTTON },         // "\n'
    { 541, 0, TSystemButton::KEY_BUTTON },         // ~\n`
    { 542, 0, TSystemButton::KEY_BUTTON },         // Shift left
    { 543, 0, TSystemButton::KEY_BUTTON },         // |
    { 544, 0, TSystemButton::KEY_BUTTON },         // Z
    { 545, 0, TSystemButton::KEY_BUTTON },         // X
    { 546, 0, TSystemButton::KEY_BUTTON },         // C
    { 547, 0, TSystemButton::KEY_BUTTON },         // V
    { 548, 0, TSystemButton::KEY_BUTTON },         // B
    { 549, 0, TSystemButton::KEY_BUTTON },         // N
    { 550, 0, TSystemButton::KEY_BUTTON },         // M
    { 551, 0, TSystemButton::KEY_BUTTON },         // <\n,
    { 552, 0, TSystemButton::KEY_BUTTON },         // >\n.
    { 553, 0, TSystemButton::KEY_BUTTON },         // ?\n/
    { 554, 0, TSystemButton::KEY_BUTTON },         // Shift right
    { 556, 0, TSystemButton::KEY_BUTTON },         // Alt left
    { 557, 0, TSystemButton::KEY_BUTTON },         // Space
    { 558, 0, TSystemButton::KEY_BUTTON },         // Caps lock
    { 559, 0, TSystemButton::KEY_BUTTON },         // F1
    { 560, 0, TSystemButton::KEY_BUTTON },         // F2
    { 561, 0, TSystemButton::KEY_BUTTON },         // F3
    { 562, 0, TSystemButton::KEY_BUTTON },         // F4
    { 563, 0, TSystemButton::KEY_BUTTON },         // F5
    { 564, 0, TSystemButton::KEY_BUTTON },         // F6
    { 565, 0, TSystemButton::KEY_BUTTON },         // F7
    { 566, 0, TSystemButton::KEY_BUTTON },         // F8
    { 567, 0, TSystemButton::KEY_BUTTON },         // F9
    { 568, 0, TSystemButton::KEY_BUTTON },         // F10
    { 587, 0, TSystemButton::KEY_BUTTON },         // F11
    { 588, 0, TSystemButton::KEY_BUTTON },         // F12

    { 597, 0, TSystemButton::KEY_BUTTON },         // Ctrl right

    { 600, 0, TSystemButton::KEY_BUTTON },         // Alt right (AltGr)

    { 602, 0, TSystemButton::KEY_BUTTON },         // Home
    { 603, 0, TSystemButton::KEY_BUTTON },         // Arrow up
    { 604, 0, TSystemButton::KEY_BUTTON },         // PgUp
    { 605, 0, TSystemButton::KEY_BUTTON },         // Arrow left
    { 606, 0, TSystemButton::KEY_BUTTON },         // Arrow right
    { 607, 0, TSystemButton::KEY_BUTTON },         // End
    { 608, 0, TSystemButton::KEY_BUTTON },         // Arrow down
    { 609, 0, TSystemButton::KEY_BUTTON },         // PgDn
    { 610, 0, TSystemButton::KEY_BUTTON },         // Insert
    { 611, 0, TSystemButton::KEY_BUTTON },         // Delete
    {   0, 0, TSystemButton::KEY_UNDEFINED}        // End of table marker
};

TSystemButton::TSystemButton()
{
    DECL_TRACER("TSystemButton::TSystemButton()");
}

TSystemButton::~TSystemButton()
{
    DECL_TRACER("TSystemButton::~TSystemButton()");

    setBank(1);
    mButtons.clear();
}

void TSystemButton::addSysButton(TButton *bt)
{
    DECL_TRACER("TSystemButton::addButton(TButton *bt)");

    if (!bt)
    {
        MSG_WARNING("No valid button pointer!");
        return;
    }

    if (!(bt->getAddressPort() == 0 && bt->getAddressChannel() > 0) &&
        !(bt->getChannelPort() == 0 && bt->getChannelNumber() > 0))
    {
        MSG_DEBUG("No system keyboard button: channel number=" << bt->getChannelNumber() << ", channel port=" << bt->getChannelPort());
        return;
    }

    // First we look if the button is already there
    if (!mButtons.empty())
    {
        vector<TButton *>::iterator iter;
        int channel = bt->getChannelNumber();
        int addrChannel = bt->getAddressChannel();

        for (iter = mButtons.begin(); iter != mButtons.end(); ++iter)
        {
            TButton *button = *iter;

            if (channel == button->getChannelNumber() &&
                addrChannel == button->getAddressChannel() &&
                bt->getName() == button->getName())
            {
                MSG_WARNING("Don't add the keyboard button " << button->getName() << " again!");
                return;
            }
        }
    }

    // Is the button a recognized system button
    int i = 0;

    while(_sysButtons[i].type != KEY_UNDEFINED)
    {
        if (_sysButtons[i].channel == bt->getChannelNumber() &&
            _sysButtons[i].port == bt->getAddressChannel())
        {
            if (!isKeyboard && bt->getChannelNumber() == KB_KEYBOARD_KEY)
            {
                char ch = bt->getText(STATE_1)[0];

                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
                    isKeyboard = true;
            }

            bt->regCallButtonPress(bind(&TSystemButton::buttonPress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            mButtons.push_back(bt);
            MSG_DEBUG("Button " << bt->getName() << " was added to system keyboard buttons list.");
            return;
        }

        i++;
    }

    MSG_DEBUG("Button " << bt->getName() << " is not a supported system keyboard button!");
}

TButton *TSystemButton::getSysButton(int channel, int port)
{
    DECL_TRACER("TSystemButton::getButton(int channel, int port)");

    if (channel == 0 && port == 0)
        return nullptr;

    if (mButtons.empty())
        return nullptr;

    vector<TButton *>::iterator iter;

    for (iter = mButtons.begin(); iter != mButtons.end(); ++iter)
    {
        TButton *button = *iter;

        if (button->getChannelNumber() == channel && button->getAddressChannel() == port)
            return *iter;
    }

    return nullptr;
}

void TSystemButton::setBank(int bank)
{
    DECL_TRACER("TSystemButton::setBank(int bank)");

    if (bank < BANK_1 || bank > BANK_3)
    {
        MSG_WARNING("Illegal bank " << bank << "! Ignoring it.");
        return;
    }

    if (mButtons.empty())
        return;

    mStateKeyActive = (bank - 1) * 2;
    vector<TButton *>::iterator iter;

    for (iter = mButtons.begin(); iter != mButtons.end(); ++iter)
    {
        TButton *button = *iter;

        if (button->getActiveInstance() == mStateKeyActive)
            continue;

        if (getSystemButtonType(button->getChannelNumber(), button->getAddressChannel()) == KEY_KEY && button->getNumberInstances() <= (MAX_BANK * 2))
            button->setActive(mStateKeyActive);
    }
}

TSystemButton::SYS_BUTTON_TYPE TSystemButton::getSystemButtonType(int channel, int port)
{
    DECL_TRACER("TSystemButton::getSystemButtonType(int channel, int port)");

    int i = 0;

    while(_sysButtons[i].type != KEY_UNDEFINED)
    {
        if (_sysButtons[i].channel == channel && _sysButtons[i].port == port)
            return _sysButtons[i].type;

        i++;
    }

    return KEY_UNDEFINED;
}

TButton *TSystemButton::getSystemInputLine()
{
    DECL_TRACER("TSystemButton::getSystemInputLine()");

    vector<TButton *>::iterator iter;

    for (iter = mButtons.begin(); iter != mButtons.end(); ++iter)
    {
        TButton *button = *iter;

        if (button->getAddressChannel() == KB_DISPLAY_INPUT)
        {
            MSG_DEBUG("Found input line " << button->getName() << ".");
            return *iter;
        }
    }

    return nullptr;
}

TButton *TSystemButton::getSystemKey(int channel, uint handle)
{
    DECL_TRACER("TSystemButton::getSystemKey(int channel)");

    uint intHandle = handle;

    vector<TButton *>::iterator iter;

    for (iter = mButtons.begin(); iter != mButtons.end(); ++iter)
    {
        TButton *button = *iter;

        if (handle == 0)
            intHandle = button->getHandle();

        if (button->getChannelNumber() == channel && button->getHandle() == intHandle)
        {
            MSG_DEBUG("Found system key " << button->getName());
            return *iter;
        }
    }

    return nullptr;
}

void TSystemButton::buttonPress(int channel, uint handle, bool pressed)
{
    DECL_TRACER("TSystemButton::buttonPress(int channel, const std::string& text, bool pressed);");

    if (mButtons.empty())
        return;

    SYS_BUTTON_TYPE type = getSystemButtonType(channel, 0);

    if (type == KEY_UNDEFINED)
        return;

    MSG_DEBUG("Found button of type " << typeToString(type));

    if (channel > 500)
    {
        handleDedicatedKeys(channel, pressed);
        return;
    }

    TButton *btShift = nullptr;
    TButton *btBank3 = nullptr;
    TButton *btCaps = nullptr;

    // Handle the switch keys
    if (pressed)
    {
        if (type == KEY_BUTTON)
        {
            if (channel == KB_SHIFT)
            {
                mShift = !mShift;

                if (mShift)
                {
                    mCapsLock = mBank3 = false;
                    mBank = BANK_2;
                }
                else
                    mBank = BANK_1;
            }
            else if (channel == KB_CAPS_LOCK)
            {
                mCapsLock = !mCapsLock;

                if (mCapsLock)
                {
                    mShift = mBank3 = false;
                    mBank = BANK_2;
                }
                else
                    mBank = BANK_1;
            }
            else if (channel == KB_BANK3)
            {
                mBank3 = !mBank3;

                if (mBank3)
                {
                    mBank = BANK_3;
                    mShift = mCapsLock = false;
                }
                else
                    mBank = BANK_1;
            }

            mStateKeyActive = (mBank - 1) * 2 + 1;
        }
        else if (type == KEY_KEY)
        {
            mBank = (mBank3 ? BANK_3 : (mShift || mCapsLock ? BANK_2 : BANK_1));
            mStateKeyActive = (mBank - 1) * 2 + 1;
        }

        btShift = getSystemKey(KB_SHIFT);
        btBank3 = getSystemKey(KB_BANK3);
        btCaps = getSystemKey(KB_CAPS_LOCK);
    }
    else
        mStateKeyActive = (mBank - 1) * 2;

    MSG_DEBUG("Button " << handleToString(handle) << ": shift is " << (mShift ? "TRUE" : "FALSE") << ", caps lock is " << (mCapsLock ? "TRUE" : "FALSE") << ", current bank: " << mBank << " (" << (mBank3 ? "TRUE" : "FALSE") << "), pressed is " << (pressed ? "TRUE" : "FALSE") << ", instance is " << mStateKeyActive);
    // Handle all keys who must be switched as a group
    TButton *input = getSystemInputLine();
    MSG_DEBUG("Input line was " << (input ? "found" : "not found"));

    if (pressed)
    {
        if (btShift)
            btShift->setActive(mShift ? STATE_ON : STATE_OFF);

        if (btCaps)
            btCaps->setActive(mCapsLock ? STATE_ON : STATE_OFF);

        if (btBank3)
            btBank3->setActive(mBank3 ? STATE_ON : STATE_OFF);
    }

    uint hd = handle;

    if ((type == KEY_BUTTON && channel != KB_SHIFT && channel != KB_CAPS_LOCK && channel != KB_BANK3) ||
        (type == KEY_KEY && !pressed))
        hd = HANDLE_UNDEF;

    if (type == KEY_BUTTON && hd != HANDLE_UNDEF && !mShift && !mCapsLock && !mBank3)
        hd = HANDLE_UNDEF;

    if (pressed && (channel == KB_BACKSPACE || channel == KB_CANCEL ||
        channel == KB_CLEAR || channel == KB_ENTER || channel == KB_SPACE ||
        channel == KB_SUBMIT) && hd == HANDLE_UNDEF)
        hd = handle;

    setKeysToBank(mBank, hd);

    if (channel == KB_SHIFT || channel == KB_CAPS_LOCK || channel == KB_BANK3)
        return;

    TButton *button = nullptr;

    if (channel == KB_SHIFT)
        button = btShift;
    else if (channel == KB_CAPS_LOCK)
        button = btCaps;
    else if (channel == KB_BANK3)
        button = btBank3;
    else
        button = getSystemKey(channel, (type == KEY_KEY ? handle : 0));

    if (!button)
        return;

    if (pressed && type == KEY_KEY)
    {
        string letter = button->getText(mStateKeyActive);
        mInputText.append(letter);

        if (input)
            input->setText(mInputText, 0);

        if (gPageManager)
            gPageManager->sendKeyStroke(letter[0]);

        MSG_DEBUG("Actual text: " << mInputText);
        mShift = mBank3 = false;
        mBank = (mCapsLock ? BANK_2 : BANK_1);
        return;
    }

    // Handle control keys
    if (pressed)
    {
        string kbtype = (isKeyboard ? "KEYB-" : "KEYP-");

        switch(channel)
        {
            case KB_BACKSPACE:
                if (gPageManager)
                    gPageManager->sendKeyboard(kbtype +"BACKSPACE");

                mInputText = mInputText.substr(0, mInputText.length() - 2);

                if (input)
                    input->setText(mInputText, 0);

                mShift = mBank3 = false;
                mBank = (mCapsLock ? BANK_2 : BANK_1);
            break;

            case KB_CANCEL:
                if (gPageManager)
                    gPageManager->sendKeyboard(kbtype + "ABORT");

                mInputText.clear();

                if (input)
                    input->setText("", 0);

                mShift = mBank3 = mCapsLock = false;
                mBank = BANK_1;
            break;

            case KB_CLEAR:
                if (gPageManager)
                    gPageManager->sendKeyboard(kbtype + "CLEAR");

                mInputText.clear();

                if (input)
                    input->setText(mInputText, 0);

                mShift = mBank3 = false;
                mBank = (mCapsLock ? BANK_2 : BANK_1);
            break;

            case KB_ENTER:
                if (gPageManager)
                    gPageManager->sendKeyboard(kbtype + "ENTER");

                mInputText.append("\n");

                if (input)
                    input->setText(mInputText, 0);
            break;

            case KB_SPACE:
                if (gPageManager)
                    gPageManager->sendKeyboard(kbtype + "SPACE");

                mInputText.append(" ");

                if (input)
                    input->setText(mInputText, 0);

                mShift = mBank3 = false;
                mBank = (mCapsLock ? BANK_2 : BANK_1);
            break;

            case KB_SUBMIT:
                if (gPageManager)
                    gPageManager->sendKeyboard(kbtype + mInputText);

                mInputText.clear();

                if (input)
                    input->setText(mInputText, 0);

                mShift = mBank3 = mCapsLock = false;
                mBank = 1;
            break;
        }

        MSG_DEBUG("Current string: " << mInputText);
    }
}

/**
 * Set the keys of a keyboard or keypad to the given \b bank. With the \b handle
 * one key can be set to pressed state.
 * This works for multi bargraph keys with 6 states as well as normal keys
 * with only 2 states.
 *
 * @param bank      The number of the bank the multi bargraph keys should be set.
 *                  This is a number between 1 and 3.
 * @param handle    If this is >0 and a button with the given handle is found,
 *                  The state is increased by 1, so that the button appears
 *                  highlighted.
 */
void TSystemButton::setKeysToBank(int bank, uint handle)
{
    DECL_TRACER("TSystemButton::setKeysToBank(int bank, int handle)");

    if (mButtons.empty() || bank < BANK_1 || bank > BANK_3)
        return;

    vector<TButton *>::iterator iter;
    int inst = (bank - 1) * 2;

    for (iter = mButtons.begin(); iter != mButtons.end(); ++iter)
    {
        TButton *button = *iter;
        SYS_BUTTON_TYPE tp = getSystemButtonType(button->getChannelNumber(), 0);

        if (tp == KEY_KEY)
        {
            if (handle == button->getHandle())
                button->setBargraphLevel(inst + 1);
            else
                button->setBargraphLevel(inst);
        }
        else if (tp == KEY_BUTTON)
        {
            if (handle == button->getHandle() || (button->getChannelNumber() == KB_CAPS_LOCK && mCapsLock))
                button->setActive(STATE_ON);
            else
                button->setActive(STATE_OFF);
        }
    }
}

void TSystemButton::handleDedicatedKeys(int channel, bool pressed)
{
    DECL_TRACER("TSystemButton::handleDedicatedKeys(int channel, bool pressed)");

    if (!gPageManager)
        return;

    string kbtype = "KEYB-";

    if (mCapsLock)
        mShift = true;

    switch (channel)
    {
        case 501: sendDedicatedKey(channel, kbtype + "ESC", pressed); break;
        case 502: sendDedicatedKey(channel, kbtype + (mShift ? "!" : "1"), pressed); break;
        case 503: sendDedicatedKey(channel, kbtype + (mShift ? "@" : "2"), pressed); break;
        case 504: sendDedicatedKey(channel, kbtype + (mShift ? "#" : "3"), pressed); break;
        case 505: sendDedicatedKey(channel, kbtype + (mShift ? "$" : "4"), pressed); break;
        case 506: sendDedicatedKey(channel, kbtype + (mShift ? "%" : "5"), pressed); break;
        case 507: sendDedicatedKey(channel, kbtype + (mShift ? "^" : "6"), pressed); break;
        case 508: sendDedicatedKey(channel, kbtype + (mShift ? "&" : "7"), pressed); break;
        case 509: sendDedicatedKey(channel, kbtype + (mShift ? "*" : "8"), pressed); break;
        case 510: sendDedicatedKey(channel, kbtype + (mShift ? "(" : "9"), pressed); break;
        case 511: sendDedicatedKey(channel, kbtype + (mShift ? ")" : "0"), pressed); break;
        case 512: sendDedicatedKey(channel, kbtype + (mShift ? "_" : "-"), pressed); break;
        case 513: sendDedicatedKey(channel, kbtype + (mShift ? "+" : "="), pressed); break;
        case 514: sendDedicatedKey(channel, kbtype + "BACKSPACE", pressed); break;
        case 515: sendDedicatedKey(channel, kbtype + "TAB", pressed); break;
        case 516: sendDedicatedKey(channel, kbtype + (mShift ? "Q" : "q"), pressed); break;
        case 517: sendDedicatedKey(channel, kbtype + (mShift ? "W" : "w"), pressed); break;
        case 518: sendDedicatedKey(channel, kbtype + (mShift ? "E" : "e"), pressed); break;
        case 519: sendDedicatedKey(channel, kbtype + (mShift ? "R" : "r"), pressed); break;
        case 520: sendDedicatedKey(channel, kbtype + (mShift ? "T" : "t"), pressed); break;
        case 521: sendDedicatedKey(channel, kbtype + (mShift ? "Y" : "y"), pressed); break;
        case 522: sendDedicatedKey(channel, kbtype + (mShift ? "U" : "u"), pressed); break;
        case 523: sendDedicatedKey(channel, kbtype + (mShift ? "I" : "i"), pressed); break;
        case 524: sendDedicatedKey(channel, kbtype + (mShift ? "O" : "o"), pressed); break;
        case 525: sendDedicatedKey(channel, kbtype + (mShift ? "P" : "p"), pressed); break;
        case 526: sendDedicatedKey(channel, kbtype + (mShift ? "{" : "["), pressed); break;
        case 527: sendDedicatedKey(channel, kbtype + (mShift ? "}" : "]"), pressed); break;
        case 528: sendDedicatedKey(channel, kbtype + "ENTER", pressed); break;
        case 529: sendDedicatedKey(channel, kbtype + "CTRLL", pressed); break;
        case 530: sendDedicatedKey(channel, kbtype + (mShift ? "A" : "a"), pressed); break;
        case 531: sendDedicatedKey(channel, kbtype + (mShift ? "S" : "s"), pressed); break;
        case 532: sendDedicatedKey(channel, kbtype + (mShift ? "D" : "d"), pressed); break;
        case 533: sendDedicatedKey(channel, kbtype + (mShift ? "F" : "f"), pressed); break;
        case 534: sendDedicatedKey(channel, kbtype + (mShift ? "G" : "g"), pressed); break;
        case 535: sendDedicatedKey(channel, kbtype + (mShift ? "H" : "h"), pressed); break;
        case 536: sendDedicatedKey(channel, kbtype + (mShift ? "J" : "j"), pressed); break;
        case 537: sendDedicatedKey(channel, kbtype + (mShift ? "K" : "k"), pressed); break;
        case 538: sendDedicatedKey(channel, kbtype + (mShift ? "L" : "l"), pressed); break;
        case 539: sendDedicatedKey(channel, kbtype + (mShift ? ":" : ";"), pressed); break;
        case 540: sendDedicatedKey(channel, kbtype + (mShift ? "\"" : "'"), pressed); break;
        case 541: sendDedicatedKey(channel, kbtype + (mShift ? "~" : "`"), pressed); break;
        case 542: sendDedicatedKey(channel, kbtype + "SHIFTL", pressed); mShift = !mShift; break;
        case 543: sendDedicatedKey(channel, kbtype + (mShift ? "|" : "\\"), pressed); break;
        case 544: sendDedicatedKey(channel, kbtype + (mShift ? "Z" : "z"), pressed); break;
        case 545: sendDedicatedKey(channel, kbtype + (mShift ? "X" : "x"), pressed); break;
        case 546: sendDedicatedKey(channel, kbtype + (mShift ? "C" : "c"), pressed); break;
        case 547: sendDedicatedKey(channel, kbtype + (mShift ? "V" : "v"), pressed); break;
        case 548: sendDedicatedKey(channel, kbtype + (mShift ? "B" : "b"), pressed); break;
        case 549: sendDedicatedKey(channel, kbtype + (mShift ? "N" : "n"), pressed); break;
        case 550: sendDedicatedKey(channel, kbtype + (mShift ? "M" : "m"), pressed); break;
        case 551: sendDedicatedKey(channel, kbtype + (mShift ? "<" : ","), pressed); break;
        case 552: sendDedicatedKey(channel, kbtype + (mShift ? ">" : "."), pressed); break;
        case 553: sendDedicatedKey(channel, kbtype + (mShift ? "?" : "/"), pressed); break;
        case 554: sendDedicatedKey(channel, kbtype + "SHIFTR", pressed); mShift = !mShift; break;
        case 556: sendDedicatedKey(channel, kbtype + "ALT", pressed); break;
        case 557: sendDedicatedKey(channel, kbtype + "SPACE", pressed); break;
        case 558: sendDedicatedKey(channel, kbtype + "CAPS", pressed); mCapsLock = !mCapsLock; break;
        case 559: sendDedicatedKey(channel, kbtype + "F1", pressed); break;
        case 560: sendDedicatedKey(channel, kbtype + "F2", pressed); break;
        case 561: sendDedicatedKey(channel, kbtype + "F3", pressed); break;
        case 562: sendDedicatedKey(channel, kbtype + "F4", pressed); break;
        case 563: sendDedicatedKey(channel, kbtype + "F5", pressed); break;
        case 564: sendDedicatedKey(channel, kbtype + "F6", pressed); break;
        case 565: sendDedicatedKey(channel, kbtype + "F7", pressed); break;
        case 566: sendDedicatedKey(channel, kbtype + "F8", pressed); break;
        case 567: sendDedicatedKey(channel, kbtype + "F9", pressed); break;
        case 568: sendDedicatedKey(channel, kbtype + "F10", pressed); break;
        case 587: sendDedicatedKey(channel, kbtype + "F11", pressed); break;
        case 588: sendDedicatedKey(channel, kbtype + "F12", pressed); break;
        case 597: sendDedicatedKey(channel, kbtype + "CTRLR", pressed); break;
        case 600: sendDedicatedKey(channel, kbtype + "ALTGR", pressed); break;
        case 602: sendDedicatedKey(channel, kbtype + "HOME", pressed); break;
        case 603: sendDedicatedKey(channel, kbtype + "UP", pressed); break;
        case 604: sendDedicatedKey(channel, kbtype + "PGDN", pressed); break;
        case 605: sendDedicatedKey(channel, kbtype + "LEFT", pressed); break;
        case 606: sendDedicatedKey(channel, kbtype + "RIGHT", pressed); break;
        case 607: sendDedicatedKey(channel, kbtype + "END", pressed); break;
        case 608: sendDedicatedKey(channel, kbtype + "DOWN", pressed); break;
        case 609: sendDedicatedKey(channel, kbtype + "PGDN", pressed); break;
        case 610: sendDedicatedKey(channel, kbtype + "INS", pressed); break;
        case 611: sendDedicatedKey(channel, kbtype + "DEL", pressed); break;
    }

    if ((mShift && channel != 554 && channel != 542) || mCapsLock)
        mShift = false;
}

void TSystemButton::sendDedicatedKey(int channel, const string str, bool pressed)
{
    DECL_TRACER("TSystemButton::sendDedicatedKey(int channel, const string str)");

    TButton *button = getSysButton(channel, 0);

    if (button)
        button->setActive(pressed ? STATE_ON : STATE_OFF);

    gPageManager->sendKeyboard(str);
}

/**
 * @brief Translates the button types into a string.
 * This method is for debugging purposes only!
 *
 * @param type  The enum number of the button type.
 *
 * @return Returns the string representation of the button type.
 */
string TSystemButton::typeToString(TSystemButton::SYS_BUUTON_TYPE type)
{
    switch (type)
    {
        case KEY_BUTTON:            return "KEY_BUTTON";
        case KEY_INPUT_MULTI:       return "KEY_INPUT_MULTI";
        case KEY_INPUT_SINGLE:      return "KEY_INPUT_SINGLE";
        case KEY_KEY:               return "KEY_KEY";
        case KEY_UNDEFINED:         return "KEY_UNDEFINED";
    }

    return string();
}
