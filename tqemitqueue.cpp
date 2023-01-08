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

#include "tqemitqueue.h"
#include "terror.h"

TQManageQueue::~TQManageQueue()
{
    DECL_TRACER("TQManageQueue::~TQManageQueue()");

    TQEmitQueue *next, *p = mEmitQueue;
    next = nullptr;

    while (p)
    {
        next = p->next;
        delete p;
        p = next;
    }
}

#ifdef _OPAQUE_SKIA_
void TQManageQueue::addBackground(ulong handle, unsigned char* image, size_t size, size_t rowBytes, int width, int height, ulong color)
#else
void TQManageQueue::addBackground(ulong handle, unsigned char* image, size_t size, size_t rowBytes, int width, int height, ulong color, int opacity)
#endif
{
    DECL_TRACER("TQManageQueue::addBackground(ulong handle, unsigned char* image, size_t size, size_t rowBytes, int width, int height, ulong color)");

    TQEmitQueue *eq = addEntity(ET_BACKGROUND);

    if (!eq)
        return;

    eq->handle = handle;
    eq->image = image;
    eq->size = size;
    eq->rowBytes = rowBytes;
    eq->width = width;
    eq->height = height;
    eq->color = color;
#ifndef _OPAQUE_SKIA_
    eq->opacity = opacity;
#endif
    removeDuplicates();
}

#ifdef _OPAQUE_SKIA_
bool TQManageQueue::getBackground(ulong* handle, unsigned char **image, size_t* size, size_t* rowBytes, int* width, int* height, ulong* color)
#else
bool TQManageQueue::getBackground(ulong* handle, unsigned char **image, size_t* size, size_t* rowBytes, int* width, int* height, ulong* color, int *opacity)
#endif
{
    DECL_TRACER("TQManageQueue::getBackground(ulong* handle, unsigned char ** image, size_t* size, size_t* rowBytes, int* width, int* height, ulong* color)");

    TQEmitQueue *eq = mEmitQueue;

    while(eq)
    {
        if (eq->etype == ET_BACKGROUND)
        {
            *handle = eq->handle;
            *image = eq->image;
            *size = eq->size;
            *rowBytes = eq->rowBytes;
            *width = eq->width;
            *height = eq->height;
            *color = eq->color;
#ifndef _OPAQUE_SKIA_
            if (opacity)
                *opacity = eq->opacity;
#endif
            return true;
        }

        eq = eq->next;
    }

    return false;
}

void TQManageQueue::addButton(ulong handle, ulong parent, unsigned char* buffer, int pixline, int left, int top, int width, int height)
{
    DECL_TRACER("TQManageQueue::addButton(ulong handle, ulong parent, unsigned char* buffer, int pixline, int left, int top, int width, int height)");

    TQEmitQueue *eq = addEntity(ET_BUTTON);

    if (!eq)
        return;

    eq->handle = handle;
    eq->parent = parent;
    eq->buffer = buffer;
    eq->pixline = pixline;
    eq->left = left;
    eq->top = top;
    eq->width = width;
    eq->height = height;
    removeDuplicates();
}

bool TQManageQueue::getButton(ulong* handle, ulong* parent, unsigned char ** buffer, int* pixline, int* left, int* top, int* width, int* height)
{
    DECL_TRACER("TQManageQueue::getButton(ulong* handle, ulong* parent, unsigned char ** buffer, int* pixline, int* left, int* top, int* width, int* height)");

    TQEmitQueue *eq = mEmitQueue;

    while(eq)
    {
        if (eq->etype == ET_BUTTON)
        {
            *handle = eq->handle;
            *parent = eq->parent;
            *buffer = eq->buffer;
            *pixline = eq->pixline;
            *left = eq->left;
            *top = eq->top;
            *width = eq->width;
            *height = eq->height;
            return true;
        }

        eq = eq->next;
    }

    return false;
}

void TQManageQueue::addInText(ulong handle, Button::TButton* button, Button::BITMAP_t bm, int frame)
{
    DECL_TRACER("TQManageQueue::addInText(ulong handle, Button::TButton* button, Button::BITMAP_t bm, int frame)");

    TQEmitQueue *eq = addEntity(ET_INTEXT);

    if (!eq)
        return;

    eq->handle = handle;
    eq->button = button;
    eq->bm = bm;
    eq->frame = frame;
    removeDuplicates();
}

void TQManageQueue::addListBox(Button::TButton* button, Button::BITMAP_t bm, int frame)
{
    DECL_TRACER("TQManageQueue::addListBox(Button::TButton* button, Button::BITMAP_t bm, int frame)");

    TQEmitQueue *eq = addEntity(ET_LISTBOX);

    if (!eq)
        return;

    eq->handle = button->getHandle();
    eq->button = button;
    eq->bm = bm;
    eq->frame = frame;
    removeDuplicates();
}

bool TQManageQueue::getInText(ulong* handle, Button::TButton ** button, Button::BITMAP_t* bm, int *frame)
{
    DECL_TRACER("TQManageQueue::getInText(ulong* handle, Button::TButton ** button, Button::BITMAP_t* bm, int *frame)");

    TQEmitQueue *eq = mEmitQueue;

    while(eq)
    {
        if (eq->etype == ET_INTEXT)
        {
            *handle = eq->handle;
            *button = eq->button;
            *bm = eq->bm;
            *frame = eq->frame;
            return true;
        }
    }

    return false;
}

void TQManageQueue::addPage(ulong handle, int width, int height)
{
    DECL_TRACER("TQManageQueue::addPage(ulong handle, int width, int height)");

    TQEmitQueue *eq = addEntity(ET_PAGE);

    if (!eq)
        return;

    eq->handle = handle;
    eq->width = width;
    eq->height = height;
    removeDuplicates();
}

bool TQManageQueue::getPage(ulong* handle, int* width, int* height)
{
    DECL_TRACER("TQManageQueue::getPage(ulong* handle, int* width, int* height)");

    TQEmitQueue *eq = mEmitQueue;

    while(eq)
    {
        if (eq->etype == ET_PAGE)
        {
            *handle = eq->handle;
            *width = eq->width;
            *height = eq->height;
            return true;
        }
    }

    return false;
}

#ifdef _OPAQUE_SKIA_
void TQManageQueue::addSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t anim)
#else
void TQManageQueue::addSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t anim, int opacity)
#endif
{
    DECL_TRACER("TQManageQueue::addSubPage(ulong handle, ulong parent, int left, int top, int width, int height, ANIMATION_t anim)");

    TQEmitQueue *eq = addEntity(ET_SUBPAGE);

    if (!eq)
        return;

    eq->handle = handle;
    eq->parent = parent;
    eq->left = left;
    eq->top = top;
    eq->width = width;
    eq->height = height;
    eq->animate = anim;
#ifndef _OPAQUE_SKIA_
    eq->opacity = opacity;
#endif
    removeDuplicates();
}

#ifdef _OPAQUE_SKIA_
bool TQManageQueue::getSubPage(ulong* handle, ulong *parent, int* left, int* top, int* width, int* height, ANIMATION_t* anim)
#else
bool TQManageQueue::getSubPage(ulong* handle, ulong *parent, int* left, int* top, int* width, int* height, ANIMATION_t* anim, int *opacity)
#endif
{
    DECL_TRACER("TQManageQueue::getSubPage(ulong* handle, ulong *parent, int* left, int* top, int* width, int* height, ANIMATION_t* anim)");

    TQEmitQueue *eq = mEmitQueue;

    while(eq)
    {
        if (eq->etype == ET_SUBPAGE)
        {
            *handle = eq->handle;
            *parent = eq->parent;
            *left = eq->left;
            *top = eq->top;
            *width = eq->width;
            *height = eq->height;
            *anim = eq->animate;
#ifndef _OPAQUE_SKIA_
            if (opacity)
                *opacity = eq->opacity;
#endif
            return true;
        }

        eq = eq->next;
    }

    return false;
}

void TQManageQueue::addVideo(ulong handle, ulong parent, ulong left, ulong top, ulong width, ulong height, std::string url, std::string user, std::string pw)
{
    DECL_TRACER("TQManageQueue::addVideo(ulong handle, ulong parent, ulong left, ulong top, ulong width, ulong height, std::string url, std::string user, std::string pw)");

    TQEmitQueue *eq = addEntity(ET_VIDEO);

    if (!eq)
        return;

    eq->handle = handle;
    eq->parent = parent;
    eq->left = left;
    eq->top = top;
    eq->width = width;
    eq->height = height;
    eq->url = url;
    eq->user = user;
    eq->pw = pw;
    removeDuplicates();
}

bool TQManageQueue::getVideo(ulong* handle, ulong* parent, int* left, int* top, int* width, int* height, std::string* url, std::string* user, std::string* pw)
{
    DECL_TRACER("TQManageQueue::getVideo(ulong* handle, ulong* parent, int* left, int* top, int* width, int* height, std::string* url, std::string* user, std::string* pw)");

    TQEmitQueue *eq = mEmitQueue;

    while(eq)
    {
        if (eq->etype == ET_VIDEO)
        {
            *handle = eq->handle;
            *parent = eq->parent;
            *left = eq->left;
            *top = eq->top;
            *width = eq->width;
            *height = eq->height;
            *url = eq->url;
            *user = eq->user;
            *pw = eq->pw;
            return true;
        }

        eq = eq->next;
    }

    return false;
}

_EMIT_TYPE_t TQManageQueue::getNextType()
{
    DECL_TRACER("TQManageQueue::getNextType()");

    if (!mEmitQueue)
        return ET_NONE;

    return mEmitQueue->etype;
}

bool TQManageQueue::isDeleted()
{
    DECL_TRACER("TQManageQueue::isDeleted()");

    if (!mEmitQueue)
        return false;

    return mEmitQueue->isDropped();
}

/**
 * This deletes the first occurance of the \p handle in the queue.
 *
 * @param handle    The handle number.
 * @return If the \p handle was found in the queue TRUE is returned.
 */
bool TQManageQueue::dropHandle(ulong handle)
{
    DECL_TRACER("TQManageQueue::dropHandle(ulong handle)");

    TQEmitQueue *prev, *next, *p = mEmitQueue;
    prev = next = nullptr;

    while (p)
    {
        next = p->next;

        if (p->handle == handle)
        {
            if (prev)
                prev->next = next;

            if (p == mEmitQueue)
                mEmitQueue = next;

            delete p;
            return true;
        }

        prev = p;
        p = p->next;
    }

    return false;
}

/**
 * This deletes the first occurance of the \p t in the queue.
 *
 * @param t     The type.
 * @return If \p t was found in the queue TRUE is returned.
 */
bool TQManageQueue::dropType(_EMIT_TYPE_t t)
{
    DECL_TRACER("TQManageQueue::dropType(_EMIT_TYPE_t t)");

    TQEmitQueue *prev, *next, *p = mEmitQueue;
    prev = next = nullptr;

    while (p)
    {
        next = p->next;

        if (p->etype == t)
        {
            if (prev)
                prev->next = next;

            if (p == mEmitQueue)
                mEmitQueue = next;

            delete p;
            return true;
        }

        prev = p;
        p = p->next;
    }

    return false;
}

void TQManageQueue::markDrop(ulong handle)
{
    DECL_TRACER("TQManageQueue::markDrop(ulong handle)");

    TQEmitQueue *eq = mEmitQueue;

    while (eq)
    {
        if (eq->handle == handle)
        {
            eq->markDropped();
            return;
        }

        eq = eq->next;
    }
}

TQEmitQueue *TQManageQueue::addEntity(_EMIT_TYPE_t etype)
{
    DECL_TRACER("TQManageQueue::addEntity(_EMIT_TYPE_t etype)");

    TQEmitQueue *eq;

    try
    {
        eq = new TQEmitQueue;
        eq->etype = etype;

        if (!mEmitQueue)
            mEmitQueue = eq;
        else
        {
            TQEmitQueue *p = mEmitQueue;

            while(p->next)
                p = p->next;

            p->next = eq;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Memory error: " << e.what());
        return nullptr;
    }

    return eq;
}

/**
 * This is a kind of garbage collector. It scans the queue for duplicate
 * entries and removes the oldest ones. An entry is duplicate if it has the
 * same handle number and, to be sure, the same type.
 * This method makes sure, that the queue contains no more events than
 * necessary. This reduces the drawings on the surface and an object is drawn
 * only once after the application became active.
 */
void TQManageQueue::removeDuplicates()
{
    DECL_TRACER("TQManageQueue::removeDuplicates()");

    TQEmitQueue *prev, *eq = mEmitQueue;
    prev = nullptr;
    bool noInc = false;

    if (!eq)
        return;

    while(eq)
    {
        TQEmitQueue *p = mEmitQueue;

        while(p)
        {
            if (p == eq)
            {
                p = p->next;
                continue;
            }

            if (p->handle == eq->handle && p->etype == eq->etype)   // Remove the duplicate;
            {
                if (!prev)
                {
                    mEmitQueue = mEmitQueue->next;
                    delete eq;
                    eq = mEmitQueue;
                    noInc = true;
                }
                else
                {
                    prev->next = eq->next;
                    delete eq;
                    eq = prev;
                }

                break;
            }

            p = p->next;
        }

        if (noInc)
        {
            noInc = false;
            continue;
        }

        prev = eq;
        eq = eq->next;
    }
}
