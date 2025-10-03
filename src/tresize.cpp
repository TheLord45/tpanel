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

#include <QDialog>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QLineEdit>
#include <QLine>
#include <QProgressBar>

#include "tresize.h"
#include "terror.h"

double TResize::mScaleFactor{1.0};

using std::vector;

void TResize::dlgResize(QDialog *dlg, vector<ELEMENTS_t> elements, double sf)
{
    DECL_TRACER("TResize::doResize(QDialog *dlg, vector<ELEMENTS_t> elements, double sf)");

    mScaleFactor = sf;
    // The main dialog window
    QSize size = dlg->size();
    QRect rect = dlg->geometry();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    dlg->resize(size);
    dlg->move(scale(rect.left()), scale(rect.top()));
    QWidget *parent = dlg->parentWidget();

    if (parent)
    {
        rect = parent->geometry();
        dlg->move(rect.center() - dlg->rect().center());
    }

    // Layout
    // Iterate through childs and resize them
    QObjectList childs = dlg->children();
    QList<QObject *>::Iterator iter;

    for (iter = childs.begin(); iter != childs.end(); ++iter)
    {
        QObject *obj = *iter;
        QString name = obj->objectName();

        vector<ELEMENTS_t>::iterator elIter;

        for (elIter = elements.begin(); elIter != elements.end(); ++elIter)
        {
            if (name.startsWith(QString::fromStdString(elIter->name)))
            {
                switch (elIter->elType)
                {
                    case QTOOLBUTTON:
                        scaleObject<QToolButton>(dynamic_cast<QToolButton *>(obj));
                    break;

                    case QPUSHBUTTON:
                        scaleObject<QPushButton>(dynamic_cast<QPushButton *>(obj));
                    break;

                    case QLABEL:
                        scaleObject<QLabel>(dynamic_cast<QLabel *>(obj));
                    break;

                    case QFRAME:
                    case QLINE:
                        scaleObject<QFrame>(dynamic_cast<QFrame *>(obj));
                    break;

                    case QLINEEDIT:
                        scaleObject<QLineEdit>(dynamic_cast<QLineEdit *>(obj));
                    break;

                    case QPROGRESSBAR:
                        scaleObject<QProgressBar>(dynamic_cast<QProgressBar *>(obj));
                    break;
                }
            }
        }
    }
}

template<typename T>
void TResize::scaleObject(T *obj)
{
    DECL_TRACER("TResize::scaleObject(T *obj)");

    QSize size = obj->size();
    size.scale(scale(size.width()), scale(size.height()), Qt::KeepAspectRatio);
    obj->resize(size);
    QRect rect = obj->geometry();
    obj->move(scale(rect.left()), scale(rect.top()));
}

int TResize::scale(int value)
{
    if (value <= 0 || mScaleFactor == 1.0)
        return value;

    return (int)((double)value * mScaleFactor);
}

