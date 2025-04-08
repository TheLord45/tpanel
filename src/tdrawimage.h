/*
 * Copyright (C) 2021 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TDRAWIMAGE__
#define __TDRAWIMAGE__

#include <include/core/SkBitmap.h>

#include "tbutton.h"

class TDrawImage
{
    public:
        TDrawImage();
        ~TDrawImage();

        /**
         * \brief Draws an image used as the background of a page or subpage.
         * This method can draw a normal or a cameleon image. It is able to
         * detect which kind of image to draw.
         * If it is a TP5 panel, it has a bitmap stack to hold all images. In
         * this case the images of the stack are crunched together to one. Then,
         * if \b imageMi is set, a cameleon can be made.
         *
         * @param bm    A pointer to the target image. The new image will be
         * drawn on top of \p bm.
         * @param idx   In case of TP5 this is the bitmap index.
         *
         * @return If everything went well TRUE is returned. Otherwise FALSE.
         * If there was an error it is documented into the logfile if the
         * loglevel ERROR and WARNING was set.
         */
        bool drawImage(SkBitmap* bm, int idx=-1);

        void setInstance(int instance) { mInstance = instance; }    // Set the instance to use (always 0 for background images)
        int getInstance() { return mInstance; }                     // Get the instance in use
        void setSr(std::vector<Button::SR_T>& sr) { mSr = sr; }     // Set the page resource
        std::vector<Button::SR_T>& getSr() { return mSr; }          // Get the page resource
        void setImageMi(SkBitmap& mi) { imageMi = mi; }             // Set the optional image mask for a cameleon image
        SkBitmap& getImageMi() { return imageMi; }                  // Get the image mask
        void setImageBm(SkBitmap& bm);                              // Set the optional bitmap
        SkBitmap& getImageBm(size_t index=0);                       // Get the bitmap
        void setBorderSize(int bs) { mBorderSize = bs; }            // Set the optional border size, if there is one
        int getBorderSize() { return mBorderSize; }                 // Get the border size
        void setWidth(int wt) { mWidth = wt; }                      // Set the total width (width of page or subpage)
        int getWidth() { return mWidth; }                           // Get the total width
        void setHeight(int ht) { mHeight = ht; }                    // Set the total height (height of page or subpage)
        int getHeight() { return mHeight; }                         // Get the total height

    private:
        SkBitmap drawImageButton(SkBitmap& imgRed, SkBitmap& imgMask, int width, int height, SkColor col1, SkColor col2);
        SkColor baseColor(SkColor basePix, SkColor maskPix, SkColor col1, SkColor col2);
        Button::POSITION_t calcImagePosition(int width, int height, int number, int index=0);

        int mInstance{0};               // The instance
        int mBorderSize{0};             // Border size
        int mWidth{0};                  // Total width of page / subpage
        int mHeight{0};                 // Total height of page / subpage
        std::vector<Button::SR_T> mSr;  // Array with background definations
        SkBitmap imageMi;               // The mask image
        SkBitmap imageBm;               // The bitmap image
        std::vector<SkBitmap> mBitmapStack; // TP5: A bitmap stack.
};

#endif
