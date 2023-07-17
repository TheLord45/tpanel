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
#ifndef TINTBORDER_H
#define TINTBORDER_H

#include <string>

class SkBitmap;
struct SkRect;

namespace Border
{
    typedef enum ERASE_PART_t
    {
        ERASE_NONE,
        ERASE_LEFT_RIGHT,
        ERASE_RIGHT_LEFT,
        ERASE_TOP_DOWN,
        ERASE_BOTTOM_UP,
        ERASE_OUTSIDE
    }ERASE_PART_t;

    typedef struct SYSBORDER_t
    {
        int id{0};                  // Internal unique ID number
        char *name{nullptr};        // Name of the border
        int number{0};              // AMX number
        char *style{nullptr};       // Style to use if dynamicaly calculated
        int width{0};               // The width of the border
        int radius{0};              // Radius for rounded corners
        bool calc{false};           // TRUE = Calculated inside, FALSE = Read from images
    }SYSBORDER_t;

    typedef struct TP4BORDERS_t
    {
        int id{0};
        std::string name;
        int prgNum[4];
    }TP4BORDERS_t;

    class TIntBorder
    {
        public:
            TIntBorder();

            bool drawBorder(SkBitmap* bm, const std::string& bname, int wt, int ht, const std::string& cb, bool force=false);
            int getBorderWidth(const std::string& bname, bool force=false);
            bool borderExist(const std::string& name);
            bool borderExist(int index);
            std::string getBorderName(int index);
            std::string getCorrectName(const std::string& name);
            bool isForcedBorder(const std::string& name);
            bool isTP4BorderValid(const std::string& name);
            std::string getTP4BorderName(int id);

            void erasePart(SkBitmap *bm, const SkBitmap& mask, ERASE_PART_t ep);
            void colorizeFrame(SkBitmap *frame, SkColor color);

        private:
            SkRect calcRect(int width, int height, int pen);
            bool setPixel(uint32_t *wpix, uint32_t color, bool bar);
    };
}

#endif // TINTBORDER_H
