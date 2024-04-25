///------------------------------------------------------------------------------------------------
///  BackgroundMusicPlayer.mm
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 14/02/2024.
///-----------------------------------------------------------------------------------------------

#import <platform_utilities/BackgroundMusicPlayer.h>

///------------------------------------------------------------------------------------------------

@implementation BackgroundMusicPlayer

///------------------------------------------------------------------------------------------------

static const float ENABLED_AUDIO_MUSIC_VOLUME = 1.0f;
static const float DISABLED_AUDIO_MUSIC_VOLUME = 0.0f;

///------------------------------------------------------------------------------------------------

- (id) init
{
    self = [super init];
    _currentPlayingMusicPath = nil;
    _nextQueuedMusicPath = nil;
    _musicPlayer = nil;
    _firstAppStateCall = YES;
    _targetMusicVolume = ENABLED_AUDIO_MUSIC_VOLUME;
    return self;
}

///------------------------------------------------------------------------------------------------

- (void) playMusicWith:(NSString*) soundResPath unloopedMusic:(BOOL) unloopedMusic
{
    NSString* sandboxFilePath = [NSBundle.mainBundle pathForResource:soundResPath ofType:@"flac"];
    
    if (!_audioEnabled)
    {
        _nextMusicUnlooped = unloopedMusic;
        _nextQueuedMusicPath = soundResPath;
        return;
    }
    
    if (sandboxFilePath != nil)
    {
        if (_musicPlayer == nil)
        {
            dispatch_queue_t backgroundQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0);
            dispatch_async(backgroundQueue, ^{
                self.musicPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:sandboxFilePath] error:nil];
                self.musicPlayer.numberOfLoops = self.nextMusicUnlooped ? 0 : -1;
                self.musicPlayer.volume = 0.0f;
                self.currentPlayingMusicPath = sandboxFilePath;
                self.nextQueuedMusicPath = sandboxFilePath;
                
                [self.musicPlayer prepareToPlay];
                [self.musicPlayer play];
            });
        }
        else if (![_currentPlayingMusicPath isEqualToString:sandboxFilePath])
        {
            _nextQueuedMusicPath = sandboxFilePath;
        }
    }
    else
    {
        NSLog(@"Can't open sound file %@", soundResPath);
    }
}

///------------------------------------------------------------------------------------------------

- (void) pauseMusic
{
    if (_musicPlayer != nil)
    {
        [_musicPlayer pause];
    }
}

///------------------------------------------------------------------------------------------------

- (void) pauseAudio
{
    [self pauseMusic];
}

///------------------------------------------------------------------------------------------------

- (void) resumeAudio
{
    if (_firstAppStateCall)
    {
        _firstAppStateCall = NO;
    }
    else
    {
        if (_musicPlayer != nil && _audioEnabled)
        {
            [_musicPlayer play];
        }
    }
}

///------------------------------------------------------------------------------------------------

- (void) updateAudioWith:(float) dtMillis
{
    float FADE_SPEED = 0.00125f;
    float TARGET_MILLIS = 16.66666f;
    
    if (_musicPlayer != nil)
    {
        if (![_nextQueuedMusicPath isEqualToString:_currentPlayingMusicPath])
        {
            if (_musicPlayer.volume > 0.0f)
            {
                _musicPlayer.volume -= TARGET_MILLIS * FADE_SPEED;
            }
            else
            {
                _musicPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:_nextQueuedMusicPath] error:nil];
                _musicPlayer.numberOfLoops = _nextMusicUnlooped ? 0 : -1;
                _musicPlayer.volume = 0.0f;
                [_musicPlayer prepareToPlay];
                [_musicPlayer play];
                _currentPlayingMusicPath = _nextQueuedMusicPath;
            }
        }
        else
        {
            if (_musicPlayer.volume < _targetMusicVolume)
            {
                _musicPlayer.volume += TARGET_MILLIS * FADE_SPEED;
            }
        }
    }
    
}

///------------------------------------------------------------------------------------------------

- (void) setAudioEnabledWith:(BOOL) audioEnabled
{
    _audioEnabled = audioEnabled;
    _targetMusicVolume = audioEnabled ? ENABLED_AUDIO_MUSIC_VOLUME : DISABLED_AUDIO_MUSIC_VOLUME;
    
    if (_musicPlayer != nil)
    {
        _musicPlayer.volume = _targetMusicVolume;
        
        if (audioEnabled)
        {
            [_musicPlayer play];
        }
        else
        {
            [_musicPlayer pause];
        }
    }
    else if (audioEnabled)
    {
        if ([_nextQueuedMusicPath length] > 0)
        {
            [self playMusicWith:_nextQueuedMusicPath unloopedMusic:_nextMusicUnlooped];
        }
    }
}

///------------------------------------------------------------------------------------------------

@end
