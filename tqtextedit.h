/*
 * Copyright (C) 2022, 2023 by Andreas Theofilu <andreas@theosys.at>
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

#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
class QPixmap;
QT_END_NAMESPACE

class TQTextEdit : public QWidget
{
    Q_OBJECT

    public:
        TQTextEdit(QWidget *parent=nullptr);
        TQTextEdit(const QString& text, QWidget *parent=nullptr);

        void setText(const QString& text);
        QString& getText() { return mText; }
        void setHandle(ulong handle) { mHandle = handle; }
        ulong getHandle() { return mHandle; }
        void setBackgroundPixmap(const QPixmap& pixmap);
        QPixmap& getBackgroundPixmap() { return mBackground; }
        void setAlignment(Qt::Alignment al);
        void setPadding(int left, int top, int right, int bottom);
        void setPasswordChar(char c) { mPwChar = (c > 0 ? c : '*'); }

    signals:
        void contentChanged(const QString& text);

    protected:
        virtual bool event(QEvent *event);
        virtual void paintEvent(QPaintEvent *);
        virtual void resizeEvent(QResizeEvent *);
        void updateCoordinates();

    private:
        void init();
        void append(const QString& txt);
        void insert(const QString& txt, int pos=-1);

        QString mText;
        int mPos{0};            // The current cursor position
        ulong mHandle{0};
        QPixmap mBackground;
        int mFontPointSize{8};
        int mTextLength{0};
        int mTextHeight{0};
        Qt::Alignment mAlignment{Qt::AlignLeft | Qt::AlignHCenter};
        int mPosX{0};
        int mPosY{0};
        int mPadLeft{0};
        int mPadTop{0};
        int mPadRight{0};
        int mPadBottom{0};
        bool mShift{false};
        bool mCapsLock{false};
        bool mCtrl{false};
        char mPwChar{'*'};
};

#endif
