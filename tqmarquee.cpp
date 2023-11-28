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
#include <QPainter>
#include <QString>
#include <QStaticText>

#include "tqmarquee.h"
#include "terror.h"

TQMarquee::TQMarquee(QWidget* parent)
    : QLabel(parent),
      mParent(parent)
{
    DECL_TRACER("TQMarquee::TQMarquee(QWidget* parent)");

    init();
}

TQMarquee::TQMarquee(QWidget* parent, int msec, MQ_TYPES type, bool enable)
    : QLabel(parent),
      mParent(parent),
      mScrollEnabled(enable),
      mType(type),
      mSpeed(msec)
{
    DECL_TRACER("TQMarquee::TQMarquee(QWidget* parent, int msec, MQ_TYPES type, bool enable)");

    if (mSpeed < 1)
        mSpeed = 1;
    else if (mSpeed > 10)
        mSpeed = 10;

    init();
}

TQMarquee::~TQMarquee()
{
    DECL_TRACER("TQMarquee::~TQMarquee()");
}

void TQMarquee::init()
{
    DECL_TRACER("TQMarquee::init()");

    switch(mType)
    {
        case MQ_LEFT:   mDirection = Qt::RightToLeft; break;
        case MQ_RIGHT:  mDirection = Qt::LeftToRight; break;

        default:
            mDirection = Qt::RightToLeft;
    }

    connect(&mTimer, &QTimer::timeout, this, &TQMarquee::refreshLabel);
    // By default the background image is transparent
    mBackgroundImage.fill(Qt::transparent);
    setBackgroundColor(mBgColor);
    mTimer.start(mSpeed);
}

void TQMarquee::setAlignment(Qt::Alignment al)
{
    DECL_TRACER("TQMarquee::setAlignment(Qt::Alignment al)");

    mAlign = al;
    updateCoordinates();
    QLabel::setAlignment(al);
}

void TQMarquee::setFrame(int left, int top, int right, int bottom)
{
    DECL_TRACER("TQMarquee::setFrame(int left, int top, int right, int bottom)");

    mFrameLeft = (left > 0 && left < width() ? left : 0);
    mFrameTop = (top > 0 && top < height() ? top : 0);
    mFrameRight = (right > 0 && right < width() ? right : 0);
    mFrameBottom = (bottom > 0 && bottom < height() ? bottom : 0);

    if ((mFrameLeft + mFrameRight) > width())
        mFrameLeft = mFrameRight = 0;

    if ((mFrameTop + mFrameBottom) > height())
        mFrameTop = mFrameBottom = 0;

    MSG_DEBUG("Frame size: l=" << mFrameLeft << ", r=" << mFrameRight << ", t=" << mFrameTop << ", b=" << mFrameBottom);
}

void TQMarquee::setText(const QString& text)
{
    DECL_TRACER("TQMarquee::setText(const QString& text)");

    mText = text;
    refreshLabel();
    update();
}

void TQMarquee::setSpeed(int msec)
{
    DECL_TRACER("TQMarquee::setSpeed(int msec)");

    if (msec < 1 || msec > 10)
    {
        MSG_WARNING("Wrong speed " << msec << "! The speed must be between 1 and 10.");
        return;
    }

    mSpeed = msec;

    if (mTimer.isActive())
        mTimer.start(mSpeed);
    else
        mTimer.setInterval(mSpeed);
}

void TQMarquee::pause()
{
    DECL_TRACER("TQMarquee::pause()");

    if (mTimer.isActive())
        mTimer.stop();
}

void TQMarquee::resume()
{
    DECL_TRACER("TQMarquee::resume()");

    if (!mTimer.isActive())
        mTimer.start(mSpeed);
}

QColor TQMarquee::backgroundColor()
{
    DECL_TRACER("TQMarquee::backgroundColor()");

    QPalette::ColorRole colorRole = backgroundRole();
    QBrush brush = palette().brush(colorRole);
    return brush.color();
}

void TQMarquee::setBackgroundColor(QColor& color)
{
    DECL_TRACER("TQMarquee::setBackgroundColor(QColor& color)");

    QBrush brush;
    brush.setColor(color);
    QPalette pal;
    pal.setBrush(backgroundRole(), brush);
    setPalette(pal);
}

QPixmap TQMarquee::background()
{
    DECL_TRACER("TQMarquee::background()");

    return mBackgroundImage;
}

void TQMarquee::setBackground(QPixmap& image)
{
    DECL_TRACER("TQMarquee::setBackground(QPixmap& image)");

    mBackgroundImage = image;
    setPixmap(mBackgroundImage);
    repaint();
}

void TQMarquee::setDirection(MQ_TYPES type)
{
    DECL_TRACER("TQMarquee::setDirection(MQ_TYPES type)");

    mType = type;

    switch(type)
    {
        case MQ_LEFT:   mDirection = Qt::RightToLeft; break;
        case MQ_RIGHT:  mDirection = Qt::LeftToRight; break;

        default:
            mDirection = Qt::RightToLeft;
    }

    if (mType == MQ_RIGHT || mType == MQ_PONG)
        px = width() - (mTextLength + mFrameLeft + mFrameRight);
    else if (mType == MQ_LEFT)
        px = mFrameLeft;
    else if (mType == MQ_DOWN)
        py = height() - (mTextHeight + mFrameTop + mFrameBottom);
    else if (mType == MQ_UP)
        py = mFrameTop;

    refreshLabel();
}

void TQMarquee::refreshLabel()
{
    DECL_TRACER("TQMarquee::refreshLabel()");

    repaint();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void TQMarquee::paintEvent(QPaintEvent*)
{
//    DECL_TRACER("TQMarquee::paintEvent(QPaintEvent*)");   // This would fill up a logfile, if there is one.

    QPainter p(this);
    p.drawPixmap(0, 0, mBackgroundImage);

    if (mScrollEnabled)
    {
        if(mType == MQ_LEFT)
        {
            px -= mSpeed;

            if(px <= (-mTextLength))
                px = width();
        }
        else if (mType == MQ_RIGHT)
        {
            px += mSpeed;

            if(px >= width())
                px = -mTextLength;
        }
        else if (mType == MQ_PONG)
        {
            if (mDirection == Qt::LeftToRight)
            {
                px -= mSpeed;
                bool changeDirection = false;

                if (mTextLength > width())
                    changeDirection = (px <= ((mTextLength - (width() - mFrameLeft - mFrameRight)) * -1));
                else
                    changeDirection = (px <= mFrameLeft);

                if (changeDirection)
                    mDirection = Qt::RightToLeft;
            }
            else
            {
                px += mSpeed;
                bool changeDirection = false;

                if (mTextLength > width())
                    changeDirection = (px >= mFrameLeft);
                else
                    changeDirection = (px >= (width() - mTextLength - mFrameRight));

                if (changeDirection)
                    mDirection = Qt::LeftToRight;
            }
        }
        else if (mType == MQ_DOWN)
        {
            py += mSpeed;

            if (py >= height())
                py = -mTextHeight;
        }
        else if (mType == MQ_UP)
        {
            py -= mSpeed;

            if (py <= (-mTextHeight))
                py = height();
        }

        if (mFrameLeft || mFrameRight || mFrameTop || mFrameBottom)
            p.setClipRect(QRect(mFrameLeft, mFrameTop, width() - mFrameLeft - mFrameRight, height() - mFrameBottom - mFrameTop), Qt::ReplaceClip);

        p.drawText(px, py + mFontPointSize, mText);
        p.translate(px,0);
    }
    else
        QLabel::setText(mText);
}

void TQMarquee::resizeEvent(QResizeEvent* evt)
{
    DECL_TRACER("TQMarquee::resizeEvent(QResizeEvent* evt)");

    updateCoordinates();
    QLabel::resizeEvent(evt);
}

void TQMarquee::updateCoordinates()
{
    DECL_TRACER("TQMarquee::updateCoordinates()");

    switch(mAlign)
    {
        case Qt::AlignTop:      py = mFrameTop; break;
        case Qt::AlignBottom:   py = height() - (mFrameTop + mFrameBottom); break;
        case Qt::AlignVCenter:  py = height() / 2; break;
    }

    mFontPointSize = font().pointSize() / 2;
    mTextLength = fontMetrics().horizontalAdvance(mText);
    mTextHeight = fontMetrics().height();

    if (mType == MQ_UP || mType == MQ_DOWN)
    {
        if (width() < mTextLength)
            px = ((mTextLength - width()) / 2) * -1;
        else
            px = width() - mTextLength / 2;
    }
}
