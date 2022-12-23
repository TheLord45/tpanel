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

#include "tiosbattery.h"
#include "UIKit/UIKit.h"
#include <Foundation/Foundation.h>
#include "terror.h"


@interface BatteryController : NSObject

    @property(getter=getLeft) int Left;
    @property(getter=getState) int State;

@end

@implementation BatteryController

- (id)init
{
    [self setLeft:0];
    [self setState:0];

    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    // Request to be notified when battery charge or state changes
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:UIDeviceBatteryLevelDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryChanged:) name:UIDeviceBatteryStateDidChangeNotification object:nil];
    return self;
}

- (void)batteryChanged:(NSNotification *)notification
{
    [self UpdateBatteryStatus];
}

- (void)UpdateBatteryStatus
{
    UIDevice *myDevice = [UIDevice currentDevice];
    [myDevice setBatteryMonitoringEnabled:YES];
    float left = [myDevice batteryLevel] * 100;

    if ([self getLeft] < 0.0)
        [self setLeft:(int)([self getLeft] * -1.0)];
    else
        [self setLeft:(int)left];

    [self setState:(int)[myDevice batteryState]];
    TIOSBattery::informStatus([self getLeft], [self getState]);
    MSG_DEBUG("Event battery level: " << [self getLeft] << ", state: " << [self getState]);
}

@end

BatteryController *battery = nil;

// -----------------------------------------------------------------------------
// ---- C++ part starts here
// -----------------------------------------------------------------------------

TIOSBattery::TIOSBattery()
{
    DECL_TRACER("TIOSBattery::TIOSBattery()");

    battery = [[BatteryController alloc] init];
}

TIOSBattery::~TIOSBattery()
{
    DECL_TRACER("TIOSBattery::~TIOSBattery()");

    if (battery)
        [battery release];

    battery = nil;
}

void TIOSBattery::update()
{
    DECL_TRACER("TIOSBattery::update()");

    UIDevice *myDevice = [UIDevice currentDevice];
    [myDevice setBatteryMonitoringEnabled:YES];
    mLeft = [myDevice batteryLevel] * 100;

    if (mLeft < 0)
        mLeft = mLeft * -1;

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
