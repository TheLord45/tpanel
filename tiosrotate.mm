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

bool TIOSRotate::mPortrait{false};

TIOSRotate::TIOSRotate()
{
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
}

void TIOSRotate::automaticRotation(bool allow)
{
    if (allow)
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    else
        [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
}
