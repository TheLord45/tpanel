/*
 * Copyright (C) 2023 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TQSCROLLAREA_H__
#define __TQSCROLLAREA_H__

#include <QScrollArea>

#include <vector>

#include "tpagemanager.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QLabel;
class QSize;
class QPixmap;
class QColor;
class QVBoxLayout;
class QHBoxLayout;
class QPoint;
QT_END_NAMESPACE

class TQScrollArea : public QScrollArea
{
    Q_OBJECT

    public:
        TQScrollArea();
        TQScrollArea(QWidget *parent);
        TQScrollArea(QWidget *parent, int w, int h, bool vertical = false);
        TQScrollArea(QWidget *parent, const QSize& size, bool vertical = false);
        ~TQScrollArea();

        void setObjectName(const QString& name);
        void setSize(int w, int h);
        void setSize(QSize size);
        QSize getSize();
        void setScrollbar(bool sb);
        void setScrollbarOffset(int offset);
        void setAnchor(Button::SUBVIEW_POSITION_t position);
        void show();
        void setBackgroundImage(const QPixmap& pix);
        void setBackGroundColor(QColor color);
        void setSpace(int s);
        int getSpace() { return mSpace; }
        void addItem(PGSUBVIEWITEM_T& item);
        void addItems(std::vector<PGSUBVIEWITEM_T>& items);
        void updateItem(PGSUBVIEWITEM_T& item);
        void showItem(ulong handle, int position);
        void toggleItem(ulong handle, int position);
        void hideAllItems();
        void hideItem(ulong handle);

        int getWidth() { return mWidth; }
        int getHeight() { return mHeight; }
        void setTotalWidth(int w);
        int getTotalWidth() { return mTotalWidth; }
        void setTotalHeight(int h);
        int getTotalHeight() { return mTotalHeight; }
        void setTotalSize(int w, int h);
        void setTotalSize(QSize& size);
        void setScaleFactor(const double& factor);
        void setWrapItems(bool wrap) { mWrapItems = wrap; }

    signals:
        void objectClicked(ulong handle, bool pressed);

    protected:
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void scrollContentsBy(int dx, int dy) override;

    private:
        typedef struct _ITEMS_T
        {
            ulong handle{0};
            ulong parent{0};
            int width{0};
            int height{0};
            bool scrollbar{false};
            int scrollbarOffset{0};
            Button::SUBVIEW_POSITION_t position{Button::SVP_CENTER};
            bool wrap{false};
            TColor::COLOR_T bgcolor;
            TBitmap image;
            std::string bounding;
            QWidget *item{nullptr};
            bool show{true};
            bool dynamic{false};
            bool visible{false};
            std::vector<PGSUBVIEWATOM_T> atoms;

            void clear()
            {
                handle = parent = 0;
                width = height = 0;
                bgcolor.alpha = bgcolor.blue = bgcolor.green = bgcolor.red = 0;
                scrollbar = wrap = dynamic = visible = false;
                scrollbarOffset = 0;
                position = Button::SVP_CENTER;
                image.clear();
                bounding.clear();
                show = true;
                atoms.clear();

                if (item)
                    item->close();

                item = nullptr;
            }
        }_ITEMS_T;

        void init();
        int scale(int value);
        void setAtom(PGSUBVIEWATOM_T& atom, QLabel *label);
        void refresh();
        void setPosition();
        void setPosition(QWidget *w, int position);
        void mouseTimerEvent();
        void _addItems(std::vector<_ITEMS_T>& items, bool intern=false);
        void _clearAllItems();
        _ITEMS_T subViewItemToItem(PGSUBVIEWITEM_T& item);
        void resetSlider(int position=0);
        void doMouseEvent();
        int calcSize(int total=0);              // Calculates the size of all (visible) items
        void applySize(int size);
        int getVisibleItems();
        void addExtraSpace(int baseW, int baseH);// Adds extra space to scroll area to make it look better
        int calcSpace(int itemSize, bool apply=false);// Calculates the pixels for spacing of the given item size.

        QWidget *mParent{nullptr};              //!< The parent of this object. This is set to QScrollArea.
        QWidget *mMain{nullptr};                //!< The widget containing the items. This is the whole scroll area
        QHBoxLayout *mHLayout{nullptr};         //!< If mVertical == FALSE then this layout is used
        QVBoxLayout *mVLayout{nullptr};         //!< If mVertical == TRUE then this layout is used
        int mWidth{0};                          //!< Width of visible part of scroll area (QScrollArea)
        int mHeight{0};                         //!< Height of visible part of scroll area (QScrollArea)
        int mTotalWidth{0};                     //!< Total width of scroll area (mMain)
        int mTotalHeight{0};                    //!< Total height of of scroll area (mMain)
        bool mVertical{false};                  //!< Direction
        int mSpace{0};                          //!< Optional: The space between the items in percent
        bool mMousePress{false};                //!< Internal: TRUE when the mouse was pressed
        bool mClick{false};                     //!< TRUE on mouse press, FALSE on mouse release
        bool mMouseScroll{false};               //!< Internal: TRUE if scrolling was detected. This prevents a button press from beeing one.
        QPointF mOldPoint;                      //!< Internal: The last point where the mouse was pressed
        double mScaleFactor{1.0};               //!< If != 1.0 and > 0.0 then mTotalHeight and mTotalWidth are scaled as well as the size of each item
        std::vector<_ITEMS_T> mItems;           //!< The list of items
        int mActPosition{0};                    //!< The absolute top/left (depends on mVertical) position in the scrolling area.
        int mOldActPosition{0};                 //!< Used to store the actual slider position temporary.
        QTimer *mMousePressTimer{nullptr};      //!< Internal: Used to distinguish between a real mouse click and a mouse move.
        QPoint mLastMousePress;                 //!< Internal: The absolute point of the last mouse press.
        Button::SUBVIEW_POSITION_t mPosition{Button::SVP_CENTER};   //!< Defines where the anchor should snap in
        bool mScrollbar{false};                 //!< TRUE = scrollbar is visible
        int mScrollbarOffset{0};                //!< Defines the offset of the scrollbar. Only valid if \b mScrollbar is TRUE.
        bool mWrapItems{false};                 //!< TRUE = The scroll area behaves like a wheel (not supported) and the item according to the anchor position is displayed.
        bool mMouseTmEventActive{false};        //!< TRUE = the mouse timer event is still running and will not accept calls.
        bool mDoMouseEvent{false};              //!< TRUE = The mouse timer event is valid.
};

#endif
