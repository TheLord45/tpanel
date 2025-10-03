/*
 * Copyright (C) 2022 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TVECTOR_H__
#define __TVECTOR_H__

#include <vector>
#include <mutex>


template <class T, class Alloc=std::allocator<T>>
class TVector
{
    public:
        TVector() {}
        TVector(const TVector& orig) : vec{orig.vec} {}
        TVector(std::vector<T>& v) : vec{v} {}
        TVector(TVector&& orig) : vec{std::move(orig.vec)} {}

        typedef typename std::vector<T>::size_type size_type;
        typedef typename std::vector<T>::value_type value_type;
        typedef typename std::vector<T>::iterator iterator;
        typedef typename std::vector<T>::const_iterator const_iterator;
        typedef typename std::vector<T>::reverse_iterator reverse_iterator;
        typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;
        typedef typename std::vector<T>::reference reference;
        typedef typename std::vector<T>::const_reference const_reference;

        /*wrappers for three different at() functions*/
        template <class InputIterator>
        void assign(InputIterator first, InputIterator last)
        {
            // using a local lock_guard to lock mutex guarantees that the
            // mutex will be unlocked on destruction and in the case of an
            // exception being thrown.
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.assign(first, last);
        }

        void assign(size_type n, const value_type& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.assign(n, val);
        }

        void assign(std::initializer_list<value_type> il)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.assign(il.begin(), il.end());
        }

        /*wrappers for at() functions*/
        reference at(size_type n)
        {
            return vec.at(n);
        }

        const_reference at(size_type n) const
        {
            return vec.at(n);
        }

        /*wrappers for back() functions*/
        reference back()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.back();
        }

        const reference back() const
        {
            return vec.back();
        }

        /*wrappers for begin() functions*/
        iterator begin()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.begin();
        }

        const iterator begin() const noexcept
        {
            return vec.begin();
        }

        /*wrapper for capacity() fucntion*/
        size_type capacity() const noexcept
        {
            return vec.capacity();
        }

        /*wrapper for cbegin() function*/
        const iterator cbegin()
        {
            return vec.cbegin();
        }

        /*wrapper for cend() function*/
        const iterator cend()
        {
            return vec.cend();
        }

        /*wrapper for clear() function*/
        void clear()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.clear();
        }

        /*wrapper for crbegin() function*/
        const_reverse_iterator crbegin() const noexcept
        {
            return vec.crbegin();
        }

        /*wrapper for crend() function*/
        const_reverse_iterator crend() const noexcept
        {
            return vec.crend();
        }

        /*wrappers for data() functions*/
        value_type* data()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.data();
        }

        const value_type* data() const noexcept
        {
            std::lock_guard<const std::mutex> vectorLockGuard(mut);

            return vec.data();
        }

        /*wrapper for emplace() function*/
        template <class... Args>
        void emplace(const iterator position, Args&&... args)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.emplace(position, args...);
        }

        /*wrapper for emplace_back() function*/
        template <class... Args>
        void emplace_back(Args&&... args)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.emplace_back(args...);
        }

        /*wrapper for empty() function*/
        bool empty() const noexcept
        {
            return vec.empty();
        }

        /*wrappers for end() functions*/
        iterator end()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.end();
        }

        const iterator end() const noexcept
        {
            return vec.end();
        }

        /*wrapper functions for erase()*/
        iterator erase(const_iterator position)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.erase(position);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.erase(first, last);
        }

        /*wrapper functions for front()*/
        reference front()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.front();
        }

        const reference front() const
        {
            return vec.front();
        }

        /*wrapper function for get_allocator()*/
        value_type get_allocator() const noexcept
        {
            return vec.get_allocator();
        }

        /*wrapper functions for insert*/
        iterator insert(const_iterator position, const value_type& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.insert(position, val);
        }

        iterator insert(const_iterator position, size_type n, const value_type& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.insert(position, n, val);
        }

        template <class InputIterator>
        iterator insert(const_iterator position, InputIterator first, InputIterator last)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.insert(position, first, last);
        }

        iterator insert(const_iterator position, value_type&& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.insert(position, val);
        }

        iterator insert(const_iterator position, std::initializer_list<value_type> il)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.insert(position, il.begin(), il.end());
        }

        /*wrapper function for max_size*/
        size_type max_size() const noexcept
        {
            return vec.max_size();
        }

        /*wrapper functions for operator =*/
        std::vector<T>& operator= (const std::vector<T>& x)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.swap(x);
            return vec;
        }

        std::vector<T>& operator= (std::vector<T>&& x)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec = std::move(x);
            return vec;
        }

        std::vector<T>& operator= (std::initializer_list<value_type> il)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.assign(il.begin(), il.end());
            return vec;
        }

        /*wrapper functions for operator []*/
        reference operator[] (size_type n)
        {
            return std::ref(n);
        }

        const_reference operator[] (size_type n) const
        {
            return std::cref(n);
        }

        /*wrapper function for pop_back()*/
        void pop_back()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.pop_back();
        }

        /*wrapper functions for push_back*/
        void push_back(const value_type& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.push_back(val);
        }

        void push_back(value_type&& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.push_back(val);
        }

        /*wrapper functions for rbegin()*/
        reverse_iterator rbegin() noexcept
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.rbegin();
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return vec.rbegin();
        }

        /*wrapper functions for rend()*/
        reverse_iterator rend() noexcept
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            return vec.rend();
        }

        const_reverse_iterator rend() const noexcept
        {
            return vec.rend();
        }

        /*wrapper function for reserve()*/
        void reserve(size_type n)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.reserve(n);
        }

        /*wrapper functions for resize()*/
        void resize(size_type n)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.resize(n);
        }

        void resize(size_type n, const value_type& val)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.resize(n, val);
        }

        void shrink_to_fit()
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.shrink_to_fit();
        }

        //add function for size
        size_type size() const noexcept
        {
            return vec.size();
        }

        /*wrapper function for swap()*/
        void swap(std::vector<T>& x)
        {
            std::lock_guard<std::mutex> vectorLockGuard(mut);

            vec.swap(x);
        }

    private:
        std::vector<T> vec;
        std::mutex mut;
};

#endif
