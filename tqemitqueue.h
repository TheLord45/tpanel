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

typedef enum _EMIT_TYPE_t
{
    ET_NONE,
    ET_BUTTON,
    ET_PAGE,
    ET_SUBPAGE,
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
        int pixline{0};
        int left{0};
        int top{0};
        int width{0};
        int height{0};
        int frame{0};
        unsigned char *image{nullptr};
        size_t size{0};
        size_t rowBytes{0};
        ulong color{0};
        ANIMATION_t animate;
        std::string url;
        std::string user;
        std::string pw;
        Button::TButton *button{nullptr};
        Button::BITMAP_t bm;

        TQEmitQueue *next{nullptr};

    private:
        bool mDropped{false};
};

class TQManageQueue
{
    public:
        TQManageQueue() {}
        ~TQManageQueue();

        void addButton(ulong handle, ulong parent, unsigned char *buffer, int pixline, int left, int top, int width, int height);
        void addPage(ulong handle, int width, int height);
        void addSubPage(ulong handle, int left, int top, int width, int height, ANIMATION_t anim);
        void addBackground(ulong handle, unsigned char *image, size_t size, size_t rowBytes, int width, int height, ulong color);
        void addVideo(ulong handle, ulong parent, ulong left, ulong top, ulong width, ulong height, std::string url, std::string user, std::string pw);
        void addInText(ulong handle, Button::TButton *button, Button::BITMAP_t bm, int frame);
        void addListBox(Button::TButton *button, Button::BITMAP_t bm, int frame);

        bool getButton(ulong *handle, ulong *parent, unsigned char **buffer, int *pixline, int *left, int *top, int *width, int *height);
        bool getPage(ulong *handle, int *width, int *height);
        bool getSubPage(ulong *handle, int *left, int *top, int *width, int *height, ANIMATION_t *anim);
        bool getBackground(ulong *handle, unsigned char **image, size_t *size, size_t *rowBytes, int *width, int *height, ulong *color);
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

