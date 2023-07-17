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
#include <string>
#include <vector>

#include <include/core/SkImage.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkPixmap.h>
#include <include/core/SkBitmap.h>
#include <include/core/SkSize.h>
#include <include/core/SkColor.h>
#include <include/core/SkSurfaceProps.h>
#include <include/core/SkRRect.h>

#include "tintborder.h"
#include "tcolor.h"
#include "tresources.h"
#include "terror.h"

using namespace Border;
using std::string;
using std::vector;

/**
 * The following table defines some of the system borders. It is mostly a
 * fallback table but defines whether a border should be calculated internaly
 * or constructed out of the images in the system border folder. The later is
 * only possible if this folder exist and contains the system images from AMX.
 * This images could be retrieved by sending a full surface with the system
 * files included from TPDesign4.
 * All borders not listed in this table are constructed out of the system
 * border images, if they exist.
 */
SYSBORDER_t sysBorders[] = {
//   ID   Name                  AMX number  Style          width radius calc
    {  1, (char *)"Single Line",         0, (char *)"solid",   1,   0, true },
    {  2, (char *)"Double Line",         0, (char *)"solid",   2,   0, true },
    {  3, (char *)"Quad Line",           0, (char *)"solid",   4,   0, true },
    {  4, (char *)"Picture Frame",       0, (char *)"double",  0,   0, false },
    {  5, (char *)"Circle 15",           8, (char *)"solid",   2,   7, true },
    {  6, (char *)"Circle 25",           9, (char *)"solid",   2,  14, true },
    {  7, (char *)"Circle 35",          10, (char *)"solid",   2,  21, true },
    {  8, (char *)"Circle 45",          11, (char *)"solid",   2,  28, true },
    {  9, (char *)"Circle 55",          12, (char *)"solid",   2,  35, true },
    { 10, (char *)"Circle 65",          13, (char *)"solid",   2,  42, true },
    { 11, (char *)"Circle 75",          14, (char *)"solid",   2,  49, true },
    { 12, (char *)"Circle 85",          15, (char *)"solid",   2,  56, true },
    { 13, (char *)"Circle 95",          16, (char *)"solid",   2,  63, true },
    { 14, (char *)"Circle 105",         17, (char *)"solid",   2,  70, true },
    { 15, (char *)"Circle 115",         18, (char *)"solid",   2,  77, true },
    { 16, (char *)"Circle 125",         19, (char *)"solid",   2,  84, true },
    { 17, (char *)"Circle 135",         20, (char *)"solid",   2,  91, true },
    { 18, (char *)"Circle 145",         21, (char *)"solid",   2,  98, true },
    { 19, (char *)"Circle 155",         22, (char *)"solid",   2, 105, true },
    { 20, (char *)"Circle 165",         23, (char *)"solid",   2, 112, true },
    { 21, (char *)"Circle 175",         24, (char *)"solid",   2, 119, true },
    { 22, (char *)"Circle 185",         25, (char *)"solid",   2, 126, true },
    { 23, (char *)"Circle 195",         26, (char *)"solid",   2, 133, true },
    { 24, (char *)"AMX Elite Inset -L",  0, (char *)"groove", 20,   0, false },
    { 25, (char *)"AMX Elite Raised -L", 0, (char *)"ridge",  20,   0, false },
    { 26, (char *)"AMX Elite Inset -M",  0, (char *)"groove", 10,   0, false },
    { 27, (char *)"AMX Elite Raised -M", 0, (char *)"ridge",  10,   0, false },
    { 28, (char *)"AMX Elite Inset -S",  0, (char *)"groove",  4,   0, false },
    { 29, (char *)"AMX Elite Raised -S", 0, (char *)"ridge",   4,   0, false },
    { 30, (char *)"Bevel Inset -L",      0, (char *)"inset",  20,   0, false },
    { 31, (char *)"Bevel Raised -L",     0, (char *)"outset", 20,   0, false },
    { 32, (char *)"Bevel Inset -M",      0, (char *)"inset",  10,   0, false },
    { 33, (char *)"Bevel Raised -M",     0, (char *)"outset", 10,   0, false },
    { 34, (char *)"Bevel Inset -S",      0, (char *)"inset",   4,   0, false },
    { 35, (char *)"Bevel Raised -S",     0, (char *)"outset",  4,   0, false },
    {  0, nullptr, 0, nullptr, 0, 0 }
};

TP4BORDERS_t tp4Borders[] = {
    { 0, "None", {0, 0, 0, 0} },
    { 1, "None", {0, 0, 0, 0} },
    { 2, "AMX Elite -L", {0, 0, 0, 0} },
    { 3, "AMX Elite -M", {0, 0, 0, 0} },
    { 4, "AMX Elite -S", {0, 0, 0, 0} },
    { 5, "Bevel -L", {0, 0, 0, 0} },
    { 6, "Bevel -M", {21, 0, 0, 0} },
    { 7, "Bevel -S", {20, 0, 0, 0} },
    { 8, "Circle 15", {5, 22, 0, 0} },
    { 9, "Circle 25", {6, 23, 0, 0} },
    { 10, "Circle 35", {0, 0, 0, 0} },
    { 11, "Circle 45", {0, 0, 0, 0} },
    { 12, "Circle 55", {0, 0, 0, 0} },
    { 13, "Circle 65", {0, 0, 0, 0} },
    { 14, "Circle 75", {0, 0, 0, 0} },
    { 15, "Circle 85", {0, 0, 0, 0} },
    { 16, "Circle 95", {0, 0, 0, 0} },
    { 17, "Circle 105", {0, 0, 0, 0} },
    { 18, "Circle 115", {0, 0, 0, 0} },
    { 19, "Circle 125", {0, 0, 0, 0} },
    { 20, "Circle 135", {0, 0, 0, 0} },
    { 21, "Circle 145", {0, 0, 0, 0} },
    { 22, "Circle 155", {0, 0, 0, 0} },
    { 23, "Circle 165", {0, 0, 0, 0} },
    { 24, "Circle 175", {0, 0, 0, 0} },
    { 25, "Circle 185", {0, 0, 0, 0} },
    { 26, "Circle 195", {0, 0, 0, 0} },
    { 27, "Cursor Bottom", {0, 0, 0, 0} },
    { 28, "Cursor Bottom with Hole", {0, 0, 0, 0} },
    { 29, "Cursor Top", {0, 0, 0, 0} },
    { 30, "Cursor Top with Hole", {0, 0, 0, 0} },
    { 31, "Cursor Left", {0, 0, 0, 0} },
    { 32, "Cursor Left with Hole", {0, 0, 0, 0} },
    { 33, "Cursor Right", {0, 0, 0, 0} },
    { 34, "Cursor Right with Hole", {0, 0, 0, 0} },
    { 35, "Custom Frame", {0, 0, 0, 0} },
    { 36, "Diamond 15", {0, 0, 0, 0} },
    { 37, "Diamond 25", {0, 0, 0, 0} },
    { 38, "Diamond 35", {0, 0, 0, 0} },
    { 39, "Diamond 45", {0, 0, 0, 0} },
    { 40, "Diamond 55", {40, 0, 0, 0} },
    { 41, "Diamond 65", {41, 0, 0, 0} },
    { 42, "Diamond 75", {0, 0, 0, 0} },
    { 43, "Diamond 85", {0, 0, 0, 0} },
    { 44, "Diamond 95", {0, 0, 0, 0} },
    { 45, "Diamond 105", {0, 0, 0, 0} },
    { 46, "Diamond 115", {0, 0, 0, 0} },
    { 47, "Diamond 125", {0, 0, 0, 0} },
    { 48, "Diamond 135", {0, 0, 0, 0} },
    { 49, "Diamond 145", {0, 0, 0, 0} },
    { 50, "Diamond 155", {0, 0, 0, 0} },
    { 51, "Diamond 165", {0, 0, 0, 0} },
    { 52, "Diamond 175", {0, 0, 0, 0} },
    { 53, "Diamond 185", {0, 0, 0, 0} },
    { 54, "Diamond 195", {0, 0, 0, 0} },
    { 55, "Double Bevel -L", {0, 0, 0, 0} },
    { 56, "Double Bevel -M", {0, 0, 0, 0} },
    { 57, "Double Bevel -S", {0, 0, 0, 0} },
    { 58, "Double Line", {3, 8, 12, 0} },
    { 59, "Fuzzy", {0, 0, 0, 0} },
    { 60, "Glow-L", {0, 0, 0, 0} },
    { 61, "Glow-S", {0, 0, 0, 0} },
    { 62, "Help Down", {0, 0, 0, 0} },
    { 63, "Neon Active -L", {27, 0, 0, 0} },
    { 64, "Neon Active -S", {26, 0, 0, 0} },
    { 65, "Neon Inactive -L", {25, 0, 0, 0} },
    { 66, "Neon Inactive -S", {24, 0, 0, 0} },
    { 67, "Oval H 60x30", {0, 0, 0, 0} },
    { 68, "Oval H 100x50", {0, 0, 0, 0} },
    { 69, "Oval H 150x75", {0, 0, 0, 0} },
    { 70, "Oval H 200x100", {0, 0, 0, 0} },
    { 71, "Oval V 30x60", {0, 0, 0, 0} },
    { 72, "Oval V 50x100", {0, 0, 0, 0} },
    { 73, "Oval V 75x150", {0, 0, 0, 0} },
    { 74, "Oval V 100x200", {0, 0, 0, 0} },
    { 75, "Picture Frame", {10, 11, 0, 0} },
    { 76, "Quad Line", {4, 9, 0, 0} },
    { 77, "Single Line", {2, 7, 0, 0} },
    { 78, "Windows Style Popup", {0, 0, 0, 0} },
    { 79, "Windows Style Popup (Status Bar)", {0, 0, 0, 0} },
    { 80, "Menu Bottom Rounded 15", {0, 0, 0, 0} },
    { 81, "Menu Bottom Rounded 25", {0, 0, 0, 0} },
    { 82, "Menu Bottom Rounded 35", {0, 0, 0, 0} },
    { 83, "Menu Bottom Rounded 45", {0, 0, 0, 0} },
    { 84, "Menu Bottom Rounded 55", {0, 0, 0, 0} },
    { 85, "Menu Bottom Rounded 65", {0, 0, 0, 0} },
    { 86, "Menu Bottom Rounded 75", {0, 0, 0, 0} },
    { 87, "Menu Bottom Rounded 85", {0, 0, 0, 0} },
    { 88, "Menu Bottom Rounded 95", {0, 0, 0, 0} },
    { 89, "Menu Bottom Rounded 105", {0, 0, 0, 0} },
    { 90, "Menu Bottom Rounded 115", {0, 0, 0, 0} },
    { 91, "Menu Bottom Rounded 125", {0, 0, 0, 0} },
    { 92, "Menu Bottom Rounded 135", {0, 0, 0, 0} },
    { 93, "Menu Bottom Rounded 145", {0, 0, 0, 0} },
    { 94, "Menu Bottom Rounded 155", {0, 0, 0, 0} },
    { 95, "Menu Bottom Rounded 165", {0, 0, 0, 0} },
    { 96, "Menu Bottom Rounded 175", {0, 0, 0, 0} },
    { 97, "Menu Bottom Rounded 185", {0, 0, 0, 0} },
    { 98, "Menu Bottom Rounded 195", {0, 0, 0, 0} },
    { 99, "Menu Top Rounded 15", {0, 0, 0, 0} },
    { 100, "Menu Top Rounded 25", {0, 0, 0, 0} },
    { 101, "Menu Top Rounded 35", {0, 0, 0, 0} },
    { 102, "Menu Top Rounded 45", {0, 0, 0, 0} },
    { 103, "Menu Top Rounded 55", {0, 0, 0, 0} },
    { 104, "Menu Top Rounded 65", {0, 0, 0, 0} },
    { 105, "Menu Top Rounded 75", {0, 0, 0, 0} },
    { 106, "Menu Top Rounded 85", {0, 0, 0, 0} },
    { 107, "Menu Top Rounded 95", {0, 0, 0, 0} },
    { 108, "Menu Top Rounded 105", {0, 0, 0, 0} },
    { 109, "Menu Top Rounded 115", {0, 0, 0, 0} },
    { 110, "Menu Top Rounded 125", {0, 0, 0, 0} },
    { 111, "Menu Top Rounded 135", {0, 0, 0, 0} },
    { 112, "Menu Top Rounded 145", {0, 0, 0, 0} },
    { 113, "Menu Top Rounded 155", {0, 0, 0, 0} },
    { 114, "Menu Top Rounded 165", {0, 0, 0, 0} },
    { 115, "Menu Top Rounded 175", {0, 0, 0, 0} },
    { 116, "Menu Top Rounded 185", {0, 0, 0, 0} },
    { 117, "Menu Top Rounded 195", {0, 0, 0, 0} },
    { 118, "Menu Right Rounded 15", {0, 0, 0, 0} },
    { 119, "Menu Right Rounded 25", {0, 0, 0, 0} },
    { 120, "Menu Right Rounded 35", {0, 0, 0, 0} },
    { 121, "Menu Right Rounded 45", {0, 0, 0, 0} },
    { 122, "Menu Right Rounded 55", {0, 0, 0, 0} },
    { 123, "Menu Right Rounded 65", {0, 0, 0, 0} },
    { 124, "Menu Right Rounded 75", {0, 0, 0, 0} },
    { 125, "Menu Right Rounded 85", {0, 0, 0, 0} },
    { 126, "Menu Right Rounded 95", {0, 0, 0, 0} },
    { 127, "Menu Right Rounded 105", {0, 0, 0, 0} },
    { 128, "Menu Right Rounded 115", {0, 0, 0, 0} },
    { 129, "Menu Right Rounded 125", {0, 0, 0, 0} },
    { 130, "Menu Right Rounded 135", {0, 0, 0, 0} },
    { 131, "Menu Right Rounded 145", {0, 0, 0, 0} },
    { 132, "Menu Right Rounded 155", {0, 0, 0, 0} },
    { 133, "Menu Right Rounded 165", {0, 0, 0, 0} },
    { 134, "Menu Right Rounded 175", {0, 0, 0, 0} },
    { 135, "Menu Right Rounded 185", {0, 0, 0, 0} },
    { 136, "Menu Right Rounded 195", {0, 0, 0, 0} },
    { 137, "Menu Left Rounded 15", {0, 0, 0, 0} },
    { 138, "Menu Left Rounded 25", {0, 0, 0, 0} },
    { 139, "Menu Left Rounded 35", {0, 0, 0, 0} },
    { 140, "Menu Left Rounded 45", {0, 0, 0, 0} },
    { 141, "Menu Left Rounded 55", {0, 0, 0, 0} },
    { 142, "Menu Left Rounded 65", {0, 0, 0, 0} },
    { 143, "Menu Left Rounded 75", {0, 0, 0, 0} },
    { 144, "Menu Left Rounded 85", {0, 0, 0, 0} },
    { 145, "Menu Left Rounded 95", {0, 0, 0, 0} },
    { 146, "Menu Left Rounded 105", {0, 0, 0, 0} },
    { 147, "Menu Left Rounded 115", {0, 0, 0, 0} },
    { 148, "Menu Left Rounded 125", {0, 0, 0, 0} },
    { 149, "Menu Left Rounded 135", {0, 0, 0, 0} },
    { 150, "Menu Left Rounded 145", {0, 0, 0, 0} },
    { 151, "Menu Left Rounded 155", {0, 0, 0, 0} },
    { 152, "Menu Left Rounded 165", {0, 0, 0, 0} },
    { 153, "Menu Left Rounded 175", {0, 0, 0, 0} },
    { 154, "Menu Left Rounded 185", {0, 0, 0, 0} },
    { 155, "Menu Left Rounded 195", {0, 0, 0, 0} },
    { -1, "", {0, 0, 0, 0}}
};

TIntBorder::TIntBorder()
{
    DECL_TRACER("TIntBorder::TIntBorder()");
}

bool TIntBorder::drawBorder(SkBitmap* bm, const string& bname, int wt, int ht, const string& cb, bool force)
{
    DECL_TRACER("TIntBorder::drawBorder(SkBitmap* bm, string& bname, int wt, int ht, string& cb, bool force)");

    if (!bm || wt <= 0 || ht <= 0)
        return false;

    if (bname.empty())
    {
        MSG_DEBUG("No border name defined.");
        return false;
    }

    // Try to find the border in the system table
    int borderIndex = -1;
    int i = 0;

    while (sysBorders[i].id)
    {
        if (strCaseCompare(bname, sysBorders[i].name) == 0)
        {
            if (!force && !sysBorders[i].calc)
            {
                MSG_DEBUG("Ignoring border " << bname << " because it was not forced.");
                return false;
            }

            borderIndex = i;
            MSG_DEBUG("Found internal system border [" << i << "]: " << sysBorders[i].name);
            break;
        }

        i++;
    }

    if (borderIndex < 0)
    {
        MSG_DEBUG(bname << " is not an internal border.");
        return false;
    }

    MSG_DEBUG("Border " << sysBorders[borderIndex].name << " found.");
    SkCanvas canvas(*bm, SkSurfaceProps());
    SkPaint paint;
    SkColor color = TColor::getSkiaColor(cb);

    paint.setColor(color);
    paint.setBlendMode(SkBlendMode::kSrc);
    paint.setStyle(SkPaint::kStroke_Style);
    SkRRect outher, inner;
    SkScalar radius = (SkScalar)sysBorders[borderIndex].radius;
    int red, green, blue;
    SkColor borderColor, bcLight, bcDark;
    int lineWidth = 0;

    switch (sysBorders[borderIndex].id)
    {
        case 1: // Single Frame
        case 2: // Double Frame
        case 3: // Quad Frame
            paint.setStrokeWidth(sysBorders[borderIndex].width);
            canvas.drawRect(calcRect(wt, ht, sysBorders[borderIndex].width), paint);
        break;

        case 4: // Picture Frame
            {
                paint.setStrokeWidth(2);
                SkRect rect = SkRect::MakeXYWH(0, 0, wt, ht);
                canvas.drawRect(rect, paint);
                rect = SkRect::MakeXYWH(4, 4, wt - 4, ht - 4);
                canvas.drawRect(rect, paint);
            }
        break;

        case 5: // Circle 15
        case 6: // Circle 25
        case 7: // Circle 35
        case 8: // Circle 45
        case 9: // Circle 55
        case 10: // Circle 65
        case 11: // Circle 75
        case 12: // Circle 85
        case 13: // Circle 95
        case 14: // Circle 105
        case 15: // Circle 115
        case 16: // Circle 125
        case 17: // Circle 135
        case 18: // Circle 145
        case 19: // Circle 155
        case 20: // Circle 165
        case 21: // Circle 175
        case 22: // Circle 185
        case 23: // Circle 195
            lineWidth = sysBorders[borderIndex].width;
            paint.setStrokeWidth(0.1);
            paint.setStyle(SkPaint::kFill_Style);
            MSG_DEBUG("Line width: " << lineWidth << ", radius: " << radius);
            // We draw a rounded rectangle to "clip" the corners. To do this
            // in a way to not miss any pixel, we draw a rectangle followed
            // by a rounded rectangle as an inner one. The space between
            // them will be filled transparent.
            outher = SkRRect::MakeRect({0, 0, (SkScalar)wt, (SkScalar)ht});
            inner = SkRRect::MakeRectXY(calcRect(wt, ht, 1), radius, radius);
            paint.setColor(SK_ColorTRANSPARENT);
            canvas.drawDRRect(outher, inner, paint);
            // Here we draw the rounded rectangle.
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(lineWidth);
            paint.setColor(color);
            paint.setStrokeJoin(SkPaint::kRound_Join);
            canvas.drawRoundRect(calcRect(wt, ht, lineWidth), radius, radius, paint);
        break;

        case 24:    // AMX Elite Inset -L
        case 26:    // AMX Elite Inset -M
        case 28:    // AMX Elite Inset -S
            {
                borderColor = TColor::getSkiaColor(cb);
                vector<SkColor> cols = TColor::colorRange(borderColor, sysBorders[borderIndex].width, 40, TColor::DIR_LIGHT_DARK_LIGHT);
                vector<SkColor>::iterator iter;
                i = 0;

                for (iter = cols.begin(); iter != cols.end(); ++iter)
                {
                    paint.setStrokeWidth(1);
                    paint.setColor(*iter);
                    SkRect rect = SkRect::MakeXYWH(i, i, wt - i, ht - i);
                    canvas.drawRect(rect, paint);
                    i++;
                }
            }
        break;

        case 25:    // AMX Elite Raised -L
        case 27:    // AMX Elite Raised -M
        case 29:    // AMX Elite Raised -S
        {
            borderColor = TColor::getSkiaColor(cb);
            vector<SkColor> cols = TColor::colorRange(borderColor, sysBorders[borderIndex].width, 40, TColor::DIR_DARK_LIGHT_DARK);
            vector<SkColor>::iterator iter;
            i = 0;

            for (iter = cols.begin(); iter != cols.end(); ++iter)
            {
                paint.setStrokeWidth(1);
                paint.setColor(*iter);
                SkRect rect = SkRect::MakeXYWH(i, i, wt - i, ht - i);
                canvas.drawRect(rect, paint);
                i++;
            }
        }
        break;

        case 30:    // Bevel Inset -L
        case 32:    // Bevel Inset -M
        case 34:    // Bevel Inset -S
            borderColor = TColor::getSkiaColor(cb);
            red = std::min((int)SkColorGetR(borderColor) + 20, 255);
            green = std::min((int)SkColorGetG(borderColor) + 20, 255);
            blue = std::min((int)SkColorGetB(borderColor) + 20, 255);
            bcLight = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
            red = std::max((int)SkColorGetR(borderColor) - 20, 0);
            green = std::max((int)SkColorGetG(borderColor) - 20, 0);
            blue = std::max((int)SkColorGetB(borderColor) - 20, 0);
            bcDark = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
            paint.setStrokeWidth(1);
            paint.setColor(bcDark);
            // Lines on the left
            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int yt = i;
                int yb = ht - i;
                canvas.drawLine(i, yt, i, yb, paint);
            }
            // Lines on the top
            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int xl = i;
                int xr = wt - i;
                canvas.drawLine(xl, i, xr, i, paint);
            }
            // Lines on right side
            paint.setColor(bcLight);

            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int yt = i;
                int yb = ht - i;
                canvas.drawLine(wt - i, yt, wt - i, yb, paint);
            }
            // Lines on bottom
            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int xl = i;
                int xr = wt - i;
                canvas.drawLine(xl, ht - i, xr, ht - i, paint);
            }
        break;

        case 31:    // Bevel Raised _L
        case 33:    // Bevel Raised _M
        case 35:    // Bevel Raised _S
            borderColor = TColor::getSkiaColor(cb);
            red = std::min((int)SkColorGetR(borderColor) + 10, 255);
            green = std::min((int)SkColorGetG(borderColor) + 10, 255);
            blue = std::min((int)SkColorGetB(borderColor) + 10, 255);
            bcLight = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
            red = std::max((int)SkColorGetR(borderColor) - 10, 0);
            green = std::max((int)SkColorGetG(borderColor) - 10, 0);
            blue = std::max((int)SkColorGetB(borderColor) - 10, 0);
            bcDark = SkColorSetARGB(SkColorGetA(borderColor), red, green, blue);
            paint.setStrokeWidth(1);
            paint.setColor(bcLight);
            // Lines on the left
            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int yt = i;
                int yb = ht - i;
                canvas.drawLine(i, yt, i, yb, paint);
            }
            // Lines on the top
            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int xl = i;
                int xr = wt - i;
                canvas.drawLine(xl, i, xr, i, paint);
            }
            // Lines on right side
            paint.setColor(bcDark);

            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int yt = i;
                int yb = ht - i;
                canvas.drawLine(wt - i, yt, wt - i, yb, paint);
            }
            // Lines on bottom
            for (i = 0; i < sysBorders[borderIndex].width; i++)
            {
                int xl = i;
                int xr = wt - borderIndex;
                canvas.drawLine(xl, ht - i, xr, ht - i, paint);
            }
        break;

        default:
            return false;
    }

    return true;
}

int TIntBorder::getBorderWidth(const string& bname, bool force)
{
    DECL_TRACER("TIntBorder::getBorderWidth(const string& bname, bool force)");

    for (int i = 0; sysBorders[i].id > 0; ++i)
    {
        if (strCaseCompare(bname, sysBorders[i].name) == 0)
        {
            if (!force && !sysBorders[i].calc)
                return 0;

            return sysBorders[i].width;
        }
    }

    return 0;
}

bool TIntBorder::borderExist(const string& name)
{
    DECL_TRACER("TIntBorder::borderExist(const string& name)");

    for (int i = 0; sysBorders[i].id > 0; ++i)
    {
        if (strCaseCompare(name, sysBorders[i].name) == 0)
            return true;
    }

    return false;
}

bool TIntBorder::borderExist(int index)
{
    DECL_TRACER("TIntBorder::borderExist(int index)");

    for (int i = 0; sysBorders[i].id > 0; ++i)
    {
        if (index == sysBorders[i].id)
            return true;
    }

    return false;
}

string TIntBorder::getBorderName(int index)
{
    DECL_TRACER("TIntBorder::getBorderName(int index)");

    for (int i = 0; sysBorders[i].id > 0; ++i)
    {
        if (sysBorders[i].id == index)
            return sysBorders[i].name;
    }

    return string();
}

string TIntBorder::getCorrectName(const string& name)
{
    DECL_TRACER("TIntBorder::getCorrectName(const string& name)");

    for (int i = 0; sysBorders[i].id > 0; ++i)
    {
        if (strCaseCompare(name, sysBorders[i].name) == 0)
            return sysBorders[i].name;
    }

    return string();
}

bool TIntBorder::isForcedBorder(const string& name)
{
    DECL_TRACER("TIntBorder::isForcedBorder(const string& name)");

    for (int i = 0; sysBorders[i].id > 0; ++i)
    {
        if (strCaseCompare(name, sysBorders[i].name) == 0)
            return sysBorders[i].calc;
    }

    return false;
}

bool TIntBorder::isTP4BorderValid(const string &name)
{
    DECL_TRACER("TIntBorder::isTP4BorderValid(const string &name)");

    if (name.empty())
        return false;

    int i = 0;

    while (tp4Borders[i].id >= 0)
    {
        if (strCaseCompare(tp4Borders[i].name, name) == 0)
            return true;

        i++;
    }

    return false;
}

string TIntBorder::getTP4BorderName(int id)
{
    DECL_TRACER("TIntBorder::getTP4BorderName(int id)");

    if (id < 1 || id > 41)
        return "None";

    int i = 0;

    while (tp4Borders[i].id >= 0)
    {
        int j = 0;

        while (j < 4)
        {
            if (tp4Borders[i].prgNum[j] == id)
                return tp4Borders[i].name;

            j++;
        }

        i++;
    }

    return "None";
}

void TIntBorder::erasePart(SkBitmap *bm, const SkBitmap& mask, ERASE_PART_t ep)
{
    DECL_TRACER("TIntBorder::erasePart(SkBitmap *bm, const SkBitmap& mask, ERASE_PART_t ep)");

    if (!bm || bm->empty() || ep == ERASE_NONE)
        return;

    int x, y;
    int width = bm->info().width();
    int height = bm->info().height();

    switch(ep)
    {
        case ERASE_LEFT_RIGHT:
            for (y = 0; y < height; ++y)
            {
                bool barrier = false;

                for (x = 0; x < width; ++x)
                {
                    uint32_t *wpix = bm->getAddr32(x, y);
                    uint32_t color = mask.getColor(x, y);
                    barrier = setPixel(wpix, color, barrier);
                }
            }
        break;

        case ERASE_RIGHT_LEFT:
            for (y = 0; y < height; ++y)
            {
                bool barrier = false;

                for (x = width - 1; x > 0; --x)
                {
                    uint32_t *wpix = bm->getAddr32(x, y);
                    uint32_t color = mask.getColor(x, y);
                    barrier = setPixel(wpix, color, barrier);
                }
            }
        break;

        case ERASE_TOP_DOWN:
            for (x = 0; x < width; ++x)
            {
                bool barrier = false;

                for (y = 0; y < height; ++y)
                {
                    uint32_t *wpix = bm->getAddr32(x, y);
                    uint32_t color = mask.getColor(x, y);
                    barrier = setPixel(wpix, color, barrier);
                }
            }
        break;

        case ERASE_BOTTOM_UP:
            for (x = 0; x < width; ++x)
            {
                bool barrier = false;

                for (y = height - 1; y > 0; --y)
                {
                    uint32_t *wpix = bm->getAddr32(x, y);
                    uint32_t color = mask.getColor(x, y);
                    barrier = setPixel(wpix, color, barrier);
                }
            }
        break;

        case ERASE_OUTSIDE:
            for (y = 0; y < height; ++y)
            {
                SkColor color = 0;

                for (x = 0; x < width; ++x)
                {
                    SkColor *wpix = bm->getAddr32(x, y);
                    SkColor alpha = SkColorGetA(mask.getColor(x, y));

                    if (alpha == 0)
                        color = SK_ColorTRANSPARENT;
                    else
                        break;

                    *wpix = color;
                }

                for (x = width - 1; x >= 0; --x)
                {
                    SkColor *wpix = bm->getAddr32(x, y);;
                    SkColor alpha = SkColorGetA(mask.getColor(x, y));

                    if (alpha == 0)
                        color = SK_ColorTRANSPARENT;
                    else
                        break;

                    *wpix = color;
                }
            }
        break;

        default:
            return;
    }
}

bool TIntBorder::setPixel(uint32_t *wpix, uint32_t col, bool bar)
{
//    DECL_TRACER("TIntBorder::setPixel(uint32_t *wpix, uint32_t col, bool bar)");

    uint32_t alpha = SkColorGetA(col);
    uint32_t color = col;
    bool barrier = bar;

    if (alpha == 0 && !barrier)
        color = SK_ColorTRANSPARENT;
    else if (alpha > 0 && !barrier)
    {
        barrier = true;
        int red = SkColorGetR(*wpix);
        int green = SkColorGetG(*wpix);
        int blue = SkColorGetB(*wpix);
        color = SkColorSetARGB(alpha, red, green, blue);
//        color = *wpix;
    }
    else if (barrier)
        color = *wpix;

    *wpix = color;
    return barrier;
}

void TIntBorder::colorizeFrame(SkBitmap *frame, SkColor color)
{
    DECL_TRACER("TIntBorder::colorizeFrame(SkBitmap *frame, SkColor color)");

    if (!frame || frame->empty())
        return;

    int red   = SkColorGetR(color);
    int green = SkColorGetG(color);
    int blue  = SkColorGetB(color);

    for (int y = 0; y < frame->info().height(); ++y)
    {
        for (int x = 0; x < frame->info().width(); ++x)
        {
            SkColor *wpix = frame->getAddr32(x, y);
            int alpha = SkColorGetA(*wpix);

            if (alpha > 0)
            {
                int wpred = SkColorGetR(*wpix);
                int wpgreen = SkColorGetG(*wpix);
                int wpblue = SkColorGetB(*wpix);

                if (wpred == 0 && wpgreen == 0 && wpblue == 0)  // black?
                    continue;
                else
                    *wpix = SkColorSetARGB(alpha, red, green, blue);

            }
            else
                *wpix = SK_ColorTRANSPARENT;
        }
    }
}

SkRect TIntBorder::calcRect(int width, int height, int pen)
{
    DECL_TRACER("TIntBorder::calcRect(int width, int height, int pen)");
    SkRect rect;

    SkScalar left = (SkScalar)pen / 2.0;
    SkScalar top = (SkScalar)pen / 2.0;
    SkScalar w = (SkScalar)width - (SkScalar)pen;
    SkScalar h = (SkScalar)height - (SkScalar)pen;
    rect.setXYWH(left, top, w, h);
    return rect;
}

