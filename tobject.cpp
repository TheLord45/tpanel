/*
 * Copyright (C) 2020 to 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include <QTextObject>
#include <QLabel>
#include <QImage>
#include <QWidget>
#include <QPropertyAnimation>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#include "tobject.h"
#include "terror.h"

TObject::TObject()
{
    DECL_TRACER("TObject::TObject()");
}

TObject::~TObject()
{
    DECL_TRACER("TObject::~TObject()");

    clear();
}

void TObject::clear(bool force)
{
    DECL_TRACER("TObject::clear()");

    OBJECT_t *obj = mObject;

    while (obj)
    {
        OBJECT_t *next = obj->next;

        if (!force)
            dropContent(obj);

        delete obj;
        obj = next;
    }

    mObject = nullptr;
}

void TObject::dropContent(OBJECT_t* obj)
{
    DECL_TRACER("TObject::dropContent(OBJECT_t* obj)");

    try
    {
        switch (obj->type)
        {
            case OBJ_TEXT:
            case OBJ_INPUT:
                if (obj->object.multitext)
                {
    //                delete obj->object.text;
                    obj->object.multitext = nullptr;
                }

                if (obj->object.linetext)
                    obj->object.linetext = nullptr;

                obj->wid = 0;
            break;

            case OBJ_BUTTON:
                if (obj->object.label)
                {
    //                delete obj->object.label;
                    obj->object.label = nullptr;
                }
            break;

            case OBJ_PAGE:
            case OBJ_SUBPAGE:
                if (obj->object.widget)
                {
                    delete obj->object.widget;
                    obj->object.widget = nullptr;
                }
                break;

            case OBJ_VIDEO:
                if (obj->object.vwidget)
                {
                    delete obj->object.vwidget;

                    if (obj->player)
                        delete obj->player;

                    obj->object.vwidget = nullptr;
                    obj->player = nullptr;
                }

            default:
                break;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error freeing an object: " << e.what());
    }
}

TObject::OBJECT_t *TObject::addObject()
{
    DECL_TRACER("TObject::addObject()");

    OBJECT_t *obj = new OBJECT_t;
    obj->next = nullptr;
    obj->object.vwidget = nullptr;
    obj->player = nullptr;
    obj->object.label = nullptr;
    obj->object.multitext = nullptr;
    obj->object.linetext = nullptr;
    obj->object.widget = nullptr;

    if (!mObject)
        mObject = obj;
    else
    {
        OBJECT_t *p = mObject;

        while (p->next)
            p = p->next;

        p->next = obj;
    }

    return obj;
}

TObject::OBJECT_t *TObject::findObject(ulong handle)
{
    DECL_TRACER("TObject::findObject(ulong handle)");

    OBJECT_t *obj = mObject;

    while (obj)
    {
        if (obj->handle == handle)
            return obj;

        obj = obj->next;
    }

    return nullptr;
}

TObject::OBJECT_t * TObject::findObject(WId id)
{
    DECL_TRACER("TObject::findObject(WId id)");

    OBJECT_t *obj = mObject;

    while (obj)
    {
        if (obj->wid == id)
            return obj;

        obj = obj->next;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findFirstChild(ulong handle)
{
    DECL_TRACER("TObject::findFirstChild(ulong handle)");

    OBJECT_t *obj = mObject;

    while (obj)
    {
        if (obj->handle != (handle & 0xffff0000) && (obj->handle & 0xffff0000) == (handle & 0xffff0000))
            return obj;

        obj = obj->next;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findNextChild(ulong handle)
{
    DECL_TRACER("TObject::findNextChild(ulong handle)");

    OBJECT_t *obj = mObject;
    bool next = false;

    while (obj)
    {
        if (next && (obj->handle & 0xffff0000) == (handle & 0xffff0000))
            return obj;

        if (obj->handle == handle)
            next = true;

        obj = obj->next;
    }

    return nullptr;
}

TObject::OBJECT_t * TObject::getMarkedRemove()
{
    DECL_TRACER("TObject::getMarkedRemove()");

    OBJECT_t *obj = mObject;

    while (obj)
    {
        if (obj->remove)
            return obj;

        obj = obj->next;
    }

    return nullptr;
}

TObject::OBJECT_t * TObject::getNextMarkedRemove(TObject::OBJECT_t* object)
{
    DECL_TRACER("TObject::getNextMarkedRemove(TObject::OBJECT_t* obj)");

    OBJECT_t *obj = nullptr;

    if (!object || !object->next)
        return nullptr;

    obj = object->next;

    while (obj)
    {
        if (obj->remove)
            return obj;

        obj = obj->next;
    }

    return nullptr;
}

void TObject::removeObject(ulong handle)
{
    DECL_TRACER("TObject::removeObject(ulong handle)");

    OBJECT_t *obj = mObject;
    OBJECT_t *prev = nullptr;

    while (obj)
    {
        if (obj->handle == handle)
        {
            if (!prev)
            {
                mObject = obj->next;
                delete obj;
            }
            else
            {
                prev->next = obj->next;
                delete obj;
            }

            return;
        }

        prev = obj;
        obj = obj->next;
    }
}

void TObject::removeAllChilds(ulong handle)
{
    DECL_TRACER("TObject::removeAllChilds(ulong handle)");

    OBJECT_t *obj = mObject;
    OBJECT_t *prev = nullptr;

    while (obj)
    {
        if ((obj->handle & 0xffff0000) == (handle & 0xffff0000) && obj->handle != (handle & 0xffff0000))
        {
            if (!prev)
            {
                mObject = obj->next;
                delete obj;
                obj = mObject;
                continue;
            }
            else
            {
                prev->next = obj->next;
                delete obj;
                obj = prev;
            }
        }

        prev = obj;
        obj = obj->next;
    }
}

std::string TObject::objectToString(TObject::OBJECT_TYPE o)
{
    switch(o)
    {
        case OBJ_BUTTON:  return "BUTTON"; break;
        case OBJ_INPUT:   return "INPUT"; break;
        case OBJ_NONE:    return "undefined"; break;
        case OBJ_PAGE:    return "PAGE"; break;
        case OBJ_SUBPAGE: return "SUBPAGE"; break;
        case OBJ_TEXT:    return "TEXT"; break;
        case OBJ_VIDEO:   return "VIDEO"; break;
    }

    return std::string();   // Should not happen but is needed to satisfy the compiler.
}
