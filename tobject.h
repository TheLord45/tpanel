/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
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

#ifndef __TOBJECT_H__
#define __TOBJECT_H__

#include <string>
#include <QtGlobal>
#include <QtWidgets>
#include "tsubpage.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QWidget;
class QTextEdit;
class QLineEdit;
class QMediaPlayer;
class QVideoWidget;
class QPropertyAnimation;
QT_END_NAMESPACE

class TObject
{
    public:
        typedef enum OBJECT_TYPE
        {
            OBJ_NONE,
            OBJ_PAGE,
            OBJ_SUBPAGE,
            OBJ_BUTTON,
            OBJ_TEXT,
            OBJ_INPUT,
            OBJ_VIDEO
        }OBJECT_TYPE;

        typedef union _OBJ
        {
            QVideoWidget *vwidget{nullptr}; // A video window
            QLabel *label;                  // For buttons
            QWidget *widget;                // For subpage
            QTextEdit *multitext;           // For multiple text lines with input
            QLineEdit *linetext;            // For single input lines
        }_OBJ;

        typedef struct OBJECT_t
        {
            OBJECT_TYPE type{OBJ_NONE};
            ulong handle{0};
            _OBJ object;
            QMediaPlayer *player{nullptr};  // Pointer to video player if this is a video button
            int left{0};
            int top{0};
            int width{0};
            int height{0};
            QPropertyAnimation *animation{nullptr};
            ANIMATION_t animate;
            bool aniDirection{false};
            WId wid{0};                     // Used to identify a QTextEdit or QLineEdit
            std::atomic<bool> remove{false};// Object is marked for remove. Used with animation.
            OBJECT_t *next{nullptr};
        }OBJECT_t;

        TObject();
        ~TObject();

        void clear(bool force=false);
        OBJECT_t *addObject();
        OBJECT_t *findObject(ulong handle);
        OBJECT_t *findObject(WId id);
        OBJECT_t *findFirstChild(ulong handle);
        OBJECT_t *findNextChild(ulong handle);
        OBJECT_t *getMarkedRemove();
        OBJECT_t *getNextMarkedRemove(OBJECT_t *obj);
        OBJECT_t *findFirstWindow();
        OBJECT_t *findNextWindow(OBJECT_t *obj);
        void cleanMarked();
        void removeAllChilds(ulong handle);
        void removeObject(ulong handle);

        static std::string handleToString(ulong handle)
        {
            ulong part1 = (handle >> 16) & 0x0000ffff;
            ulong part2 = handle & 0x0000ffff;
            return std::to_string(part1)+":"+std::to_string(part2);
        }

        static std::string objectToString(OBJECT_TYPE o);
        void dropContent(OBJECT_t *obj);

    private:
        OBJECT_t *mObject{nullptr};
};

#endif
