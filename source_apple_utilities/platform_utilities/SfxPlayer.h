///------------------------------------------------------------------------------------------------
///  SfxPlayer.h
///  Predators
///
///  Created by Alex Koukoulas on 17/02/2024
///------------------------------------------------------------------------------------------------

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <OpenAL/al.h>
#import <OpenAL/alc.h>

///------------------------------------------------------------------------------------------------

@interface SfxPlayer : NSObject

@property ALCcontext* context;
@property ALCdevice* device;

@property NSMutableArray* soundSources;
@property NSMutableDictionary* soundLibrary;

@property float targetSfxVolume;
@property bool audioEnabled;

- (id) init;
- (void) pauseSfx;
- (void) resumeSfx;
- (void) setAudioEnabledWith:(BOOL) audioEnabled;
- (void) loadSoundWithName:(NSString*)soundName filePath:(NSString*)filePath;
- (NSUInteger) playSoundWithName:(NSString*)soundName gain:(ALfloat)gain pitch:(ALfloat)pitch shouldLoop:(BOOL)shouldLoop;


@end

///------------------------------------------------------------------------------------------------
