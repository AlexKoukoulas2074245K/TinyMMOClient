///------------------------------------------------------------------------------------------------
///  SfxPlayer.mm
///  Predators
///
///  Created by Alex Koukoulas on 14/02/2024.
///-----------------------------------------------------------------------------------------------

#import <platform_utilities/SfxPlayer.h>

///------------------------------------------------------------------------------------------------
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define kMaxSources 31
#define MAX_SFX_VOLUME 0.5f
#define ALC_CALL(func) { func; [self logOpenALErrorWith:alcGetError(_device)]; }
#define AL_CALL(func) { func; [self logOpenALErrorWith:alGetError()]; }

@interface SfxPlayer (Private)
- (BOOL) initOpenAL;
- (BOOL) deinitOpenAL;
- (NSUInteger) nextAvailableSource;
- (AudioFileID) openAudioFile:(NSString*)fileName;
- (UInt32) audioFileSize:(AudioFileID)fileDescriptor;
- (void) logOpenALErrorWith:(ALenum)openALError;
@end

@implementation SfxPlayer

- (id) init
{
    _soundSources = [[NSMutableArray alloc] init];
    _soundLibrary = [[NSMutableDictionary alloc] init];
    _targetSfxVolume = MAX_SFX_VOLUME;
    _audioEnabled = YES;
    _device = nil;
    _context = nil;
    
    BOOL result = [self initOpenAL];
    if (!result) return nil;
    return self;
}

- (void) pauseSfx
{
    ALC_CALL(alcMakeContextCurrent(NULL));
    _targetSfxVolume = 0.0f;
}

- (void) resumeSfx
{
    if (_audioEnabled)
    {
        ALC_CALL(alcMakeContextCurrent(_context));
    }
    _targetSfxVolume = MAX_SFX_VOLUME;
}

- (void) setAudioEnabledWith:(BOOL)audioEnabled
{
    _audioEnabled = audioEnabled;
    
    if (_audioEnabled)
    {
        ALC_CALL(alcMakeContextCurrent(_context));
    }
    else
    {
        ALC_CALL(alcMakeContextCurrent(NULL));
    }
}

- (BOOL) initOpenAL
{
    if (_context && _device)
    {
        return YES;
    }
    
    _device = alcOpenDevice(NULL);
     
    if (_device)
    {
        ALC_CALL(_context = alcCreateContext(_device, NULL));
        ALC_CALL(alcMakeContextCurrent(_context));
        
        ALuint sourceID;
        for (int i = 0; i < kMaxSources; ++i)
        {
            AL_CALL(alGenSources(1, &sourceID));
            [_soundSources addObject:[NSNumber numberWithUnsignedInt:sourceID]];
        }
        
        return YES;
    }
    
    NSLog(@"SfxPlayer: Can't open device!");
    return NO;
}

- (void) loadSoundWithName:(NSString *)soundName filePath:(NSString *)filePath
{
    if ([_soundLibrary objectForKey:filePath] != nil)
    {
        return;
    }
    
    if (!_audioEnabled)
    {
        return;
    }
    
    NSString* sandboxFilePath = [NSBundle.mainBundle pathForResource:filePath ofType:@"wav"];
    
    AudioFileID fileID = [self openAudioFile:sandboxFilePath];
    
    UInt32 fileSize = [self audioFileSize:fileID];
    
    void* outData = malloc(fileSize);
    
    OSStatus result = noErr;
    result = AudioFileReadBytes(fileID, FALSE, 0, &fileSize, outData);
    AudioFileClose(fileID);
    
    if (result != 0)
    {
        NSLog(@"ERROR SoundEngine: Cannot load sound: %@", filePath);
    }
    
    ALuint bufferID;
    
    AL_CALL(alGenBuffers(1, &bufferID));
    AL_CALL(alBufferData(bufferID, AL_FORMAT_STEREO16, outData, fileSize, 48000));
    
    [_soundLibrary setObject:[NSNumber numberWithUnsignedInt:bufferID] forKey:filePath];
    
    if (outData)
    {
        free(outData);
        outData = NULL;
    }
}

- (AudioFileID) openAudioFile:(NSString *)filePath
{
    AudioFileID outAFID;
    
    NSURL* afUrl = [NSURL fileURLWithPath:filePath];
    
    OSStatus result = AudioFileOpenURL((__bridge CFURLRef)afUrl, kAudioFileReadPermission, 0, &outAFID);
    
    if (result != 0)
    {
        NSLog(@"ERROR SoundEngine: Cannot open file: %@", filePath);
        return nil;
    }
    
    return outAFID;
}

- (UInt32) audioFileSize:(AudioFileID)fileDescriptor
{
    UInt64 outDataSize = 0;
    UInt32 thePropSize = sizeof(UInt64);
    OSStatus result = AudioFileGetProperty(fileDescriptor, kAudioFilePropertyAudioDataByteCount, &thePropSize, &outDataSize);
    if (result != 0)
    {
        NSLog(@"ERROR: cannot find file size");
    }
    
    return (UInt32)outDataSize;
}

- (NSUInteger) playSoundWithName:(NSString *)soundName gain:(ALfloat)gain pitch:(ALfloat)pitch shouldLoop:(BOOL)shouldLoop
{
    if (!_audioEnabled)
    {
        return 0;
    }
    
    NSNumber* numVal = [_soundLibrary objectForKey:soundName];
    if (numVal == nil)
    {
        return 0;
    }
    
    ALuint bufferID = [numVal unsignedIntValue];
    ALuint sourceID = static_cast<ALuint>([self nextAvailableSource]);
    
    AL_CALL(alSourcei(sourceID, AL_BUFFER, 0));
    AL_CALL(alSourcei(sourceID, AL_BUFFER, bufferID));
    
    AL_CALL(alSourcef(sourceID, AL_PITCH, pitch));
    AL_CALL(alSourcef(sourceID, AL_GAIN, _targetSfxVolume * gain));
    
    AL_CALL(alSourcei(sourceID, AL_LOOPING, shouldLoop ? AL_TRUE : AL_FALSE));
    
    AL_CALL(alSourcePlay(sourceID));
    
    return sourceID;
}

- (NSUInteger) nextAvailableSource
{
    ALint sourceState;
    for (NSNumber* sourceNumber in _soundSources)
    {
        AL_CALL(alGetSourcei([sourceNumber unsignedIntValue], AL_SOURCE_STATE, &sourceState));
        if (sourceState != AL_PLAYING)
        {
            return [sourceNumber unsignedIntValue];
        }
    }
    
    ALint looping;
    for (NSNumber* sourceNumber in _soundSources)
    {
        AL_CALL(alGetSourcei(static_cast<ALuint>([sourceNumber unsignedIntValue]), AL_LOOPING, &looping));
        if (!looping)
        {
            NSUInteger sourceID = [sourceNumber unsignedIntValue];
            AL_CALL(alSourceStop(static_cast<ALuint>(sourceID)));
            return sourceID;
        }
    }
    
    NSUInteger sourceID = [[_soundSources objectAtIndex:0] unsignedIntegerValue];
    AL_CALL(alSourceStop(static_cast<ALuint>(sourceID)));
    return sourceID;
}

- (void) logOpenALErrorWith:(ALenum)openALError
{
    if (openALError != AL_NO_ERROR)
    {
        switch (openALError)
        {
            case AL_INVALID_NAME: NSLog(@"AL_INVALID_NAME Error"); break;
            case AL_INVALID_ENUM: NSLog(@"AL_INVALID_ENUM Error"); break;
            case AL_INVALID_VALUE: NSLog(@"AL_INVALID_VALUE Error"); break;
            case AL_INVALID_OPERATION: NSLog(@"AL_INVALID_OPERATION Error"); break;
            case AL_OUT_OF_MEMORY: NSLog(@"AL_OUT_OF_MEMORY Error"); break;
            default: NSLog(@"OpenAL Unknown Error"); break;
        }
    }
}

@end

#pragma clang diagnostic pop
