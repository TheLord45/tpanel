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

#ifndef __TQMARQUEE_H__
#define __TQMARQUEE_H__

#include <QLabel>
#include <QTimer>

class QString;
class QPaintEvent;
class QResizeEvent;
class QHideEvent;
class QShowEvent;
class QStaticText;
class QPixmap;
class QFont;
class QRawFont;

class TQMarquee : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)

    public:
        typedef enum MQ_TYPES
        {
            MQ_NONE,    // 0
            MQ_LEFT,    // 1
            MQ_RIGHT,   // 2
            MQ_PONG,    // 3
            MQ_UP,      // 4
            MQ_DOWN     // 5
        }MQ_TYPES;

        TQMarquee(QWidget *parent=0);
        TQMarquee(QWidget *parent, int msec, MQ_TYPES type=MQ_LEFT, bool enable=true);
        ~TQMarquee();

        void setAlignment(Qt::Alignment al);
        void setFrame(int left, int top, int right, int bottom);
        void setSpeed(int msec);
        void pause();
        void resume();

    public slots:
        QString text() const { return mText; }
        void setText(const QString& text);
        QColor backgroundColor();
        void setBackgroundColor(QColor& color);
        QPixmap background();
        void setBackground(QPixmap& bitmap);
        void setDirection(MQ_TYPES type);

    protected:
        virtual void paintEvent(QPaintEvent *);
        virtual void resizeEvent(QResizeEvent *);
        virtual void hideEvent(QHideEvent *);
        virtual void showEvent(QShowEvent *);
        void updateCoordinates();

    private slots:
        void refreshLabel();

    private:
        void init();
        bool testVisibility(const QRegion& region);

        QWidget *mParent{nullptr};
        QString mText;
        bool mScrollEnabled{true};
        MQ_TYPES mType{MQ_NONE};
        QPixmap mBackgroundImage;
        QColor mBgColor{Qt::transparent};
        int mFrameLeft{0};
        int mFrameTop{0};
        int mFrameRight{0};
        int mFrameBottom{0};
        bool mPaused{false};

        int px{0};
        int py{0};
        QTimer *mTimer{nullptr};
        uint mDelay{10};         // Delay of drawing im milli seconds
        Qt::Alignment mAlign{Qt::AlignCenter};
        int mSpeed{200};
        Qt::LayoutDirection mDirection{Qt::LeftToRight};
        int mFontPointSize{8};
        int mTextLength{0};
        int mTextHeight{0};
};

#endif
