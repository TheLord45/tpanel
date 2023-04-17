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

#ifndef __TLOCK_H__
#define __TLOCK_H__

#include <string>
#include <mutex>
#include <thread>

typedef struct __LOCKLIST_t
{
    std::mutex::native_handle_type native_handle;   // Handle of mutex
    bool state{false};                              // TRUE = mutex is locked
    bool noDeathLock{false};                        // TRUE = Ignores additional logging to prevent a death lock.
}__LOCKLIST_t;

template<typename _TMutex>
class TLock
{
    public:
        typedef _TMutex mutex_type;
        explicit TLock(mutex_type& __m);
        explicit TLock(mutex_type& __m, char *file, int line);
        explicit TLock(mutex_type& __m, bool tryit, char *file, int line);
        TLock(mutex_type& __m, std::adopt_lock_t) noexcept;
        ~TLock();

        void unlock();
        void unlock(char *file, int line);
        void relock();
        bool isLocked();
        void setNoDeathLock(bool l);     // USE WITH CARE!! This disables locking for the mutex!!
        void _setMetaData(char *file, int line) { mFilename = file; mLineNumber = line; }

        TLock(const TLock&) = delete;
        TLock& operator=(const TLock&) = delete;

    private:
        bool addLock(bool *death=nullptr);
        void wait();
        bool removeLock();
        void stripFileName();

        mutex_type&  _M_device;
        bool mNoDeathLock{false};
        std::string mFilename;
        int mLineNumber{0};
};

#include "tlock.cc"

#define TLOCKER(mtx)    TLock<std::mutex> __Guard(mtx, (char *)__FILE__, __LINE__)
#define TUNLOCKER()     __Guard.unlock((char *)__FILE__, __LINE__)
#define TTRYLOCK(mtx)   TLock<std::mutex> __Guard(mtx, true, (char *)__FILE__, __LINE__)

#endif
