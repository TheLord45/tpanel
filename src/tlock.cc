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

#include "tlock.h"
#include <vector>
#include <thread>

#include "terror.h"

using std::vector;
using std::thread;

static std::vector<__LOCKLIST_t> _locks;

template<typename _TMutex>
TLock<_TMutex>::TLock(TLock::mutex_type& __m)
    : _M_device(__m)
{
//    DECL_TRACER("TLock<_TMutex>::TLock(TLock::mutex_type& __m)");

    bool death = false;
    bool l = addLock(&death);

    if ((mNoDeathLock || death) && !l)
        return;

    _M_device.lock();
}

template<typename _TMutex>
TLock<_TMutex>::TLock(TLock::mutex_type& __m, char *file, int line)
    : _M_device(__m),
      mFilename(file),
      mLineNumber(line)
{
//    DECL_TRACER("TLock<_TMutex>::TLock(TLock::mutex_type& __m)");

    stripFileName();
    bool death = false;
    bool l = addLock(&death);

//    if ((mNoDeathLock || death) && !l)
//        return;

    if (!l && (mNoDeathLock || death))
    {
        wait();

        if (isLocked())
            return;
    }

    _M_device.lock();
}

template<typename _TMutex>
TLock<_TMutex>::TLock(mutex_type& __m, bool tryit, char *file, int line)
    : _M_device(__m),
      mNoDeathLock(tryit),
      mFilename(file),
      mLineNumber(line)
{
//    DECL_TRACER("TLock<_TMutex>::TLock(mutex_type& __m, bool tryit, char *file, int line)");

    stripFileName();
    bool death = false;
    bool l = addLock(&death);

//    if ((mNoDeathLock || death) && !l)
//        return;

    if (!l && tryit)
    {
        wait();

        if (isLocked())
            return;
    }

    _M_device.lock();
}

template<typename _TMutex>
TLock<_TMutex>::TLock(TLock::mutex_type& __m, std::adopt_lock_t) noexcept
    : _M_device(__m)
{
//    DECL_TRACER("TLock<_TMutex>::TLock(TLock::mutex_type& __m, std::adopt_lock_t) noexcept");
}

template<typename _TMutex>
TLock<_TMutex>::~TLock()
{
//    DECL_TRACER("TLock<_TMutex>::~TLock()");

    if (removeLock())
        _M_device.unlock();
}

template<typename _TMutex>
void TLock<_TMutex>::unlock()
{
//    DECL_TRACER("TLock<_TMutex>::unlock()");

    if (_locks.empty())
        return;

    vector<__LOCKLIST_t>::iterator iter;
    std::mutex::native_handle_type nhandle = _M_device.native_handle();

    for (iter = _locks.begin(); iter != _locks.end(); ++iter)
    {
        if (iter->native_handle == nhandle && iter->state)
        {
            iter->state = false;
            iter->noDeathLock = false;
            _M_device.unlock();

            if (mFilename.empty())
            {
                MSG_DEBUG("The mutex handle " << iter->native_handle << " was released!");
            }
            else
            {
                MSG_DEBUG("The mutex handle " << iter->native_handle << " was released on file " << mFilename << " at line " << mLineNumber << "!");
            }

            return;
        }
    }
}

template<typename _TMutex>
void TLock<_TMutex>::unlock(char* file, int line)
{
//    DECL_TRACER("TLock<_TMutex>::unlock(char* file, int line)");

    mFilename = file;
    mLineNumber = line;
    stripFileName();

    unlock();
}

template<typename _TMutex>
void TLock<_TMutex>::relock()
{
//    DECL_TRACER("TLock<_TMutex>::relock(TLock::mutex_type& __m)");

    if (_locks.empty())
        return;

    vector<__LOCKLIST_t>::iterator iter;
    std::mutex::native_handle_type nhandle = _M_device.native_handle();

    for (iter = _locks.begin(); iter != _locks.end(); ++iter)
    {
        if (iter->native_handle == nhandle && !iter->state)
        {
            iter->state = true;
            _M_device.lock();
            return;
        }
    }
}

template<typename _TMutex>
bool TLock<_TMutex>::isLocked()
{
    if (_locks.empty())
        return false;

    vector<__LOCKLIST_t>::iterator iter;
    std::mutex::native_handle_type nhandle = _M_device.native_handle();

    for (iter = _locks.begin(); iter != _locks.end(); ++iter)
    {
        if (iter->native_handle == nhandle && iter->state)
            return true;
    }

    return false;
}

template<typename _TMutex>
void TLock<_TMutex>::setNoDeathLock(bool l)
{
//    DECL_TRACER("TLock<_TMutex>::setNoDeathLock(bool l)");

    mNoDeathLock = l;
    vector<__LOCKLIST_t>::iterator iter;
    std::mutex::native_handle_type nhandle = _M_device.native_handle();

    for (iter = _locks.begin(); iter != _locks.end(); ++iter)
    {
        if (iter->native_handle == nhandle)
        {
            iter->noDeathLock = l;
            break;
        }
    }
}


template<typename _TMutex>
bool TLock<_TMutex>::addLock(bool *death)
{
//    DECL_TRACER("TLock<_TMutex>::addLock(bool *death)");

    __LOCKLIST_t lc;
    lc.state = true;
    lc.noDeathLock = mNoDeathLock;
    lc.native_handle = _M_device.native_handle();

    if (_locks.empty())
    {
        _locks.push_back(lc);

        if (mFilename.empty())
        {
            MSG_DEBUG("Lock for mutex handle " << lc.native_handle << " was added.");
        }
        else
        {
            MSG_DEBUG("Lock for mutex handle " << lc.native_handle << " was added on file " << mFilename << " at line " << mLineNumber << ".");
        }

        if (death)
            *death = lc.noDeathLock;

        return true;
    }

    vector<__LOCKLIST_t>::iterator iter;

    for (iter = _locks.begin(); iter != _locks.end(); ++iter)
    {
        if (iter->native_handle == lc.native_handle)
        {
            if (death)
                *death = iter->noDeathLock;

            if (iter->state)
            {
                if (mNoDeathLock || iter->noDeathLock)
                {
                    if (mFilename.empty())
                    {
                        MSG_WARNING("The mutex handle " << iter->native_handle << " is already locked!");
                    }
                    else
                    {
                        MSG_WARNING("The mutex handle " << iter->native_handle << " is already locked on file " << mFilename << " at line " << mLineNumber << "!");
                    }
                }
                else
                {
                    if (mFilename.empty())
                    {
                        MSG_ERROR("Death lock detected! The mutex handle " << iter->native_handle << " is already locked.");
                    }
                    else
                    {
                        MSG_ERROR("Death lock detected! The mutex handle " << iter->native_handle << " is already locked on file " << mFilename << " at line " << mLineNumber << ".");
                    }
                }

                return false;
            }
            else
            {
                iter->state = true;

                if (mFilename.empty())
                {
                    MSG_DEBUG("Lock for mutex handle " << iter->native_handle << " was reactivated.");
                }
                else
                {
                    MSG_DEBUG("Lock for mutex handle " << iter->native_handle << " was reactivated on file " << mFilename << " at line " << mLineNumber << ".");
                }

                return true;
            }
        }
    }

    _locks.push_back(lc);

    if (mFilename.empty())
    {
        MSG_DEBUG("Lock for mutex handle " << lc.native_handle << " was added.");
    }
    else
    {
        MSG_DEBUG("Lock for mutex handle " << lc.native_handle << " was added on file " << mFilename << " at line " << mLineNumber << ".");
    }

    if (death)
        *death = lc.noDeathLock;

    return true;
}

template<typename _TMutex>
void TLock<_TMutex>::wait()
{
//    DECL_TRACER("TLock<_TMutex>::wait()");

    int loops = 100;

    while (!_M_device.try_lock() && loops >= 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        loops--;
    }

    if (loops >= 0)
        addLock(nullptr);
}

template<typename _TMutex>
bool TLock<_TMutex>::removeLock()
{
//    DECL_TRACER("TLock<_TMutex>::removeLock(__LOCKLIST_t& e)");

    if (_locks.empty())
        return false;

    vector<__LOCKLIST_t>::iterator iter;
    std::mutex::native_handle_type nhandle = _M_device.native_handle();

    for (iter = _locks.begin(); iter != _locks.end(); ++iter)
    {
        if (iter->native_handle == nhandle)
        {
            if (mFilename.empty())
            {
                MSG_DEBUG("Lock for mutex handle " << iter->native_handle << " will be removed.");
            }
            else
            {
                MSG_DEBUG("Lock for mutex handle " << iter->native_handle << " will be removed on file " << mFilename << " at line " << mLineNumber << ".");
            }

            bool ret = iter->state;
            _locks.erase(iter);
            return ret;
        }
    }

    return false;
}

template<typename _TMutex>
void TLock<_TMutex>::stripFileName()
{
//    DECL_TRACER("TLock<_TMutex>::stripFileName()");

    if (mFilename.empty())
        return;

    size_t pos = mFilename.find_last_of("/");

    if (pos != std::string::npos)
        mFilename = mFilename.substr(pos + 1);
}
