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

#include <QPushButton>
#ifdef QT5_LINUX
#include <QSound>
#else
#include <QMediaPlayer>
#include <QAudioOutput>
#endif

#include "tqkeyboard.h"
#include "ui_keyboard.h"
#include "terror.h"
#include "tresources.h"
#include "tconfig.h"
#include "tpagemanager.h"

extern TPageManager *gPageManager;              //!< The pointer to the global defined main class.

TQKeyboard::TQKeyboard(const std::string& init, const std::string& prompt, QWidget *parent, bool priv)
    : QDialog(parent),
      TSystemSound(TConfig::getSystemPath(TConfig::SOUNDS)),
      ui(new Ui::TQKeyboard),
      mPrivate(priv),
      mText(init)
{
    DECL_TRACER("TQKeyboard::TQKeyboard(const std::string& init, const std::string& prompt, QWidget *parent, bool priv)");

    ui->setupUi(this);
    connect(ui->key_Enter, SIGNAL(pressed()), this, SLOT(accept()));
    connect(ui->key_Cancel, SIGNAL(pressed()), this, SLOT(reject()));

    connect(ui->key_A, SIGNAL(pressed()), this, SLOT(keyA()));
    connect(ui->key_B, SIGNAL(pressed()), this, SLOT(keyB()));
    connect(ui->key_C, SIGNAL(pressed()), this, SLOT(keyC()));
    connect(ui->key_D, SIGNAL(pressed()), this, SLOT(keyD()));
    connect(ui->key_E, SIGNAL(pressed()), this, SLOT(keyE()));
    connect(ui->key_F, SIGNAL(pressed()), this, SLOT(keyF()));
    connect(ui->key_G, SIGNAL(pressed()), this, SLOT(keyG()));
    connect(ui->key_H, SIGNAL(pressed()), this, SLOT(keyH()));
    connect(ui->key_I, SIGNAL(pressed()), this, SLOT(keyI()));
    connect(ui->key_J, SIGNAL(pressed()), this, SLOT(keyJ()));
    connect(ui->key_K, SIGNAL(pressed()), this, SLOT(keyK()));
    connect(ui->key_L, SIGNAL(pressed()), this, SLOT(keyL()));
    connect(ui->key_M, SIGNAL(pressed()), this, SLOT(keyM()));
    connect(ui->key_N, SIGNAL(pressed()), this, SLOT(keyN()));
    connect(ui->key_O, SIGNAL(pressed()), this, SLOT(keyO()));
    connect(ui->key_P, SIGNAL(pressed()), this, SLOT(keyP()));
    connect(ui->key_Q, SIGNAL(pressed()), this, SLOT(keyQ()));
    connect(ui->key_R, SIGNAL(pressed()), this, SLOT(keyR()));
    connect(ui->key_S, SIGNAL(pressed()), this, SLOT(keyS()));
    connect(ui->key_T, SIGNAL(pressed()), this, SLOT(keyT()));
    connect(ui->key_U, SIGNAL(pressed()), this, SLOT(keyU()));
    connect(ui->key_V, SIGNAL(pressed()), this, SLOT(keyV()));
    connect(ui->key_W, SIGNAL(pressed()), this, SLOT(keyW()));
    connect(ui->key_X, SIGNAL(pressed()), this, SLOT(keyX()));
    connect(ui->key_Y, SIGNAL(pressed()), this, SLOT(keyY()));
    connect(ui->key_Z, SIGNAL(pressed()), this, SLOT(keyZ()));
    connect(ui->key_AE, SIGNAL(pressed()), this, SLOT(keyAE()));
    connect(ui->key_OE, SIGNAL(pressed()), this, SLOT(keyOE()));
    connect(ui->key_UE, SIGNAL(pressed()), this, SLOT(keyUE()));
    connect(ui->key_SS, SIGNAL(pressed()), this, SLOT(keySS()));
    connect(ui->key_Caret, SIGNAL(pressed()), this, SLOT(keyCaret()));
    connect(ui->key_SQ, SIGNAL(pressed()), this, SLOT(keySQ()));
    connect(ui->key_Backspace, SIGNAL(pressed()), this, SLOT(keyBackspace()));
    connect(ui->key_Plus, SIGNAL(pressed()), this, SLOT(keyPlus()));
    connect(ui->key_Hash, SIGNAL(pressed()), this, SLOT(keyHash()));
    connect(ui->key_Komma, SIGNAL(pressed()), this, SLOT(keyKomma()));
    connect(ui->key_Dot, SIGNAL(pressed()), this, SLOT(keyDot()));
    connect(ui->key_Dash, SIGNAL(pressed()), this, SLOT(keyDash()));
    connect(ui->key_GtLt, SIGNAL(pressed()), this, SLOT(keyGtLt()));
    connect(ui->key_Tab, SIGNAL(pressed()), this, SLOT(keyTab()));
    connect(ui->key_Caps, SIGNAL(pressed()), this, SLOT(keyCaps()));
    connect(ui->key_Shift, SIGNAL(pressed()), this, SLOT(keyShift()));
    connect(ui->key_Blank, SIGNAL(pressed()), this, SLOT(keyBlank()));
    connect(ui->key_Clear, SIGNAL(pressed()), this, SLOT(keyClear()));
    connect(ui->key_AltGR, SIGNAL(pressed()), this, SLOT(keyAltGr()));
    connect(ui->key_1, SIGNAL(pressed()), this, SLOT(key1()));
    connect(ui->key_2, SIGNAL(pressed()), this, SLOT(key2()));
    connect(ui->key_3, SIGNAL(pressed()), this, SLOT(key3()));
    connect(ui->key_4, SIGNAL(pressed()), this, SLOT(key4()));
    connect(ui->key_5, SIGNAL(pressed()), this, SLOT(key5()));
    connect(ui->key_6, SIGNAL(pressed()), this, SLOT(key6()));
    connect(ui->key_7, SIGNAL(pressed()), this, SLOT(key7()));
    connect(ui->key_8, SIGNAL(pressed()), this, SLOT(key8()));
    connect(ui->key_9, SIGNAL(pressed()), this, SLOT(key9()));
    connect(ui->key_0, SIGNAL(pressed()), this, SLOT(key0()));

    ui->label_Prompt->setText(prompt.c_str());

    if (!mPrivate)
        ui->label_TextLine->setText(init.c_str());
    else
        ui->label_TextLine->setText(fillString('*', mText.length()).c_str());
}

TQKeyboard::~TQKeyboard()
{
    DECL_TRACER("TQKeyboard::~TQKeyboard()");
    delete ui;
}

void TQKeyboard::doResize()
{
    DECL_TRACER("TQKeyboard::doResize()");

    QRect rect = this->geometry();
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    QSize size = this->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    this->resize(size);
#endif
    QWidget *parent = this->parentWidget();

    if (parent)     // Move window to lower left corner
    {
        rect = parent->geometry();
        this->move(0, rect.height() - this->geometry().height());
    }
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // Iterate through childs and resize them
    QObjectList childs = children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QString name = iter.i->t()->objectName();
        QObject *obj = iter.i->t();

        if (name.startsWith("key_"))
        {
            QPushButton *bt = dynamic_cast<QPushButton *>(obj);
            size = bt->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            bt->resize(size);
            rect = bt->geometry();
            bt->move(scale(rect.left()), scale(rect.top()));
        }
        else    // It's a label
        {
            QLabel *lb = dynamic_cast<QLabel *>(obj);
            size = lb->size();
            size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
            lb->resize(size);
            rect = lb->geometry();
            lb->move(scale(rect.left()), scale(rect.top()));
        }
    }
#endif
}

void TQKeyboard::setKey(Ui::KEYS_t key)
{
    DECL_TRACER("TQKeyboard::setKey(Ui::KEYS_t key)");

    switch(key)
    {
        case Ui::KEY_0:         mText += ((mShift || mCaps) ? "=" : (mGr ? "}" : "0")); break;
        case Ui::KEY_1:         mText += ((mShift || mCaps) ? "!" : "1"); break;
        case Ui::KEY_2:         mText += ((mShift || mCaps) ? "\"" : (mGr ? "²" : "2")); break;
        case Ui::KEY_3:         mText += ((mShift || mCaps) ? "§" : (mGr ? "³" : "3")); break;
        case Ui::KEY_4:         mText += ((mShift || mCaps) ? "$" : "4"); break;
        case Ui::KEY_5:         mText += ((mShift || mCaps) ? "%" : "5"); break;
        case Ui::KEY_6:         mText += ((mShift || mCaps) ? "&" : "6"); break;
        case Ui::KEY_7:         mText += ((mShift || mCaps) ? "/" : (mGr ? "{" : "7")); break;
        case Ui::KEY_8:         mText += ((mShift || mCaps) ? "(" : (mGr ? "[" : "8")); break;
        case Ui::KEY_9:         mText += ((mShift || mCaps) ? ")" : (mGr ? "]" : "9")); break;
        case Ui::KEY_A:         mText += ((mShift || mCaps) ? "A" : "a"); break;
        case Ui::KEY_B:         mText += ((mShift || mCaps) ? "B" : "b"); break;
        case Ui::KEY_C:         mText += ((mShift || mCaps) ? "C" : "c"); break;
        case Ui::KEY_D:         mText += ((mShift || mCaps) ? "D" : "d"); break;
        case Ui::KEY_E:         mText += ((mShift || mCaps) ? "E" : (mGr ? "€" : "e")); break;
        case Ui::KEY_F:         mText += ((mShift || mCaps) ? "F" : "f"); break;
        case Ui::KEY_G:         mText += ((mShift || mCaps) ? "G" : "g"); break;
        case Ui::KEY_H:         mText += ((mShift || mCaps) ? "H" : "h"); break;
        case Ui::KEY_I:         mText += ((mShift || mCaps) ? "I" : "i"); break;
        case Ui::KEY_J:         mText += ((mShift || mCaps) ? "J" : "j"); break;
        case Ui::KEY_K:         mText += ((mShift || mCaps) ? "K" : "k"); break;
        case Ui::KEY_L:         mText += ((mShift || mCaps) ? "L" : "l"); break;
        case Ui::KEY_M:         mText += ((mShift || mCaps) ? "M" : (mGr ? "µ" : "m")); break;
        case Ui::KEY_N:         mText += ((mShift || mCaps) ? "N" : "n"); break;
        case Ui::KEY_O:         mText += ((mShift || mCaps) ? "O" : "o"); break;
        case Ui::KEY_P:         mText += ((mShift || mCaps) ? "P" : "p"); break;
        case Ui::KEY_Q:         mText += ((mShift || mCaps) ? "Q" : (mGr ? "@" : "q")); break;
        case Ui::KEY_R:         mText += ((mShift || mCaps) ? "R" : "r"); break;
        case Ui::KEY_S:         mText += ((mShift || mCaps) ? "S" : "s"); break;
        case Ui::KEY_T:         mText += ((mShift || mCaps) ? "T" : "t"); break;
        case Ui::KEY_U:         mText += ((mShift || mCaps) ? "U" : "u"); break;
        case Ui::KEY_V:         mText += ((mShift || mCaps) ? "V" : "v"); break;
        case Ui::KEY_W:         mText += ((mShift || mCaps) ? "W" : "w"); break;
        case Ui::KEY_X:         mText += ((mShift || mCaps) ? "X" : "x"); break;
        case Ui::KEY_Y:         mText += ((mShift || mCaps) ? "Y" : "y"); break;
        case Ui::KEY_Z:         mText += ((mShift || mCaps) ? "Z" : "z"); break;
        case Ui::KEY_AE:        mText += ((mShift || mCaps) ? "Ä" : "ä"); break;
        case Ui::KEY_OE:        mText += ((mShift || mCaps) ? "Ö" : "ö"); break;
        case Ui::KEY_UE:        mText += ((mShift || mCaps) ? "Ü" : "ü"); break;
        case Ui::KEY_SS:        mText += ((mShift || mCaps) ? "?" : (mGr ? "\\" : "ß")); break;
        case Ui::KEY_SQ:        mText += ((mShift || mCaps) ? "`" : "´"); break;
        case Ui::KEY_Caret:     mText += ((mShift || mCaps) ? "°" : "^"); break;
        case Ui::KEY_Clear:     mText.clear(); break;
        case Ui::KEY_Backspace: mText = mText.substr(0, mText.length() -1); break;
        case Ui::KEY_Plus:      mText += ((mShift || mCaps) ? "*" : (mGr ? "~" : "+")); break;
        case Ui::KEY_Hash:      mText += ((mShift || mCaps) ? "'" : "#"); break;
        case Ui::KEY_Komma:     mText += ((mShift || mCaps) ? ";" : ","); break;
        case Ui::KEY_Dot:       mText += ((mShift || mCaps) ? ":" : "."); break;
        case Ui::KEY_Dash:      mText += ((mShift || mCaps) ? "_" : "-"); break;
        case Ui::KEY_GtLt:      mText += ((mShift || mCaps) ? ">" : (mGr ? "|" : "<")); break;
        case Ui::KEY_Tab:       mText += "\t"; break;
        case Ui::KEY_Caps:      mCaps = !mCaps; break;
        case Ui::KEY_Shift:     mShift = true; break;
        case Ui::KEY_AltGR:     mGr = true; break;
        case Ui::KEY_Blank:     mText += " "; break;
    }

    if (key != Ui::KEY_Shift)
        mShift = false;

    if (key != Ui::KEY_AltGR)
        mGr = false;

    if (mMaxLen > 0 && mText.length() > (size_t)mMaxLen)
        mText = mText.substr(0, mMaxLen);

    if (!mPrivate)
        ui->label_TextLine->setText(mText.c_str());
    else
        ui->label_TextLine->setText(fillString('*', mText.length()).c_str());

    if (getSystemSoundState())
    {
        std::string snd = getTouchFeedbackSound();
        MSG_DEBUG("Playing sound: " << snd);
#ifdef QT6_LINUX
        QMediaPlayer *player = new QMediaPlayer;
        QAudioOutput *audioOutput = new QAudioOutput;
        player->setAudioOutput(audioOutput);
        // ...
        player->setSource(QUrl::fromLocalFile(snd.c_str()));
        player->play();
        delete player;
        delete audioOutput;
#else
        QSound::play(snd.c_str());
#endif
    }

    if (gPageManager && gPageManager->getPassThrough() && !mText.empty() &&
        key != Ui::KEY_Backspace && key != Ui::KEY_Clear &&
        key != Ui::KEY_Shift && key != Ui::KEY_AltGR && key != Ui::KEY_Caps)
    {
        size_t pos = mText.length() - 1;
        gPageManager->sendKeyStroke(mText[pos]);
    }
}

void TQKeyboard::setString(const std::string& str)
{
    DECL_TRACER("TQKeyboard::setString(const string& str)");

    mText += str;

    if (mMaxLen > 0 && mText.length() > (size_t)mMaxLen)
        mText = mText.substr(0, mMaxLen);

    if (!mPrivate)
        ui->label_TextLine->setText(mText.c_str());
    else
        ui->label_TextLine->setText(fillString('*', mText.length()).c_str());
}

int TQKeyboard::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}
