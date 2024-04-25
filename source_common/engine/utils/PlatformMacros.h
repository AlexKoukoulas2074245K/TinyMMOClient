///------------------------------------------------------------------------------------------------
///  PlatformMacros.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/11/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef PlatformMacros_h
#define PlatformMacros_h

///------------------------------------------------------------------------------------------------

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define DESKTOP_FLOW
    #define WINDOWS
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #define MOBILE_FLOW
    #else
        #define DESKTOP_FLOW
        #define MACOS
    #endif
#endif

///------------------------------------------------------------------------------------------------

#endif /* PlatformMacros_h */
