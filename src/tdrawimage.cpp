/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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

#include <include/core/SkBitmap.h>

#include "tdrawimage.h"
#include "tbutton.h"
#include "terror.h"
#include <qglobal.h>

extern bool prg_stopped;

using std::string;
using std::vector;

TDrawImage::TDrawImage()
{
    DECL_TRACER("TDrawImage::TDrawImage()");
}

TDrawImage::~TDrawImage()
{
    DECL_TRACER("TDrawImage::~TDrawImage()");
}

bool TDrawImage::drawImage(SkBitmap* bm)
{
    DECL_TRACER("TDrawImage::drawImage(SkBitmap* bm)");

    if (prg_stopped)
        return false;

    if (mSr.size() == 0)
    {
        MSG_ERROR("No SR information!");
        return false;
    }

    int instance = mInstance;

    if (mInstance < 0)
        instance = 0;
    else if ((size_t)mInstance >= mSr.size())
        instance = mSr.size() - 1;

    if (mSr[instance].dynamic)
    {
        MSG_WARNING("Dynamic images are not handled by TDrawImage!");
        return false;
    }

    if (!imageMi.empty() && !mSr[instance].mi.empty() && mSr[instance].bs.empty())       // Chameleon image?
    {
        MSG_TRACE("Chameleon image ...");
        SkBitmap imgRed(imageMi);
        SkBitmap imgMask;

        if (!imageBm.empty() && !mSr[instance].bm.empty())
            imgMask.installPixels(imageBm.pixmap());
        else
        {
            imgMask.allocN32Pixels(mSr[instance].mi_width, mSr[instance].mi_height);
            imgMask.eraseColor(SK_ColorTRANSPARENT);
        }

        SkBitmap img = drawImageButton(imgRed, imgMask, mSr[instance].mi_width, mSr[instance].mi_height, TColor::getSkiaColor(mSr[instance].cf), TColor::getSkiaColor(mSr[instance].cb));

        if (img.empty())
        {
            MSG_ERROR("Error creating the cameleon image \"" << mSr[instance].mi << "\" / \"" << mSr[instance].bm << "\"!");
            TError::SetError();
            return false;
        }

        SkCanvas ctx(img, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
        SkImageInfo info = img.info();
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        sk_sp<SkImage> _image = SkImages::RasterFromBitmap(imgMask);
        ctx.drawImage(_image, 0, 0, SkSamplingOptions(), &paint);

        Button::POSITION_t position = calcImagePosition(mSr[instance].mi_width, mSr[instance].mi_height, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image!");
            TError::SetError();
            return false;
        }

        SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));
        paint.setBlendMode(SkBlendMode::kSrc);

        if (mSr[instance].sb == 0)
        {
            sk_sp<SkImage> _image = SkImages::RasterFromBitmap(img);
            can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
        }
        else    // Scale to fit
        {
            SkRect rect = SkRect::MakeXYWH(position.left, position.top, position.width, position.height);
            sk_sp<SkImage> im = SkImages::RasterFromBitmap(img);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
        }
    }
    else if (!imageBm.empty() && !mSr[instance].bm.empty())
    {
        MSG_TRACE("Drawing normal image ...");
        SkBitmap image = imageBm;

        if (image.empty())
        {
            MSG_ERROR("Error creating the image \"" << mSr[instance].bm << "\"!");
            TError::SetError();
            return false;
        }

        Button::POSITION_t position = calcImagePosition(mSr[instance].bm_width, mSr[instance].bm_height, instance);

        if (!position.valid)
        {
            MSG_ERROR("Error calculating the position of the image.");
            TError::SetError();
            return false;
        }

        MSG_DEBUG("Putting bitmap on top of image ...");
        SkPaint paint;
        paint.setBlendMode(SkBlendMode::kSrcOver);
        SkCanvas can(*bm, SkSurfaceProps(1, kUnknown_SkPixelGeometry));

        if (mSr[instance].sb == 0)
        {
            if ((mSr[instance].jb == 0 && mSr[instance].bx >= 0 && mSr[instance].by >= 0) || mSr[instance].jb != 0)  // Draw the full image
            {
                MSG_DEBUG("Drawing full image ...");
                sk_sp<SkImage> _image = SkImages::RasterFromBitmap(image);
                can.drawImage(_image, position.left, position.top, SkSamplingOptions(), &paint);
            }
            else    // We need only a subset of the image
            {
                MSG_DEBUG("Create a subset of an image ...");

                // Create a new Info to have the size of the subset.
                SkImageInfo info = SkImageInfo::Make(position.width, position.height, kRGBA_8888_SkColorType, kPremul_SkAlphaType);
                size_t byteSize = info.computeMinByteSize();

                if (byteSize == 0)
                {
                    MSG_ERROR("Unable to calculate size of image!");
                    TError::SetError();
                    return false;
                }

                MSG_DEBUG("Rectangle of part: x: " << position.left << ", y: " << position.top << ", w: " << position.width << ", h: " << position.height);
                SkBitmap part;      // Bitmap receiving the wanted part from the whole image
                SkIRect irect = SkIRect::MakeXYWH(position.left, position.top, position.width, position.height);
                image.extractSubset(&part, irect);  // Extract the part of the image containg the pixels we want
                sk_sp<SkImage> _image = SkImages::RasterFromBitmap(part);
                can.drawImage(_image, 0, 0, SkSamplingOptions(), &paint); // Draw the image
            }
        }
        else    // Scale to fit
        {
            MSG_DEBUG("Scaling image to fit.");
            SkRect rect = SkRect::MakeXYWH(position.left, position.top, position.width, position.height);
            sk_sp<SkImage> im = SkImages::RasterFromBitmap(image);
            can.drawImageRect(im, rect, SkSamplingOptions(), &paint);
        }
    }
    else if ((imageBm.empty() && !mSr[0].bm.empty()) || (imageMi.empty() && !mSr[0].mi.empty()))
    {
        if (imageBm.empty())
        {
            MSG_ERROR("Image BM " << mSr[0].bm << " defined but got no image!");
        }

        if (imageMi.empty())
        {
            MSG_ERROR("Image MI " << mSr[0].mi << " defined but got no image!");
        }

        return false;
    }
    else
    {
        MSG_DEBUG("No bitmap defined.");
    }

    return true;
}

SkBitmap TDrawImage::drawImageButton(SkBitmap& imgRed, SkBitmap& imgMask, int width, int height, SkColor col1, SkColor col2)
{
    DECL_TRACER("TDrawImage::drawImageButton(SkImage& imgRed, SkImage& imgMask, int width, int height, SkColor col1, SkColor col2)");

    if (width <= 0 || height <= 0)
    {
        MSG_WARNING("Got invalid width of height! (width: " << width << ", height: " << height);
        return SkBitmap();
    }

    SkPixmap pixmapRed = imgRed.pixmap();
    SkPixmap pixmapMask;

    if (!imgMask.empty())
        pixmapMask = imgMask.pixmap();

    SkImageInfo maskPixInfo = SkImageInfo::MakeN32Premul(width, height);
    maskPixInfo.makeColorType(kRGBA_8888_SkColorType);
    SkBitmap maskBm;
    maskBm.allocPixels(SkImageInfo::MakeN32Premul(width, height));

    for (int ix = 0; ix < width; ix++)
    {
        for (int iy = 0; iy < height; iy++)
        {
            SkColor pixelRed = pixmapRed.getColor(ix, iy);
            SkColor pixelMask;

            if (!imgMask.empty())
                pixelMask = pixmapMask.getColor(ix, iy);
            else
                pixelMask = SK_ColorWHITE;

            SkColor pixel = baseColor(pixelRed, pixelMask, col1, col2);
            uint32_t alpha = SkColorGetA(pixel);
            uint32_t *wpix = maskBm.getAddr32(ix, iy);

            if (!wpix)
            {
                MSG_ERROR("No pixel buffer!");
                break;
            }

            if (alpha == 0)
                pixel = pixelMask;
            // Skia has a bug and has changed the red and the blue color
            // channel. Therefor we must change this 2 color channels for
            // Linux based OSs here. On Android this is not necessary.
//#ifdef __ANDROID__
            *wpix = pixel;
//#else   // We've to invert the pixels here to have the correct colors
//            uchar red   = SkColorGetR(pixel);   // This is blue in reality
//            uchar green = SkColorGetG(pixel);
//            uchar blue  = SkColorGetB(pixel);   // This is red in reality
//            uchar al    = SkColorGetA(pixel);
//            *wpix = SkColorSetARGB(al, blue, green, red);
//#endif
        }
    }

    return maskBm;
}

SkColor TDrawImage::baseColor(SkColor basePix, SkColor maskPix, SkColor col1, SkColor col2)
{
    uint alpha = SkColorGetA(basePix);
    uint green = SkColorGetG(basePix);
#ifndef __ANDROID__
    uint red = SkColorGetR(basePix);
#else
    uint red = SkColorGetB(basePix);
#endif

    if (alpha == 0)
        return maskPix;

    if (red && green)
    {
//#ifdef __ANDROID__
//        uint newB = (SkColorGetR(col1) + SkColorGetR(col2) / 2) & 0x0ff;
//        uint newG = (SkColorGetG(col1) + SkColorGetG(col2) / 2) & 0x0ff;
//        uint newR = (SkColorGetB(col1) + SkColorGetB(col2) / 2) & 0x0ff;
//#else
        uint newR = (SkColorGetR(col1) + SkColorGetR(col2) / 2) & 0x0ff;
        uint newG = (SkColorGetG(col1) + SkColorGetG(col2) / 2) & 0x0ff;
        uint newB = (SkColorGetB(col1) + SkColorGetB(col2) / 2) & 0x0ff;
//#endif
        uint newA = (SkColorGetA(col1) + SkColorGetA(col2) / 2) & 0x0ff;

        return SkColorSetARGB(newA, newR, newG, newB);
    }

    if (red)
        return col1;

    if (green)
        return col2;

    return SK_ColorTRANSPARENT; // transparent pixel
}

Button::POSITION_t TDrawImage::calcImagePosition(int width, int height, int number)
{
    DECL_TRACER("TDrawImage::calcImagePosition(int with, int height, CENTER_CODE code, int number)");

    Button::SR_T act_sr;
    Button::POSITION_t position;
    int ix, iy;

    if (mSr.size() == 0)
        return position;

    if (number <= 0)
        act_sr = mSr.at(0);
    else if ((size_t)number < mSr.size())
        act_sr = mSr.at(number);
    else if ((size_t)number >= mSr.size())
        act_sr = mSr.at(mSr.size() - 1);
    else
        return position;

    int border_size = mBorderSize;
    int code, border = border_size;
    string dbgCC;
    int rwt = 0, rht = 0;

    code = act_sr.jb;
    ix = act_sr.bx;
    iy = act_sr.by;
    rwt = std::min(mWidth - border * 2, width);
    rht = std::min(mHeight - border_size * 2, height);

    if (width > rwt || height > rht)
        position.overflow = true;

    switch (code)
    {
        case 0: // absolute position
            position.left = ix;
            position.top = iy;

            if (ix < 0 && rwt < width)
                position.left *= -1;

            position.width = rwt;
            position.height = rht;
        break;

        case 1: // top, left
            position.width = rwt;
            position.height = rht;
        break;

        case 2: // center, top
            position.left = (mWidth - rwt) / 2;
            position.height = rht;
            position.width = rwt;
        break;

        case 3: // right, top
            position.left = mWidth - rwt;
            position.width = rwt;
            position.height = rht;
            break;

        case 4: // left, middle
            position.top = (mHeight - rht) / 2;
            position.width = rwt;
            position.height = rht;
            break;

        case 6: // right, middle
            position.left = mWidth - rwt;
            position.top = (mHeight - rht) / 2;
            position.width = rwt;
            position.height = rht;
            break;

        case 7: // left, bottom
            position.top = mHeight - rht;
            position.width = rwt;
            position.height = rht;
            break;

        case 8: // center, bottom
            position.left = (mWidth - rwt) / 2;
            position.top = mHeight - rht;
            position.width = rwt;
            position.height = rht;
            break;

        case 9: // right, bottom
            position.left = mWidth - rwt;
            position.top = mHeight - rht;
            break;

        default: // center, middle
            position.left = (mWidth - rwt) / 2;
            position.top = (mHeight - rht) / 2;
            position.width = rwt;
            position.height = rht;
    }

    MSG_DEBUG("PosType=" << code << ", Position: x=" << position.left << ", y=" << position.top << ", w=" << position.width << ", h=" << position.height << ", Overflow: " << (position.overflow ? "YES" : "NO"));
    position.valid = true;
    return position;
}
