///------------------------------------------------------------------------------------------------
///  BackgroundMusicPlayer.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 14/02/2024
///------------------------------------------------------------------------------------------------

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

///------------------------------------------------------------------------------------------------

@interface BackgroundMusicPlayer : NSObject

@property AVAudioPlayer* musicPlayer;
@property BOOL firstAppStateCall;
@property BOOL nextMusicUnlooped;
@property BOOL audioEnabled;
@property NSString* currentPlayingMusicPath;
@property NSString* nextQueuedMusicPath;
@property float targetMusicVolume;

- (id) init;
- (void) playMusicWith:(NSString*) soundResPath unloopedMusic:(BOOL) unloopedMusic;
- (void) pauseMusic;
- (void) pauseAudio;
- (void) resumeAudio;
- (void) updateAudioWith:(float) dtMillis;
- (void) setAudioEnabledWith:(BOOL) audioEnabled;

@end

///------------------------------------------------------------------------------------------------
