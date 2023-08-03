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

#include <functional>
#include <map>

#include <QTextObject>
#include <QLabel>
#include <QImage>
#include <QWidget>
#include <QPropertyAnimation>
#include <QListWidget>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimediaWidgets/QVideoWidget>
#else
#include <QTextLayout>
#endif
#include "tobject.h"
#include "terror.h"
#include "tqscrollarea.h"
#include "tlock.h"
#include "tqtmain.h"
#include "tresources.h"

using std::string;
using std::map;
using std::pair;

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

    if (mObjects.empty())
        return;

    std::map<ulong, OBJECT_t>::iterator iter;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if (!force)
            dropContent(&iter->second);
    }

    mObjects.clear();
}

void TObject::dropContent(OBJECT_t* obj, bool lock)
{
    DECL_TRACER("TObject::dropContent(OBJECT_t* obj, bool lock)");

    if (!obj)
        return;

    if (lock)
        TLOCKER(mutex_obj);

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
                obj->invalid = true;
            break;

            case OBJ_BUTTON:
                if (obj->object.label)
                    obj->object.label = nullptr;

                obj->invalid = true;
            break;

            // This are the parent widgets (windows) and must be deleted.
            // If this widgets are deleted, Qt deletes their children.
            case OBJ_PAGE:
            case OBJ_SUBPAGE:
                obj->invalid = true;

                if (obj->object.widget)
                {
                    if (obj->type == OBJ_SUBPAGE)
                    {
                        obj->object.widget->close();        // This deletes all childs and the widget itself
                        obj->object.widget = nullptr;
                    }
                    else
                        obj->object.widget->hide();         // Don't delete a page because it is still stored in the stacked widget.
                }
            break;

            case OBJ_SUBVIEW:
                obj->invalid = true;

                if (obj->object.area)
                {
                    if (mMainWindow)
                    {
                        obj->connected = false;
                        mMainWindow->disconnectArea(obj->object.area);
                    }

                    delete obj->object.area;
                    obj->object.area = nullptr;
                }
            break;

            case OBJ_VIDEO:
                if (obj->object.vwidget)
                {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                    if (obj->player)
                        delete obj->player;
#endif
                    obj->object.vwidget = nullptr;
                    obj->player = nullptr;
                }

                obj->invalid = true;
            break;

            case OBJ_LIST:
                if (obj->object.list)
                {
                    if (mMainWindow)
                        mMainWindow->disconnectList(obj->object.list);

                    obj->object.list->close();
                    obj->object.list = nullptr;
                }

                obj->invalid = true;
            break;

            default:
                obj->invalid = true;
                break;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error freeing an object: " << e.what());
    }
}

bool TObject::addObject(OBJECT_t& obj)
{
    DECL_TRACER("TObject::addObject(OBJECT_t& obj)");

    if (obj.handle == 0 || obj.type == OBJ_NONE)
        return false;

    TLOCKER(mutex_obj);

    if (!mObjects.empty() && mObjects.find(obj.handle) != mObjects.end())
        return false;

    const auto [o, success] = mObjects.insert(pair<ulong, OBJECT_t>(obj.handle, obj));
    return success;
}

TObject::OBJECT_t *TObject::findObject(ulong handle)
{
    DECL_TRACER("TObject::findObject(ulong handle)");

    if (mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter = mObjects.find(handle);

    if (iter != mObjects.end())
        return &iter->second;

    return nullptr;
}

TObject::OBJECT_t *TObject::findObject(WId id)
{
    DECL_TRACER("TObject::findObject(WId id)");

    if (mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if (iter->second.wid == id)
            return &iter->second;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findFirstChild(ulong handle)
{
    DECL_TRACER("TObject::findFirstChild(ulong handle)");

    if (mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if (iter->first != (handle & 0xffff0000) && (iter->first & 0xffff0000) == (handle & 0xffff0000))
            return &iter->second;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findNextChild(ulong handle)
{
    DECL_TRACER("TObject::findNextChild(ulong handle)");

    if (mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;
    bool next = true;

    for (iter = mObjects.find(handle); iter != mObjects.end(); ++iter)
    {
        if (next)
        {
            next = false;
            continue;
        }

        if ((iter->first & 0xffff0000) == (handle & 0xffff0000))
            return &iter->second;
    }

    return nullptr;
}

TObject::OBJECT_t * TObject::getMarkedRemove()
{
    DECL_TRACER("TObject::getMarkedRemove()");

    if (mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if (iter->second.remove)
            return &iter->second;
    }

    return nullptr;
}

TObject::OBJECT_t * TObject::getNextMarkedRemove(TObject::OBJECT_t* object)
{
    DECL_TRACER("TObject::getNextMarkedRemove(TObject::OBJECT_t* obj)");

    if (!object || mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;
    bool next = true;

    for (iter = mObjects.find(object->handle); iter != mObjects.end(); ++iter)
    {
        if (next)
        {
            next = false;
            continue;
        }

        if (iter->second.remove)
            return &iter->second;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findFirstWindow()
{
    DECL_TRACER("TObject::getFirstWindow()");

    if (mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if (iter->second.type == OBJ_SUBPAGE)
            return &iter->second;
    }

    return nullptr;
}

TObject::OBJECT_t *TObject::findNextWindow(TObject::OBJECT_t *obj)
{
    DECL_TRACER("TObject::findNextWindow()");

    if (!obj || mObjects.empty())
        return nullptr;

    map<ulong, OBJECT_t>::iterator iter;
    bool next = true;

    for (iter = mObjects.find(obj->handle); iter != mObjects.end(); ++iter)
    {
        if (next)
        {
            next = false;
            continue;
        }

        if (iter->second.type == OBJ_SUBPAGE)
            return &iter->second;
    }

    return nullptr;
}

void TObject::removeObject(ulong handle, bool drop)
{
    DECL_TRACER("TObject::removeObject(ulong handle, bool drop)");

    if (!handle || mObjects.empty())
        return;

    TLOCKER(mutex_obj);
    map<ulong, OBJECT_t>::iterator iter = mObjects.find(handle);

    if (iter != mObjects.end())
    {
        if (drop)
            dropContent(&iter->second, false);

        mObjects.erase(iter);
    }
}

void TObject::removeAllChilds(ulong handle, bool drop)
{
    DECL_TRACER("TObject::removeAllChilds(ulong handle, bool drop)");

    if (!handle || mObjects.empty())
        return;

    TLOCKER(mutex_obj);
    map<ulong, OBJECT_t>::iterator iter;
    bool repeat = false;

    do
    {
        repeat = false;

        for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
        {
            if ((iter->first & 0xffff0000) == (handle & 0xffff0000) && iter->first != (handle & 0xffff0000))
            {
                if (drop)
                    dropContent(&iter->second, false);

                mObjects.erase(iter);
                repeat = true;
                break;
            }
        }
    }
    while (repeat);
}

void TObject::cleanMarked()
{
    DECL_TRACER("TObject::cleanMarked()");

    if (mObjects.empty())
        return;

    TLOCKER(mutex_obj);
    map<ulong, OBJECT_t>::iterator iter;
    bool repeat = false;

    do
    {
        repeat = false;

        for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
        {
            if (iter->second.remove && (!iter->second.animation || iter->second.animation->state() != QAbstractAnimation::Running))
            {
                if (iter->second.type == OBJ_SUBPAGE && iter->second.object.widget)
                {
                    iter->second.object.widget->close();
                    iter->second.object.widget = nullptr;
                }

                mObjects.erase(iter);
                repeat = true;
                break;
            }
        }
    }
    while (repeat);
}

void TObject::invalidateAllObjects()
{
    DECL_TRACER("TObject::invalidateAllObjects()");

    if (mObjects.empty())
        return;

    TLOCKER(mutex_obj);
    map<ulong, OBJECT_t>::iterator iter;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        iter->second.remove = false;
        iter->second.invalid = true;
    }
}

void TObject::invalidateObject(ulong handle)
{
    DECL_TRACER("TObject::invalidateObject(ulong handle)");

    if (mObjects.empty())
        return;

    TLOCKER(mutex_obj);
    map<ulong, OBJECT_t>::iterator iter = mObjects.find(handle);

    if (iter != mObjects.end())
    {
        iter->second.remove = false;
        iter->second.invalid = true;
    }
}

void TObject::invalidateAllSubObjects(ulong handle)
{
    DECL_TRACER("TObject::invalidateAllSubObjects(ulong handle)");

    if (mObjects.empty())
        return;

    TLOCKER(mutex_obj);

    map<ulong, OBJECT_t>::iterator iter;
    bool first = true;

    for (iter = mObjects.find(handle); iter != mObjects.end(); ++iter)
    {
        if (first)
        {
            first = false;
            continue;
        }

        if ((iter->first & 0xffff0000) == handle)
        {
            MSG_DEBUG("Invalidating object " << handleToString(iter->first) << " of type " << objectToString(iter->second.type));
            iter->second.remove = false;
            iter->second.invalid = true;

            if (iter->second.type == OBJ_SUBVIEW && iter->second.object.area && mMainWindow && iter->second.connected)
            {
                iter->second.connected = false;
                mMainWindow->disconnectArea(iter->second.object.area);
            }
            else if (iter->second.type == OBJ_LIST && iter->second.object.list && mMainWindow && iter->second.connected)
            {
                iter->second.connected = false;
                mMainWindow->disconnectList(iter->second.object.list);
            }
        }
    }
}

bool TObject::enableObject(ulong handle)
{
    DECL_TRACER("TObject::enableObject(ulong handle)");

    if (mObjects.empty())
        return false;

    map<ulong, OBJECT_t>::iterator iter = mObjects.find(handle);

    if (iter != mObjects.end())
    {
        if (!iter->second.object.widget)
        {
            iter->second.remove = true;
            MSG_ERROR("Object " << handleToString(iter->first) << " of type " << objectToString(iter->second.type) << " has no QObject!");
            return false;
        }

        TLOCKER(mutex_obj);
        iter->second.remove = false;
        iter->second.invalid = false;

        if (iter->second.type == OBJ_SUBVIEW && iter->second.object.area && mMainWindow && !iter->second.connected)
        {
            mMainWindow->reconnectArea(iter->second.object.area);
            iter->second.connected = true;
        }
        else if (iter->second.type == OBJ_LIST && iter->second.object.list && mMainWindow && !iter->second.connected)
        {
            mMainWindow->reconnectList(iter->second.object.list);
            iter->second.connected = true;
        }

        return true;                         // We can savely return here because a handle is unique
    }

    return false;
}

bool TObject::enableAllSubObjects(ulong handle)
{
    DECL_TRACER("::enableAllSubObjects(ulong handle)");

    if (mObjects.empty())
        return false;

    TLOCKER(mutex_obj);
    map<ulong, OBJECT_t>::iterator iter;
    bool ret = true;

    for (iter = mObjects.begin(); iter != mObjects.end(); ++iter)
    {
        if (iter->first != handle && (iter->first & 0xffff0000) == handle)
        {
            if (!iter->second.object.widget)
            {
                iter->second.remove = true;
                MSG_ERROR("Object " << handleToString(iter->first) << " of type " << objectToString(iter->second.type) << " has no QObject!");
                ret = false;
            }
            else
            {
                iter->second.remove = false;
                iter->second.invalid = false;

                if (iter->second.type == OBJ_SUBVIEW && iter->second.object.area && mMainWindow && !iter->second.connected)
                {
                    mMainWindow->reconnectArea(iter->second.object.area);
                    iter->second.connected = true;
                }
                else if (iter->second.type == OBJ_LIST && iter->second.object.list && mMainWindow && !iter->second.connected)
                {
                    mMainWindow->reconnectList(iter->second.object.list);
                    iter->second.connected = true;
                }
            }
        }
    }

    return ret;
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
        case OBJ_SUBVIEW: return "SUBVIEW"; break;
    }

    return string();   // Should not happen but is needed to satisfy the compiler.
}
