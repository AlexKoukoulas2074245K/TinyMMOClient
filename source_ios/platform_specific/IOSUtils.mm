///------------------------------------------------------------------------------------------------
///  IOSUtils.mm
///  Predators
///
///  Created by Alex Koukoulas on 16/11/2023.
///-----------------------------------------------------------------------------------------------

#include <platform_specific/IOSUtils.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIDevice.h>

///-----------------------------------------------------------------------------------------------

namespace ios_utils
{

///-----------------------------------------------------------------------------------------------

bool IsIPad()
{
    NSString *deviceType = [UIDevice currentDevice].model;
    if([deviceType isEqualToString:@"iPhone"]) {
        return false;
    }
    else if([deviceType isEqualToString:@"iPod touch"]) {
        return false;
    }
    
    return true;
}

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------
