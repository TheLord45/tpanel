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

#define O_UNDEFINED             -1
#define O_LANDSCAPE             0
#define O_PORTRAIT              1
#define O_REVERSE_LANDSCAPE     8
#define O_REVERSE_PORTRAIT      9
#define O_FACE_UP               15
#define O_FACE_DOWN             16

@interface Rotation : UIDevice <UIApplicationDelegate>

-(UIInterfaceOrientationMask)supportedInterfaceOrientationsForWindow:(UIWindow *)window;

@end

@implementation Rotation

/*
- (id)init
{
    [super init];
    return self;
}
*/
- (UIInterfaceOrientationMask)supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
    if (TIOSRotate::getAllowedOrientation() == true)
        return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
    else
        return UIInterfaceOrientationMaskLandscapeRight | UIInterfaceOrientationMaskLandscapeLeft;
}

@end

// -----------------------------------------------------------------------------
// ---- C++ part starts here
// -----------------------------------------------------------------------------

bool TIOSRotate::mPortrait{false};

TIOSRotate::TIOSRotate()
{
//    [Rotation initialize];
}

void TIOSRotate::rotate(int dir)
{
//    NSNumber *value = nil;
    float value = 0.0;

    switch(dir)
    {
        case O_LANDSCAPE:           value = UIInterfaceOrientationMaskLandscapeRight; break;
        case O_REVERSE_LANDSCAPE:   value = UIInterfaceOrientationMaskLandscapeLeft; break;
        case O_PORTRAIT:            value = UIInterfaceOrientationMaskPortrait; break;
        case O_REVERSE_PORTRAIT:    value = UIInterfaceOrientationMaskPortraitUpsideDown; break;
    }

    NSArray *array = [[[UIApplication sharedApplication] connectedScenes] allObjects];
    UIWindowScene *scene = (UIWindowScene *)array[0];
    UIWindowSceneGeometryPreferencesIOS *geometryPreferences = [[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:value];
    [scene requestGeometryUpdateWithPreferences:geometryPreferences errorHandler:^(NSError * _Nonnull error) { NSLog(@"%@", error); }];

//    UIWindow* window = [[UIApplication sharedApplication] keyWindow];
//    UIViewController *vctrl = [window rootViewController];

}

void TIOSRotate::automaticRotation(bool allow)
{
    if (allow)
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    else
        [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}
