#include "tiosbattery.h"
#include "UIKit/UIKit.h"
#include <Foundation/Foundation.h>

TIOSBattery::TIOSBattery()
{
}

void TIOSBattery::update()
{
    UIDevice *myDevice = [UIDevice currentDevice];
    [myDevice setBatteryMonitoringEnabled:YES];
    mLeft = [myDevice batteryLevel] * 100;
    int status = [myDevice batteryState];

    switch (status)
    {
        case UIDeviceBatteryStateUnplugged: mState = BS_UNPLUGGED; break;
        case UIDeviceBatteryStateCharging:  mState = BS_CHARGING; break;
        case UIDeviceBatteryStateFull:      mState = BS_FULL; break;

        default:
            mState = BS_UNKNOWN;
    }
}
