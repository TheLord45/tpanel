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
#ifdef QT5_LINUX
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#else
#include <QTextLayout>
#endif
#include "tobject.h"
#include "terror.h"

using std::string;

std::mutex mutex_obj;

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

void TObject::dropContent(OBJECT_t* obj, bool lock)
{
    DECL_TRACER("TObject::dropContent(OBJECT_t* obj, bool lock)");

    if (lock)
        mutex_obj.lock();

    try
    {
        switch (obj->type)
        {
            case OBJ_TEXT:
            case OBJ_INPUT:
                if (obj->object.plaintext)
                {
                    obj->object.plaintext->close();
                    obj->object.plaintext = nullptr;
                }

                obj->wid = 0;
            break;

            case OBJ_BUTTON:
                if (obj->object.label)
                    obj->object.label = nullptr;
            break;

            // This are the parent widgets (windows) and must be deleted.
            // If this widgets are deleted, Qt deletes their children.
            case OBJ_PAGE:
            case OBJ_SUBPAGE:
                if (obj->object.widget)
                {
                    obj->object.widget->close();        // This deletes all childs and the window itself
                    obj->object.widget = nullptr;
                }
            break;

            case OBJ_VIDEO:
                if (obj->object.vwidget)
                {
#ifdef QT5_LINUX
                    if (obj->player)
                        delete obj->player;
#endif
                    obj->object.vwidget = nullptr;
                    obj->player = nullptr;
                }
            break;

            case OBJ_LIST:
                if (obj->object.list)
                    obj->object.list = nullptr;
            break;

            default:
                break;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error freeing an object: " << e.what());
    }

    if (lock)
        mutex_obj.unlock();
}

TObject::OBJECT_t *TObject::addObject()
{
    DECL_TRACER("TObject::addObject()");

    mutex_obj.lock();
    OBJECT_t *obj = new OBJECT_t;
    obj->next = nullptr;
    obj->object.vwidget = nullptr;
    obj->player = nullptr;
    obj->object.label = nullptr;
    obj->object.plaintext = nullptr;
    obj->object.widget = nullptr;
    obj->object.list = nullptr;

    if (!mObject)
        mObject = obj;
    else
    {
        OBJECT_t *p = mObject;

        while (p->next)
            p = p->next;

        p->next = obj;
    }

    mutex_obj.unlock();
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

TObject::OBJECT_t *TObject::findFirstWindow()
{
    DECL_TRACER("TObject::getFirstWindow()");

    OBJECT_t *obj = mObject;

    while (obj)
    {
        if (obj->type == OBJ_SUBPAGE)
            return obj;

        obj = obj->next;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findNextWindow(TObject::OBJECT_t *obj)
{
    DECL_TRACER("TObject::findNextWindow()");

    if (!obj || !obj->next)
        return nullptr;

    OBJECT_t *o = obj->next;

    while (o)
    {
        if (o->type == OBJ_SUBPAGE)
            return o;

        o = o->next;
    }

    return nullptr;
}

void TObject::removeObject(ulong handle, bool drop)
{
    DECL_TRACER("TObject::removeObject(ulong handle, bool drop)");

    mutex_obj.lock();
    OBJECT_t *obj = mObject;
    OBJECT_t *prev = nullptr;

    while (obj)
    {
        if (obj->handle == handle)
        {
            if (!prev)
            {
                mObject = obj->next;

                if (drop)
                    dropContent(obj, false);

                delete obj;
            }
            else
            {
                prev->next = obj->next;

                if (drop)
                    dropContent(obj, false);

                delete obj;
            }

            mutex_obj.unlock();
            return;
        }

        prev = obj;
        obj = obj->next;
    }

    mutex_obj.unlock();
}

void TObject::removeAllChilds(ulong handle, bool drop)
{
    DECL_TRACER("TObject::removeAllChilds(ulong handle, bool drop)");

    mutex_obj.lock();
    OBJECT_t *obj = mObject;
    OBJECT_t *prev = nullptr;

    while (obj)
    {
        if ((obj->handle & 0xffff0000) == (handle & 0xffff0000) && obj->handle != (handle & 0xffff0000))
        {
            if (!prev)
            {
                mObject = obj->next;

                if (drop)
                    dropContent(obj, false);

                delete obj;
                obj = mObject;
                continue;
            }
            else
            {
                prev->next = obj->next;

                if (drop)
                    dropContent(obj, false);

                delete obj;
                obj = prev;
            }
        }

        prev = obj;
        obj = obj->next;
    }

    mutex_obj.unlock();
}

void TObject::cleanMarked()
{
    DECL_TRACER("TObject::cleanMarked()");

    mutex_obj.lock();
    OBJECT_t *obj = mObject;
    OBJECT_t *prev = nullptr;

    while (obj)
    {
        if (obj->remove && (!obj->animation || obj->animation->state() != QAbstractAnimation::Running))
        {
            if (obj->type == OBJ_SUBPAGE && obj->object.widget)
            {
                delete obj->object.widget;
                obj->object.widget = nullptr;
            }

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

    mutex_obj.unlock();
}

string TObject::objectToString(TObject::OBJECT_TYPE o)
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
        case OBJ_LIST:    return "LIST"; break;
    }

    return string();   // Should not happen but is needed to satisfy the compiler.
}
