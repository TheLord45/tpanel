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

        disconnect(mMousePressTimer, &QTimer::timeout, this, &TQScrollArea::mouseTimerEvent);
        delete mMousePressTimer;
        mMousePressTimer = nullptr;
    }
}

void TQScrollArea::init()
{
    DECL_TRACER("TQScrollArea::init()");

    QScrollArea::setParent(mParent);
    QScrollArea::setViewportMargins(0, 0, 0, 0);

    if (mWidth > 0 || mHeight > 0)
        QScrollArea::setFixedSize(mWidth, mHeight);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Set a transparent background
    QPalette palette(this->palette());
    palette.setColor(QPalette::Window, Qt::transparent);
    setPalette(palette);

    if (!mMain)
        mMain = new QWidget(this);

    mMain->move(0, 0);
    mMain->setContentsMargins(0, 0, 0, 0);

    if (mWidth && mHeight)
        setFixedSize(mWidth, mHeight);

    setWidget(mMain);

    if (mVertical && !mVLayout)
    {
        mVLayout = new QVBoxLayout(mMain);
        mVLayout->setSpacing(0);
        mVLayout->setContentsMargins(0, 0, 0, 0);
    }
    else if (!mVertical && !mHLayout)
    {
        mHLayout = new QHBoxLayout(mMain);
        mHLayout->setSpacing(0);
        mHLayout->setContentsMargins(0, 0, 0, 0);
    }


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
 * @brief TQScrollArea::setScrollbar
 * Makes the scrollbar visible or invisible.
 *
 * @param sb    TRUE: The scrollbar is set to visible. It depends on the type
 *              of scroll area. On vertical areas the bottom scrollbar may
 *              become visible and on horizontal areas the right one.
 */
void TQScrollArea::setScrollbar(bool sb)
{
    DECL_TRACER("TQScrollArea::setScrollbar(bool sb)");

    if (sb == mScrollbar)
        return;

    mScrollbar = sb;

    if (sb)
    {
        if (mVertical)
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        else
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    }
    else
    {
        if (mVertical)
            setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        else
            setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

/**
 * @brief TQScrollArea::setScrollbarOffset
 * Positions the scrollbar to the \b offset position. On a vertical scroll
 * area the offset is alway the top visible pixel line and on horizontal
 * scroll areas it is the left visible pixel.
 *
 * @param offset    The offset position. If this is less then 0 or grater then
 *                  the whole size, the value is ignored.
 */
void TQScrollArea::setScrollbarOffset(int offset)
{
    DECL_TRACER("TQScrollArea::setScrollbarOffset(int offset)");

    if (!mScrollbar)
        return;

    if (offset <= 0)
        mScrollbarOffset = 0;
    else
        mScrollbarOffset = scale(offset);

    QScrollBar *sbar = nullptr;

    if (mVertical)
        sbar = verticalScrollBar();
    else
        sbar = horizontalScrollBar();

    if (sbar)
    {
        if (mScrollbarOffset > (mVertical ? sbar->pos().y() : sbar->pos().x()))
            mScrollbarOffset = (mVertical ? sbar->pos().y() : sbar->pos().x());

        sbar->setSliderPosition(mScrollbarOffset);
    }
}

/**
 * @brief TQScrollArea::setAnchor
 * Sets the anchor position. This can be left, center or right for horizontal
 * scroll areas or top, center or bottom for vertical.
 * The anchor position is the one the items scroll to if they should appear at
 * this position. The center position is the default one.
 *
 * @param position  Defines the anchor position.
 */
void TQScrollArea::setAnchor(Button::SUBVIEW_POSITION_t position)
{
    DECL_TRACER("TQScrollArea::setAnchor(Button::SUBVIEW_POSITION_t position)");

    mPosition = position;

    if (!mWrapItems)
    {
        QWidget *w = mMain->childAt(mWidth / 2, mHeight / 2);

        if (w)
            setPosition(w, 0);
    }
    else
        setPosition();
}

/**
 * @brief TQScrollArea::show
 * Shows all items in the scroll area. If some were invisible or not enabled,
 * they are visible and enabled. Depending on the set anchor position the
 * whole area is set to it. This means for example, that the center item is
 * moved into the view area if the anchor position is set to SVP_CENTER.
 */
void TQScrollArea::show()
{
    DECL_TRACER("TQScrollArea::show()");

    if (!QScrollArea::isEnabled())
        QScrollArea::setEnabled(true);

    QScrollArea::show();

    if (mMain)
    {
        if (!mMain->isEnabled())
            mMain->setEnabled(true);

        if (!mMain->isVisible())
            mMain->show();
    }

    if (mWrapItems)
        setPosition();
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

    if (s < 0 || s > 99 || mSpace == s)
        return;

    mSpace = s;
    refresh();
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

    _clearAllItems();
    resetSlider();
    mItems.push_back(subViewItemToItem(item));
    _addItems(mItems, true);
}

/**
 * @brief TQScrollArea::addItems
 * Adds one or more items to the scroll area and sizes the scroll area large
 * enough to hold all items. It calculates also the space between the items.
 * If there were already some items visible, they are deleted first. Then they
 * are displayed again.
 *
 * @param items     A list of items to be displayed
 */
void TQScrollArea::addItems(std::vector<PGSUBVIEWITEM_T>& items)
{
    DECL_TRACER("TQScrollArea::addItems(std::vector<PGSUBVIEWITEM_T>& items)");

    if (items.empty())
        return;

    _clearAllItems();
    resetSlider();
    mItems.clear();

    vector<PGSUBVIEWITEM_T>::iterator iter;

    for (iter = items.begin(); iter != items.end(); ++iter)
        mItems.push_back(subViewItemToItem(*iter));

    _addItems(mItems, true);
}

void TQScrollArea::_addItems(std::vector<_ITEMS_T>& items, bool intern)
{
//    DECL_TRACER("_addItems(std::vector<ITEMS_T>& items, bool intern)");

    mWrapItems = items[0].wrap;     // Endless scroll

    if (!intern)
        mItems = items;             // Store the items to the internal vector array

    if (mVertical)
        mTotalHeight = 0;
    else
        mTotalWidth = 0;

    int total = 0;                  // The total width or height
    vector<_ITEMS_T>::iterator iter;
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

//        int num = 0;

        for (iter = items.begin(); iter != items.end(); ++iter)
        {
            if (!mVertical)
                mTotalWidth += iter->width;
            else
                mTotalHeight += iter->height;

//            num++;
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
    }

//    MSG_DEBUG("Number of items: " << items.size());

    if (mSpace > 0)
    {
        int space = (int)((double)total / 100.0 * (double)mSpace);

        if (space > 0 && mVertical && mVLayout && mMain)
        {
            int newHeight = space + mTotalHeight;
            mMain->setFixedHeight(newHeight);
//            MSG_DEBUG("Calculated space: " << space << " (" << mSpace << "%). Total height: " << newHeight << ", Old total height: " << mTotalHeight);
            mTotalHeight = newHeight;
        }
        else if (space > 0 && !mVertical && mHLayout && mMain)
        {
            int newWidth = space + mTotalWidth;
            mMain->setFixedWidth(newWidth);
//            MSG_DEBUG("Calculated space: " << space << " (" << mSpace << "%). Total width: " << newWidth << ", Old total width: " << mTotalWidth);
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
            setAtom(*itAtom, label);
        }

        iter->item = item;

        if (mVertical && mVLayout)
            mVLayout->addWidget(item);
        else if (!mVertical && mHLayout)
            mHLayout->addWidget(item);
        else
        {
            MSG_ERROR("Layout not initialized!");
        }
    }

    if (mOldActPosition > 0)
    {
        MSG_DEBUG("Setting position to old value " << mOldActPosition);
        resetSlider(mOldActPosition);
        mOldActPosition = 0;
    }
}

/**
 * @brief TQScrollArea::updateItem
 * Updates one item.
 *
 * @param item  The item to update.
 */
void TQScrollArea::updateItem(PGSUBVIEWITEM_T& item)
{
    DECL_TRACER("TQScrollArea::updateItem(PGSUBVIEWITEM_T& item)");

    if (mItems.empty())
        return;

    vector<_ITEMS_T>::iterator iter;

    for (iter = mItems.begin(); iter != mItems.end(); ++iter)
    {
        if (iter->handle == item.handle)
        {
            iter->bgcolor = item.bgcolor;
            iter->bounding = item.bounding;
            iter->image = item.image;
            iter->atoms = item.atoms;

            if (!iter->atoms.empty())
            {
                QObjectList list = iter->item->children();
                QList<QObject *>::Iterator obit;
                vector<PGSUBVIEWATOM_T>::iterator atit;

                for (atit = iter->atoms.begin(); atit != iter->atoms.end(); ++atit)
                {
                    for (obit = list.begin(); obit != list.end(); ++obit)
                    {
                        QObject *o = *obit;
                        QString obname = o->objectName();
                        ulong atHandle = extractHandle(obname.toStdString());

                        if (atit->handle == atHandle && obname.startsWith("Label_"))
                        {
                            QLabel *lbl = dynamic_cast<QLabel *>(o);
                            setAtom(*atit, lbl);
                            break;
                        }
                    }
                }

                iter->item->show();
            }

            break;
        }
    }
}

void TQScrollArea::showItem(ulong handle, int position)
{
    DECL_TRACER("TQScrollArea::showItem(ulong handle, int position)");

    if (mItems.empty())
        return;

    vector<_ITEMS_T>::iterator iter;

    for (iter = mItems.begin(); iter != mItems.end(); ++iter)
    {
        if (!iter->item)
            continue;

        if (iter->handle == handle)
        {
            if (!iter->item->isVisible())
                iter->item->setVisible(true);

            setPosition(iter->item, position);
            break;
        }
    }
}

void TQScrollArea::toggleItem(ulong handle, int position)
{
    DECL_TRACER("TQScrollArea::toggleItem(ulong handle, int position)");

    if (mItems.empty())
        return;

    vector<_ITEMS_T>::iterator iter;

    for (iter = mItems.begin(); iter != mItems.end(); ++iter)
    {
        if (iter->handle == handle && iter->item)
        {
            if (iter->item->isVisible())
                iter->item->setVisible(false);
            else
            {
                iter->item->setVisible(true);
                setPosition(iter->item, position);
            }

            break;
        }
    }
}

void TQScrollArea::hideAllItems()
{
    DECL_TRACER("TQScrollArea::hideAllItems()");

    if (mItems.empty())
        return;

    resetSlider();
    vector<_ITEMS_T>::iterator iter;

    for (iter = mItems.begin(); iter != mItems.end(); ++iter)
    {
        if (iter->item)
            iter->item->setVisible(false);
    }
}

void TQScrollArea::hideItem(ulong handle)
{
    DECL_TRACER("TQScrollArea::hideItem(ulong handle)");

    if (mItems.empty())
        return;

    vector<_ITEMS_T>::iterator iter;

    for (iter = mItems.begin(); iter != mItems.end(); ++iter)
    {
        if (iter->handle == handle && iter->item)
        {
            iter->item->setVisible(false);
            return;
        }
    }
}

int TQScrollArea::scale(int value)
{
//    DECL_TRACER("TQScrollArea::scale(int value)");

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

void TQScrollArea::setAtom(PGSUBVIEWATOM_T& atom, QLabel *label)
{
//    DECL_TRACER("TQScrollArea::setAtom(PGSUBVIEWATOM_T& atom, QLabel *label)");

    if (!label)
        return;

    int scaWidth = scale(atom.width);
    int scaHeight = scale(atom.height);
    QColor bg(qRgba(atom.bgcolor.red, atom.bgcolor.green, atom.bgcolor.blue, atom.bgcolor.alpha));

    if (atom.image.isValid())
    {
        QPixmap pix(scaWidth, scaHeight);

        if (atom.bgcolor.alpha == 0)
            pix.fill(Qt::transparent);
        else
            pix.fill(bg);

        QImage img(atom.image.getBitmap(), atom.image.getWidth(), atom.image.getHeight(), atom.image.getPixline(), QImage::Format_ARGB32);
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

        if (atom.bgcolor.alpha == 0)
            palette.setColor(QPalette::Window, Qt::transparent);
        else
            palette.setColor(QPalette::Window, bg);

        label->setPalette(palette);
    }
}

void TQScrollArea::refresh()
{
    DECL_TRACER("TQScrollArea::refresh()");

    if (!mMain || mItems.empty())
        return;

    resetSlider();
    _clearAllItems();
    _addItems(mItems, true);
}

void TQScrollArea::setPosition()
{
    DECL_TRACER("TQScrollArea::setPosition()");

    if (mItems.empty())
        return;

    QWidget *anchor = nullptr;
    size_t pos = 0;
    MSG_DEBUG("Wrap items: " << (mWrapItems ? "TRUE" : "FALSE") << ", number items: " << mItems.size());

    switch (mPosition)
    {
        case Button::SVP_LEFT_TOP:
            anchor = mItems[0].item;
        break;

        case Button::SVP_CENTER:
            pos = mItems.size() / 2;
            anchor = mItems[pos].item;
        break;

        case Button::SVP_RIGHT_BOTTOM:
            pos = mItems.size() - 1;
            anchor = mItems[pos].item;
        break;
    }

    if (anchor)
    {
        QRect geom = anchor->geometry();
        bool makeVisible = false;

        if (mVertical)
        {
            if ((geom.y() + geom.height()) < mActPosition ||
                    geom.y() > (mActPosition + mHeight))
                makeVisible = true;
        }
        else
        {
            if ((geom.x() + geom.width()) < mActPosition ||
                    geom.x() > (mActPosition + mWidth))
                makeVisible = true;
        }

        ulong handle = extractHandle(anchor->objectName().toStdString());

        if (makeVisible)
        {
            ensureWidgetVisible(anchor);
            MSG_DEBUG("Item number " << pos << " (" << handleToString(handle) << ") was moved to position.");
        }
        else
        {
            MSG_DEBUG("Item number " << pos << " (" << handleToString(handle) << ") is already at visible position.");
        }
    }
}

void TQScrollArea::setPosition(QWidget* w, int position)
{
    DECL_TRACER("TQScrollArea::setPosition(QWidget* w, int position)");

    int defPosX = 50;
    int defPosY = 50;

    if (position > 0 && position < 65535)
    {
        if (mVertical)
        {
            defPosY = position;
            defPosX = 0;
        }
        else
        {
            defPosY = 0;
            defPosX = position;
        }

        ensureWidgetVisible(w, defPosX, defPosY);
    }
    else if (mPosition == Button::SVP_LEFT_TOP)
    {
        if (mVertical)
        {
            int top = w->geometry().y();
            QScrollBar *bar = verticalScrollBar();

            if (bar)
                bar->setSliderPosition(top);
        }
        else
        {
            int left = w->geometry().x();
            QScrollBar *bar = horizontalScrollBar();

            if (bar)
                bar->setSliderPosition(left);
        }
    }
    else if (mPosition == Button::SVP_CENTER)
    {
        if (mVertical)
        {
            int topMargin = (mHeight - w->geometry().height()) / 2;
            int top = w->geometry().y() - topMargin;
            QScrollBar *bar = verticalScrollBar();

            if (bar)
                bar->setSliderPosition(top);
        }
        else
        {
            int leftMargin = (mWidth - w->geometry().width()) / 2;
            int left = w->geometry().x() - leftMargin;
            QScrollBar *bar = horizontalScrollBar();

            if (bar)
                bar->setSliderPosition(left);
        }
    }
    else if (mPosition == Button::SVP_RIGHT_BOTTOM)
    {
        if (mVertical)
        {
            int bottom = w->geometry().y() + w->geometry().height();
            QScrollBar *bar = verticalScrollBar();

            if (bar)
                bar->setSliderPosition(bottom);
        }
        else
        {
            int right = w->geometry().x() + w->geometry().width();
            QScrollBar *bar = horizontalScrollBar();

            if (bar)
                bar->setSliderPosition(right);
        }
    }
}

TQScrollArea::_ITEMS_T TQScrollArea::subViewItemToItem(PGSUBVIEWITEM_T& item)
{
//    DECL_TRACER("TQScrollArea::subViewItemToItem(PGSUBVIEWITEM_T& item)");

    _ITEMS_T it;
    it.handle = item.handle;
    it.parent = item.parent;
    it.width = item.width;
    it.height = item.height;
    it.bgcolor = item.bgcolor;
    it.bounding = item.bounding;
    it.image = item.image;
    it.position = item.position;
    it.scrollbar = item.scrollbar;
    it.scrollbarOffset = item.scrollbarOffset;
    it.wrap = item.wrap;
    it.atoms = item.atoms;

    return it;
}

void TQScrollArea::_clearAllItems()
{
//    DECL_TRACER("TQScrollArea::_clearAllItems()");

    if (mItems.empty())
        return;

    vector<_ITEMS_T>::iterator clIter;

    for (clIter = mItems.begin(); clIter != mItems.end(); ++clIter)
    {
        if (clIter->item)
        {
            if (mVertical)
            {
                if (mVLayout)
                    mVLayout->removeWidget(clIter->item);
            }
            else
            {
                if (mHLayout)
                    mHLayout->removeWidget(clIter->item);
            }

            clIter->item->close();
            clIter->item = nullptr;
        }
    }
}

void TQScrollArea::resetSlider(int position)
{
    DECL_TRACER("TQScrollArea::resetSlider(int position)");

    if (mActPosition <= 0)
        return;

    QScrollBar *sbar = nullptr;

    if (mVertical)
        sbar = verticalScrollBar();
    else
        sbar = horizontalScrollBar();

    if (sbar)
    {
        mOldActPosition = sbar->value();
        sbar->setSliderPosition(position);
    }
    else
        mOldActPosition = mActPosition;
}

/*****************************************************************************
 * Signals and overwritten functions start here
 *****************************************************************************/

void TQScrollArea::scrollContentsBy(int dx, int dy)
{
    DECL_TRACER("TQScrollArea::scrollContentsBy(int dx, int dy)");

    QScrollArea::scrollContentsBy(dx, dy);      // First let the original class do it's job.
    QScrollBar *sbar = nullptr;

    if (mVertical)
        sbar = verticalScrollBar();
    else
        sbar = horizontalScrollBar();

    if (sbar)
    {
        mActPosition = sbar->value();
        MSG_DEBUG("Actual slider position: " << mActPosition);

        if (mScrollbar && mScrollbarOffset > 0 && mActPosition < mScrollbarOffset)
        {
            sbar->setSliderPosition(mScrollbarOffset);
            mActPosition = sbar->value();
        }
    }
}

void TQScrollArea::mouseMoveEvent(QMouseEvent* event)
{
    DECL_TRACER("TQScrollArea::mouseMoveEvent(QMouseEvent* event)");

    mMousePress = false;
    mMouseScroll = true;
    mDoMouseEvent = false;

    if (mMousePressTimer && mMousePressTimer->isActive())
        mMousePressTimer->stop();

    int move = 0;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    MSG_DEBUG("Scroll event at " << event->pos().x() << "x" << event->pos().y() << ", old point at " << mOldPoint.x() << "x" << mOldPoint.y());
#else
    MSG_DEBUG("Scroll event at " << event->position().x() << "x" << event->position().y() << ", old point at " << mOldPoint.x() << "x" << mOldPoint.y());
#endif
    if (mVertical)
    {
        if (mOldPoint.y() != 0)
        {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            move = event->pos().y() - mOldPoint.y();
#else
            move = event->position().y() - mOldPoint.y();
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
            move = event->pos().x() - mOldPoint.x();
#else
            move = event->position().x() - mOldPoint.x();
#endif
            QScrollBar *bar = horizontalScrollBar();

            if (bar)
            {
                int value = bar->value();
                int newValue = value + (move * -1);

                if (newValue >= 0 && newValue != value)
                    bar->setValue(newValue);
            }
        }
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    mOldPoint = event->pos();
#else
    mOldPoint = event->position();
#endif
}

void TQScrollArea::mousePressEvent(QMouseEvent* event)
{
    DECL_TRACER("TQScrollArea::mousePressEvent(QMouseEvent* event)");

    if (!event || mMouseScroll || event->button() != Qt::LeftButton)
        return;

    mMousePress = true;
    mOldPoint.setX(0.0);
    mOldPoint.setY(0.0);

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    MSG_DEBUG("Mouse press event at " << x << " x " << y << " // " << event->globalX() << " x " << event->globalY());
#else
    MSG_DEBUG("Mouse press event at " << x << " x " << y << " // " << event->globalPosition().x() << " x " << event->globalPosition().y());
#endif
    /*
        * Here we're starting a timer with 200 ms. If after this time the
        * mouse button is still pressed and no scroll event was detected,
        * then we've a real click.
        * In case of a real click the method mouseTimerEvent() will call a
        * signal to inform the parent about the click.
        */
    mClick = true;  // This means PRESSED state
    mMouseTmEventActive = true;
    mDoMouseEvent = true;
    QTimer::singleShot(200, this, &TQScrollArea::mouseTimerEvent);
}

void TQScrollArea::mouseReleaseEvent(QMouseEvent* event)
{
    DECL_TRACER("TQScrollArea::mouseReleaseEvent(QMouseEvent* event)");

    if (!event || event->button() != Qt::LeftButton)
        return;

    mDoMouseEvent = false;

    if (mMouseTmEventActive)
    {
        if (!mMouseScroll)
        {
            mClick = true;      // This means PRESSED state
            doMouseEvent();
            mClick = false;     // This means RELEASED state
            doMouseEvent();
        }
    }
    else if (!mMouseScroll && mClick)
    {
        mClick = false;         // This means RELEASED state
        doMouseEvent();
    }

    mMousePress = false;
    mMouseScroll = false;
    mOldPoint.setX(0.0);
    mOldPoint.setY(0.0);
}

void TQScrollArea::doMouseEvent()
{
    DECL_TRACER("TQScrollArea::doMouseEvent()");

    if (!mMousePress || mMouseScroll || !mMain)
        return;

    QWidget *w = nullptr;

    if (mVertical)
        w = mMain->childAt(mLastMousePress.x(), mActPosition + mLastMousePress.y());
    else
        w = mMain->childAt(mActPosition + mLastMousePress.x(), mLastMousePress.y());

    if (!w)
        return;

    QString obname = w->objectName();
    ulong handle = extractHandle(obname.toStdString());

    if (!handle)
        return;

    // We must make sure the found object is not marked as pass through.
    // Because of this we'll scan the items for the handle and if we
    // find that it is marked as pass through we must look for another
    // one on the same position. If there is none, the click is ignored.
    //
    // Find the object in our list
    vector<_ITEMS_T>::iterator iter;
    QRect rect;
    bool call = true;

    for (iter = mItems.begin(); iter != mItems.end(); ++iter)
    {
        if (iter->handle == handle)     // Handle found?
        {                               // Yes, then ...
            if (iter->bounding == "passThru")   // Item marked as pass through?
            {                                   // Yes, then start to search for another item
                rect = w->rect();
                call = false;
                // Walk through the childs to find another one on the
                // clicked position.
                QObjectList ol = mMain->children(); // Get list of all objects
                QList<QObject *>::iterator obiter;  // Define an iterator
                // Loop through all objects
                for (obiter = ol.begin(); obiter != ol.end(); ++obiter)
                {
                    QObject *object = *obiter;

                    if (object->objectName() != obname && object->objectName().startsWith("Label_"))    // Have we found a QLabel object?
                    {                                                                                   // Yes, then test it's position
                        QLabel *lb = dynamic_cast<QLabel *>(object);    // Cast the object to a QLabel

                        if (lb->rect().contains(mLastMousePress))       // Is the QLabel under the mouse coordinates?
                        {                                               // Yes, then select it.
                            ulong h = extractHandle(lb->objectName().toStdString());  // Get the handle

                            if (!h)
                                break;

                            handle = h;
                            // Reset the main loop
                            iter = mItems.begin();
                            break;
                        }
                    }
                }
            }
            else
            {
                call = true;
                break;
            }
        }
    }

    if (call)
    {
        MSG_DEBUG("Calling signal with handle " << (handle >> 16 & 0x0000ffff) << ":" << (handle & 0x0000ffff) << ": STATE=" << (mClick ? "PRESSED" : "RELEASED"));
        emit objectClicked(handle, mClick);
        mClick = false;
    }
}

void TQScrollArea::mouseTimerEvent()
{
    DECL_TRACER("TQScrollArea::mouseTimerEvent()");

    if (!mDoMouseEvent)
        return;

    mDoMouseEvent = false;

    if (mClick)         // Only if PRESSED
        doMouseEvent();

    mMouseTmEventActive = false;
}

