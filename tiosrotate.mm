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

void TIOSRotate::rotate(int dir)
{
    NSNumber *value = nil;

    switch(dir)
    {
        case O_LANDSCAPE:           value = [NSNumber numberWithInt:UIInterfaceOrientationLandscapeRight]; break;
        case O_REVERSE_LANDSCAPE:   value = [NSNumber numberWithInt:UIInterfaceOrientationLandscapeLeft]; break;
        case O_PORTRAIT:            value = [NSNumber numberWithInt:UIInterfaceOrientationPortrait]; break;
        case O_REVERSE_PORTRAIT:    value = [NSNumber numberWithInt:UIInterfaceOrientationPortraitUpsideDown]; break;
    }

    [[UIDevice currentDevice] setValue:value forKey:@"orientation"];
    [UIViewController attemptRotationToDeviceOrientation];
}
