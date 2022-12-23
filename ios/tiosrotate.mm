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

#include "tiosrotate.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <QGuiApplication>
#include <QScreen>
#include "terror.h"

#define O_UNDEFINED             -1
#define O_LANDSCAPE             0
#define O_PORTRAIT              1
#define O_REVERSE_LANDSCAPE     8
#define O_REVERSE_PORTRAIT      9
#define O_FACE_UP               15
#define O_FACE_DOWN             16

bool TIOSRotate::mPortrait{false};

TIOSRotate::TIOSRotate()
{
    DECL_TRACER("TIOSRotate::TIOSRotate()");
}

TIOSRotate::~TIOSRotate()
{
    DECL_TRACER("TIOSRotate::~TIOSRotate()");
}

void TIOSRotate::rotate(int dir)
{
    DECL_TRACER("TIOSRotate::rotate(int dir)");

    float value = 0.0;

    switch(dir)
    {
        case O_LANDSCAPE:           value = UIInterfaceOrientationMaskLandscapeRight; break;
        case O_REVERSE_LANDSCAPE:   value = UIInterfaceOrientationMaskLandscapeLeft; break;
        case O_PORTRAIT:            value = UIInterfaceOrientationMaskPortrait; break;
        case O_REVERSE_PORTRAIT:    value = UIInterfaceOrientationMaskPortraitUpsideDown; break;
    }

    NSArray *array = [[[UIApplication sharedApplication] connectedScenes] allObjects];

    if (!array)
    {
        MSG_ERROR("Error getting the array of screnes! Will not rotate.");
        return;
    }

    UIWindowScene *scene = (UIWindowScene *)array[0];

    if (!scene)
    {
        MSG_ERROR("Error getting the first scene! Will not roteate.");
        return;
    }

    UIWindowSceneGeometryPreferencesIOS *geometryPreferences = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:value];

    if (!geometryPreferences)
    {
        MSG_ERROR("Error getting the geometry preferences! Changing orientation failed.");
        return;
    }

    [scene requestGeometryUpdateWithPreferences:geometryPreferences errorHandler:^(NSError * _Nonnull error) { NSLog(@"%@", error); }];
}

void TIOSRotate::automaticRotation(bool allow)
{
    DECL_TRACER("TIOSRotate::automaticRotation(bool allow)");

    UIDevice *device = [UIDevice currentDevice];
    BOOL generatesNotes = [device isGeneratingDeviceOrientationNotifications];

    if (allow && !generatesNotes)
    {
        [device beginGeneratingDeviceOrientationNotifications];
    }
    else if (generatesNotes)
    {
        [device endGeneratingDeviceOrientationNotifications];
    }

}

bool TIOSRotate::isAutomaticRotation()
{
    DECL_TRACER("TIOSRotate::isAutomaticRotation()");

    UIDevice *device = [UIDevice currentDevice];
    return [device isGeneratingDeviceOrientationNotifications];
}
