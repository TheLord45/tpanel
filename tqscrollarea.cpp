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

#include <QWidget>
#include <QScrollArea>
#include <QPixmap>
#include <QColor>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPoint>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

#include "tqscrollarea.h"
#include "terror.h"
#include "tresources.h"

using std::vector;

/**
 * @brief TQScrollArea::TQScrollArea
 * Constructor for the class.
 */
TQScrollArea::TQScrollArea()
{
    DECL_TRACER("TQScrollArea::TQScrollArea()");
}

/**
 * @brief TQScrollArea::TQScrollArea
 * @param parent    The parent widget
 */
TQScrollArea::TQScrollArea(QWidget* parent)
    : mParent(parent)
{
    DECL_TRACER("TQScrollArea::TQScrollArea(QWidget* parent)");

    if (parent)
    {
        mWidth = parent->geometry().width();
        mHeight = parent->geometry().height();
    }

    init();
}

/**
 * @brief TQScrollArea::TQScrollArea
 * @param parent        Parent widget
 * @param w             Visible width in pixels
 * @param h             Visible height in pixels
 * @param vertical      TRUE: Scrolling in vertical direction
 */
TQScrollArea::TQScrollArea(QWidget* parent, int w, int h, bool vertical)
    : mParent(parent),
      mVertical(vertical)
{
    DECL_TRACER("TQScrollArea::TQScrollArea(QWidget* parent, int w, int h, bool vertical)");

    if (w > 0)
        mWidth = w;

    if (h > 0)
        mHeight = h;

    init();
}

/**
 * @brief TQScrollArea::TQScrollArea
 * @param parent    Parent widget
 * @param size      Size (width and height) of visible area
 * @param vertical  TRUE: Scrolling in vertical direction
 */
TQScrollArea::TQScrollArea(QWidget* parent, const QSize& size, bool vertical)
    : mParent(parent),
      mVertical(vertical)
{
    DECL_TRACER("TQScrollArea::TQScrollArea(QWidget* parent, const QSize& size, bool vertical)");

    if (size.width() > 0)
        mWidth = size.width();

    if (size.height() > 0)
        mHeight = size.height();

    init();
}

TQScrollArea::~TQScrollArea()
{
    DECL_TRACER("TQScrollArea::~TQScrollArea()");

    if (mMain)
        mMain->close();

    if (mMousePressTimer)
    {
        if (mMousePressTimer->isActive())
            mMousePressTimer->stop();

        delete mMousePressTimer;
        mMousePressTimer = nullptr;
    }
}

void TQScrollArea::init()
{
    DECL_TRACER("TQScrollArea::init()");

    QScrollArea::setParent(mParent);
    QScrollArea::setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QScrollArea::move(0, 0);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Set a transparent background
    QPalette palette(this->palette());
    palette.setColor(QPalette::Window, QColor(Qt::transparent));
    setPalette(palette);

    if (!mMain)
        mMain = new QWidget(this);

    mMain->move(0, 0);

    if (mWidth && mHeight)
        setFixedSize(mWidth, mHeight);

    setWidget(mMain);

    if (mVertical && !mVLayout)
        mVLayout = new QVBoxLayout(mMain);
    else if (!mVertical && !mHLayout)
        mHLayout = new QHBoxLayout(mMain);

    if (!mTotalWidth)
        mTotalWidth = mWidth;

    if (!mTotalHeight)
        mTotalHeight = mHeight;
}

/**
 * @brief TQScrollArea::setObjectName
 * This sets the object name of the internal main QWidget.
 *
 * @param name  The name of the object.
 */
void TQScrollArea::setObjectName(const QString& name)
{
    DECL_TRACER("TQScrollArea::setObjectName(const QString& name)");

    if (mMain)
        mMain->setObjectName(name);
}

/**
 * @brief TQScrollArea::setSize
 * Sets the size of the visible area of the object.
 *
 * @param w     The width in pixels
 * @param h     The height in pixels
 */
void TQScrollArea::setSize(int w, int h)
{
    DECL_TRACER("TQScrollArea::setSize(int w, int h)");

    if (w < 1 || h < 1)
        return;

    mWidth = w;
    mHeight = h;
    setFixedSize(mWidth, mHeight);
}

/**
 * @brief TQScrollArea::setSize
 * Sets the size of the visible area of the object.
 *
 * @param size  The size (width and height)
 */
void TQScrollArea::setSize(QSize size)
{
    DECL_TRACER("TQScrollArea::setSize(QSize size)");

    mWidth = size.width();
    mHeight = size.height();
    setFixedSize(mWidth, mHeight);
}

/**
 * @brief TQScrollArea::getSize
 * Returns the size of the visible area.
 *
 * @return A QSize object containg the size in pixels.
 */
QSize TQScrollArea::getSize()
{
    DECL_TRACER("TQScrollArea::getSize()");

    return QSize(mWidth, mHeight);
}

/**
 * @brief TQScrollArea::setTotalWidth
 * Sets the total width of the scrolling area. This must be equal or grater
 * then the width of the visible area.
 *
 * @param w The width in pixels.
 */
void TQScrollArea::setTotalWidth(int w)
{
    DECL_TRACER("TQScrollArea::setTotalWidth(int w)");

    if (!mMain || w < mWidth)
        return;

    mTotalWidth = w;
    mMain->setFixedWidth(mTotalWidth);
}

/**
 * @brief TQScrollArea::setTotalHeight
 * Sets the total height of the scrolling area. This must be equal or grater
 * then the height of the visible area.
 *
 * @param h  The height in pixels.
 */
void TQScrollArea::setTotalHeight(int h)
{
    DECL_TRACER("TQScrollArea::setTotalHeight(int h)");

    if (!mMain || h < mHeight)
        return;

    mTotalHeight = h;
    mMain->setFixedHeight(mTotalHeight);
}

/**
 * @brief TQScrollArea::setTotalSize
 * Sets the total size of the scrolling area. The width and height must be
 * equal or grater then the width and height of the visible area.
 *
 * @param w  The width in pixels.
 * @param h  The height in pixels.
 */
void TQScrollArea::setTotalSize(int w, int h)
{
    DECL_TRACER("TQScrollArea::setTotalSize(int w, int h)");

    if (!mMain || w < mWidth || h < mHeight)
        return;

    mTotalWidth = w;
    mTotalHeight = h;
    mMain->setFixedSize(mTotalWidth, mTotalHeight);
}

/**
 * @brief TQScrollArea::setTotalSize
 * Sets the total size of the scrolling area in pixels.
 *
 * @param size  The total size of the scrolling area.
 */
void TQScrollArea::setTotalSize(QSize& size)
{
    DECL_TRACER("TQScrollArea::setTotalSize(QSize& size)");

    if (!mMain || size.width() < mWidth || size.height() < mHeight)
        return;

    mTotalWidth = size.width();
    mTotalHeight = size.height();
    mMain->setFixedSize(mTotalWidth, mTotalHeight);
}

/**
 * @brief TQScrollArea::setBackgroundImage
 * This sets the background image of the scroll area. If the pixmap is smaller
 * then the total size, the image is painted in tiles.
 *
 * @param pix       The pixmap.
 */
void TQScrollArea::setBackgroundImage(const QPixmap& pix)
{
    DECL_TRACER("TQScrollArea::setBackgroundImage(const QPixmap& pix)");

    if (!mMain || pix.isNull())
        return;

    QPalette palette(mMain->palette());
    palette.setBrush(QPalette::Window, QBrush(pix));
    mMain->setPalette(palette);
}

/**
 * @brief TQScrollArea::setBackGroundColor
 * Sets the background color of the scroll area.
 *
 * @param color     The color
 */
void TQScrollArea::setBackGroundColor(QColor color)
{
    DECL_TRACER("TQScrollArea::setBackGroundColor(QColor color)");

    if (!mMain)
        return;

    QPalette palette(mMain->palette());
    palette.setColor(QPalette::Window, color);
    mMain->setPalette(palette);
}

/**
 * @brief TQScrollArea::setSpace
 * Sets the space between the items in percent. The pixels are calculated
 * from the total size of the scroll area. Then the spce is inserted.
 *
 * @param s     A value between 0 and 99. 0 Removes the space.
 */
void TQScrollArea::setSpace(int s)
{
    DECL_TRACER("TQScrollArea::setSpace(double s)");

    if (s < 0 || s > 99)
        return;

    mSpace = s;
}

/**
 * @brief TQScrollArea::addItem
 * Adds one item to the list of items.
 *
 * @param item  Item to add
 */
void TQScrollArea::addItem(PGSUBVIEWITEM_T& item)
{
    DECL_TRACER("TQScrollArea::addItem(PGSUBVIEWITEM_T& item)");

    mItems.push_back(item);
    addItems(mItems);
}

void TQScrollArea::addItems(std::vector<PGSUBVIEWITEM_T>& items)
{
    DECL_TRACER("TQScrollArea::addItems(std::vector<PGSUBVIEWITEM_T>& items)");

    if (items.empty())
        return;

    if (mMainWidgets.size() > 0)
    {
        vector<QWidget *>::iterator iter;

        for (iter = mMainWidgets.begin(); iter != mMainWidgets.end(); ++iter)
        {
            QWidget *w = *iter;
            w->close();
        }

        mMainWidgets.clear();
    }

    mItems = items;

    if (mVertical)
        mTotalHeight = 0;
    else
        mTotalWidth = 0;

    int total = 0;      // The total width or height
    vector<PGSUBVIEWITEM_T>::iterator iter;
    // First calculate the total width and height if it was not set by a previous call
    if ((mTotalWidth <= 0 && !mVertical) || (mTotalHeight <= 0 && mVertical))
    {
        if (mTotalWidth <= 0)
            mTotalWidth = mWidth;

        if (mTotalHeight <= 0)
            mTotalHeight = mHeight;

        if (!mVertical || mTotalWidth <= 0)
            mTotalWidth = 0;

        if (mVertical || mTotalHeight <= 0)
            mTotalHeight = 0;

        int num = 0;

        for (iter = items.begin(); iter != items.end(); ++iter)
        {
            if (!mVertical)
                mTotalWidth += iter->width;
            else
                mTotalHeight += iter->height;

            MSG_DEBUG("Item " << num << " (" << handleToString(iter->handle) << "): Size: " << iter->width << " x " << iter->height);
            num++;
        }

        if (mVertical)
        {
            mTotalHeight = scale(mTotalHeight);
            total = mTotalHeight;
        }
        else
        {
            mTotalWidth = scale(mTotalWidth);
            total = mTotalWidth;
        }

        if (mMain)
            mMain->setFixedSize(mTotalWidth, mTotalHeight);

        MSG_DEBUG("Total size: " << mTotalWidth << " x " << mTotalHeight << " (Vertical is " << (mVertical ? "TRUE" : "FALSE") << ")");
    }

    MSG_DEBUG("Number of items: " << items.size());

    if (mSpace > 0)
    {
        int space = (int)((double)total / 100.0 * (double)mSpace);

        if (space > 0 && mVertical && mVLayout && mMain)
        {
            int newHeight = space + mTotalHeight;
            mMain->setFixedHeight(newHeight);
            MSG_DEBUG("Calculated space: " << space << " (" << mSpace << "%). Total height: " << newHeight << ", Old total height: " << mTotalHeight);
            mTotalHeight = newHeight;
        }
        else if (space > 0 && !mVertical && mHLayout && mMain)
        {
            int newWidth = space + mTotalWidth;
            mMain->setFixedWidth(newWidth);
            MSG_DEBUG("Calculated space: " << space << " (" << mSpace << "%). Total width: " << newWidth << ", Old total width: " << mTotalWidth);
            mTotalWidth = newWidth;
        }
    }

    for (iter = items.begin(); iter != items.end(); ++iter)
    {
        int iWidth = scale(iter->width);
        int iHeight = scale(iter->height);
        QWidget *item = new QWidget;
        item->setObjectName(QString("Item_%1").arg(handleToString(iter->handle).c_str()));
        item->setFixedSize(iWidth, iHeight);
        item->setAutoFillBackground(true);
        QColor bgcolor(qRgba(iter->bgcolor.red, iter->bgcolor.green, iter->bgcolor.blue, iter->bgcolor.alpha));

        if (iter->image.getSize() > 0)
        {
            QPixmap pixmap(iWidth, iHeight);

            if (iter->bgcolor.alpha == 0)
                pixmap.fill(Qt::transparent);
            else
                pixmap.fill(bgcolor);

            QImage img(iter->image.getBitmap(), iter->image.getWidth(), iter->image.getHeight(), iter->image.getPixline(), QImage::Format_ARGB32);  // Original size
            bool ret = false;

            if (mScaleFactor != 1.0)
            {
                QSize size(iWidth, iHeight);
                ret = pixmap.convertFromImage(img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));   // Scaled size
            }
            else
                ret = pixmap.convertFromImage(img);

            if (!ret || pixmap.isNull())
            {
                MSG_ERROR("Unable to create a pixmap out of an image!");
                return;
            }

            QPalette palette(item->palette());
            palette.setBrush(QPalette::Window, QBrush(pixmap));
            item->setPalette(palette);
        }
        else
        {
            QPalette palette(item->palette());

            if (iter->bgcolor.alpha == 0)
                palette.setColor(QPalette::Window, Qt::transparent);
            else
                palette.setColor(QPalette::Window, bgcolor);

            item->setPalette(palette);
        }

        // Add the buttons to the item widget
        if (iter->atoms.empty())
        {
            delete item;
            continue;
        }

        vector<PGSUBVIEWATOM_T>::iterator itAtom;

        for (itAtom = iter->atoms.begin(); itAtom != iter->atoms.end(); ++itAtom)
        {
            int scaWidth = scale(itAtom->width);
            int scaHeight = scale(itAtom->height);

            QLabel *label = new QLabel(item);
            label->move(scale(itAtom->left), scale(itAtom->top));
            label->setFixedSize(scaWidth, scaHeight);
            label->setObjectName(QString("Label_%1").arg(handleToString(itAtom->handle).c_str()));
            QColor bg(qRgba(itAtom->bgcolor.red, itAtom->bgcolor.green, itAtom->bgcolor.blue, itAtom->bgcolor.alpha));

            if (itAtom->image.isValid())
            {
                QPixmap pix(scaWidth, scaHeight);

                if (itAtom->bgcolor.alpha == 0)
                    pix.fill(Qt::transparent);
                else
                    pix.fill(bg);

                QImage img(itAtom->image.getBitmap(), itAtom->image.getWidth(), itAtom->image.getHeight(), itAtom->image.getPixline(), QImage::Format_ARGB32);
                bool ret = false;

                if (mScaleFactor != 1.0)
                {
                    QSize size(scaWidth, scaHeight);
                    ret = pix.convertFromImage(img.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)); // Scaled size
                }
                else
                    ret = pix.convertFromImage(img);

                if (!ret || pix.isNull())
                {
                    MSG_ERROR("Unable to create a pixmap out of an image!");
                    return;
                }

                label->setPixmap(pix);
            }
            else
            {
                QPalette palette(label->palette());

                if (itAtom->bgcolor.alpha == 0)
                    palette.setColor(QPalette::Window, Qt::transparent);
                else
                    palette.setColor(QPalette::Window, bg);

                label->setPalette(palette);
            }
        }

        mMainWidgets.push_back(item);

        if (mVertical && mVLayout)
            mVLayout->addWidget(item);
        else if (!mVertical && mHLayout)
            mHLayout->addWidget(item);
        else
        {
            MSG_ERROR("Layout not initialized!");
        }
    }
}

int TQScrollArea::scale(int value)
{
    DECL_TRACER("TQScrollArea::scale(int value)");

    if (mScaleFactor != 1.0)
        return (int)((double)value * mScaleFactor);

    return value;
}

void TQScrollArea::setScaleFactor(const double& factor)
{
    DECL_TRACER("TQScrollArea::setScaleFactor(const double& factor)");

    if (factor > 0.0 && factor != 1.0)
        mScaleFactor = factor;
}

/*****************************************************************************
 * Signals and overwritten functions start here
 *****************************************************************************/

bool TQScrollArea::event(QEvent* event)
{
    if (event->type() == QEvent::MouseMove && mMousePress)
    {
        if (mMousePressTimer && mMousePressTimer->isActive())
        {
            mMousePressTimer->stop();
            mClick = false;
        }

        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        int move = 0;
        mMouseScroll = true;

        if (mVertical)
        {
            if (mOldPoint.y() != 0)
            {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                move = me->pos().y() - mOldPoint.y();
#else
                move = me->position().y() - mOldPoint.y();
#endif
                QScrollBar *bar = verticalScrollBar();

                if (bar)
                {
                    int value = bar->value();
                    value += (move * -1);
                    bar->setValue(value);
                }
            }
        }
        else
        {
            if (mOldPoint.x() != 0)
            {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                move = me->pos().x() - mOldPoint.x();
#else
                move = me->position().x() - mOldPoint.x();
#endif
                scrollContentsBy(move, 0);
                QScrollBar *bar = horizontalScrollBar();

                if (bar)
                {
                    int value = bar->value();
                    value += (move * -1);
                    bar->setValue(value);
                }
            }
        }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        mOldPoint = me->pos();
#else
        mOldPoint = me->position();
#endif
    }

    return QScrollArea::event(event);
}

void TQScrollArea::mousePressEvent(QMouseEvent* event)
{
    if (!event)
        return;

    if(event->button() == Qt::LeftButton)
    {
        mMousePress = true;
        mOldPoint.setX(0);
        mOldPoint.setY(0);

        int x = 0, y = 0;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            x = event->pos().x();
            y = event->pos().y();
#else
            x = event->position().x();
            y = event->position().y();
#endif
        mLastMousePress.setX(x);
        mLastMousePress.setY(y);

        MSG_DEBUG("Mouse press event at " << x << " x " << y << " // " << event->globalX() << " x " << event->globalY());
        /*
         * Here we're starting a timer with 200 ms. If after this time the
         * mouse button is still pressed and no scroll event was detected,
         * then we've a real click.
         * In case of a real click the method mouseTimerEvent() will call a
         * signal to inform the parent about the click.
         */
        mClick = true;

        if (!mMousePressTimer)
        {
            mMousePressTimer = new QTimer(this);
            connect(mMousePressTimer, &QTimer::timeout, this, &TQScrollArea::mouseTimerEvent);
            mMousePressTimer->start(200);
        }
        else
            mMousePressTimer->start();
    }
}

void TQScrollArea::mouseReleaseEvent(QMouseEvent* event)
{
    if (!event)
        return;

    if(event->button() == Qt::LeftButton)
    {
        mMouseScroll = false;

        if (mMousePressTimer && mMousePressTimer->isActive())
        {
            mMousePressTimer->stop();
            mClick = true;
            mouseTimerEvent();
            mClick = false;
            mouseTimerEvent();
        }
        else if (mMousePressTimer && mClick)
        {
            mClick = false;
            mouseTimerEvent();
        }

        mMousePress = false;
    }
}

void TQScrollArea::scrollContentsBy(int dx, int dy)
{
    QScrollArea::scrollContentsBy(dx, dy);      // First let the original class do it's job.

    QScrollBar *sbar = nullptr;

    if (mVertical)
        sbar = verticalScrollBar();
    else
        sbar = horizontalScrollBar();

    if (sbar)
        mActPosition = sbar->value();
}

void TQScrollArea::mouseTimerEvent()
{
    if (mMousePressTimer)
        mMousePressTimer->stop();

    if (!mMousePress || mMouseScroll)
        return;

    if (mMain)
    {
        QWidget *w = nullptr;

        if (mVertical)
            w = mMain->childAt(mLastMousePress.x(), mActPosition + mLastMousePress.y());
        else
            w = mMain->childAt(mActPosition + mLastMousePress.x(), mLastMousePress.y());

        if (w)
        {
            QString obname = w->objectName();
            // Extract the handle out of the object name
            int pos = obname.indexOf("_");

            if (pos < 0)
                return;

            QString qshandle = obname.mid(pos + 1);
            pos = qshandle.indexOf(":");

            if (pos < 0)
                return;

            int high = qshandle.mid(0, pos).toInt();
            int low = qshandle.mid(pos + 1).toInt();
            ulong handle = (ulong)(((high << 16) & 0xffff0000) | low);
            MSG_DEBUG("Calling signal with handle " << high << ":" << low << ": STATE=" << (mClick ? "PRESSED" : "RELEASED"));
            emit objectClicked(handle, mClick);
        }
    }
}
