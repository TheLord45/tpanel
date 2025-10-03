/*
 * Copyright (C) 2025 by Andreas Theofilu <andreas@theosys.at>
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
#ifndef __TRESIZE_H__
#define __TRESIZE_H__

#include <string>
#include <vector>

class QDialog;

class TResize
{
    public:
        typedef enum
        {
            QTOOLBUTTON,
            QPUSHBUTTON,
            QLABEL,
            QFRAME,
            QLINEEDIT,
            QLINE,
            QPROGRESSBAR
        }ELEMTYPES_t;

        typedef struct ELEMENTS_t
        {
            std::string name;
            ELEMTYPES_t elType;
        }ELEMENTS_t;

        static void dlgResize(QDialog *dlg, std::vector<ELEMENTS_t> elements, double sf);

    protected:
        template<typename T>
        void static scaleObject(T *obj);
        static int scale(int value);

    private:
        TResize() {};   // Don't use the constructor!

        static double mScaleFactor;
};

#endif
