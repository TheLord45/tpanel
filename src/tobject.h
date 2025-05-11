/*
 * Copyright (C) 2020 to 2025 by Andreas Theofilu <andreas@theosys.at>
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

#include "tpageinterface.h"
//#include "tqtextedit.h"
#include "tqeditline.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QWidget;
//class QTextEdit;
//class QLineEdit;
class QMediaPlayer;
class QVideoWidget;
class QPropertyAnimation;
class QListWidget;
class TQScrollArea;
class MainWindow;
class TQMarquee;
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
            OBJ_MARQUEE,
            OBJ_TEXT,
            OBJ_INPUT,
            OBJ_LIST,
            OBJ_VIDEO,
            OBJ_SUBVIEW
        }OBJECT_TYPE;

        typedef union _OBJ
        {
//            QVideoWidget *vwidget{nullptr}; // A video window
            QLabel *label{nullptr};         // For buttons
            TQMarquee *marquee;             // For marquee lines
            QWidget *widget;                // For subpage
            TQEditLine *plaintext;          // For text input
//            TQTextEdit *plaintext;          // For text input
            QListWidget *list;              // For lists
            TQScrollArea *area;             // For scroll area
        }_OBJ;

        typedef struct OBJECT_t
        {
            OBJECT_TYPE type{OBJ_NONE};     // The type of the object.
            ulong handle{0};                // The handle of the object. This the primary key!
            _OBJ object;                    // Union holding the pointer to the object
            QVideoWidget *vwidget{nullptr}; // Pointer to a video widget. Used to play videos
            QMediaPlayer *player{nullptr};  // Pointer to video player if this is a video button
            QUrl videoUrl;                  // Holds the URL used for the button if it is of type OBJ_VIDEO.
            int left{0};                    // Left position in relation to main page
            int top{0};                     // Top position in relation to main page
            int width{0};                   // Width of object
            int height{0};                  // Height of object
            int rows{0};                    // If the object is a list, this are the number of rows.
            int cols{0};                    // If the object is a list, this are the number of columns.
            bool invalid{true};             // FALSE = Object is valid
            QPropertyAnimation *animation{nullptr}; // Pointer to the class doing the animation
            ANIMATION_t animate;            // Holds information for animation
            bool aniDirection{false};       // TRUE = animation in progress
            WId wid{0};                     // Used to identify a QTextEdit or QLineEdit
            bool remove{false};             // Object is marked for remove. Used with animation.
            bool connected{false};          // TRUE = there is a connection.
            bool dirty{false};              // TRUE = Object was changed during surface was suspended.
            bool collapsible{false};        // TRUE = Object must be a "window" and it is collapsible.
        }OBJECT_t;

        TObject();
        ~TObject();

        void setParent(MainWindow *mw) { mMainWindow = mw; }
        void clear(bool force=false);
        bool addObject(OBJECT_t& obj);
        OBJECT_t *findObject(ulong handle);
        OBJECT_t *findObject(WId id);
        OBJECT_t *findFirstChild(ulong handle);
        OBJECT_t *findNextChild(ulong handle);
        OBJECT_t *getMarkedRemove();
        OBJECT_t *getNextMarkedRemove(OBJECT_t *obj);
        OBJECT_t *getFirstDirty();
        OBJECT_t *getNextDirty(OBJECT_t *obj);
        OBJECT_t *findFirstWindow();
        OBJECT_t *findNextWindow(OBJECT_t *obj);
        void cleanMarked();
        void removeAllChilds(ulong handle, bool drop=true);
        void removeObject(ulong handle, bool drop=true);
        void invalidateAllObjects();
        void invalidateObject(ulong handle);
        void invalidateAllSubObjects(ulong handle);
        bool enableObject(ulong handle);
        bool enableAllSubObjects(ulong handle);

        static std::string objectToString(OBJECT_TYPE o);
        void dropContent(OBJECT_t *obj, bool lock=true);
        void markDroped(ulong handle);

    private:
        std::mutex mutex_obj;

        std::map<ulong, OBJECT_t> mObjects;
        MainWindow *mMainWindow;
};

#endif
