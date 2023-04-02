/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef __QTEMITQUEUE_H__
#define __QTEMITQUEUE_H__

#include "tsubpage.h"
#include "tbutton.h"
#include "tbitmap.h"

typedef enum _EMIT_TYPE_t
{
    ET_NONE,
    ET_BUTTON,
    ET_PAGE,
    ET_SUBPAGE,
    ET_SUBVIEW,
    ET_BACKGROUND,
    ET_DROPPAGE,
    ET_DROPSUBPAGE,
    ET_VIDEO,
    ET_INTEXT,
    ET_LISTBOX,
    ET_SURFRESET
}_EMIT_TYPE_t;

class TQEmitQueue
{
    public:
        void markDropped() { mDropped = true; }
        bool isDropped() { return mDropped; }

        _EMIT_TYPE_t etype{ET_NONE};
        ulong handle{0};
        ulong parent{0};
        unsigned char *buffer{nullptr};
        TBitmap bitmap;
        TColor::COLOR_T amxColor;
        int pixline{0};
        int left{0};
        int top{0};
        int width{0};
        int height{0};
        int frame{0};
        int space{0};
        unsigned char *image{nullptr};
        size_t size{0};
        size_t rowBytes{0};
        ulong color{0};
        int opacity{255};
        ANIMATION_t animate;
        std::string url;
        std::string user;
        std::string pw;
        Button::TButton *button{nullptr};
        Button::BITMAP_t bm;
        bool view{false};
        bool vertical{false};

        TQEmitQueue *next{nullptr};

    private:
        bool mDropped{false};
};

class TQManageQueue
{
    public:
        TQManageQueue() {}
        ~TQManageQueue();

        void addButton(ulong handle, ulong parent, TBitmap& buffer, int left, int top, int width, int height, bool passthrough);
        void addViewButton(ulong handle, ulong parent, bool vertical, TBitmap& buffer, int left, int top, int width, int height, int space, TColor::COLOR_T fillColor);
        void addPage(ulong handle, int width, int height);
#ifdef _OPAQUE_SKIA_
        void addSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t anim);
        void addBackground(ulong handle, TBitmap& image, int width, int height, ulong color);
#else
        void addSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t anim, int opacity=255);
        void addBackground(ulong handle, TBitmap& image, size_t size, int width, int height, ulong color, int opacity=255);
#endif
        void addVideo(ulong handle, ulong parent, int left, int top, int width, int height, std::string url, std::string user, std::string pw);
        void addInText(ulong handle, Button::TButton *button, Button::BITMAP_t bm, int frame);
        void addListBox(Button::TButton *button, Button::BITMAP_t bm, int frame);

        bool getButton(ulong *handle, ulong *parent, TBitmap *buffer, int *left, int *top, int *width, int *height, bool *passthrough);
        bool getViewButton(ulong *handle, ulong *parent, bool *vertical, TBitmap *buffer, int *left, int *top, int *width, int *height, int *space, TColor::COLOR_T *fillColor);
        bool getPage(ulong *handle, int *width, int *height);
#ifdef _OPAQUE_SKIA_
        bool getSubPage(ulong *handle, ulong *parent, int *left, int *top, int *width, int *height, ANIMATION_t *anim);
        bool getBackground(ulong *handle, TBitmap *image, int *width, int *height, ulong *color);
#else
        bool getSubPage(ulong *handle, ulong *parent, int *left, int *top, int *width, int *height, ANIMATION_t *anim, int *opacity=nullptr);
        bool getBackground(ulong *handle, TBitmap **image, int *width, int *height, ulong *color, int *opacity=nullptr);
#endif
        bool getVideo(ulong *handle, ulong *parent, int *left, int *top, int *width, int *height, std::string *url, std::string *user, std::string *pw);
        bool getInText(ulong *handle, Button::TButton **button, Button::BITMAP_t *bm, int *frame);

        _EMIT_TYPE_t getNextType();
        bool isDeleted();
        bool dropHandle(ulong handle);
        bool dropType(_EMIT_TYPE_t t);
        void markDrop(ulong handle);

    private:
        TQEmitQueue *addEntity(_EMIT_TYPE_t etype);
        void removeDuplicates();

        TQEmitQueue *mEmitQueue{nullptr};
};

#endif

