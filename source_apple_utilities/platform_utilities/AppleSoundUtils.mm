///------------------------------------------------------------------------------------------------
///  AppleSoundUtils.mm
///  Predators
///
///  Created by Alex Koukoulas on 14/02/2024.
///-----------------------------------------------------------------------------------------------

#include <platform_utilities/AppleSoundUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/StringUtils.h>

#import <platform_utilities/BackgroundMusicPlayer.h>
#import <platform_utilities/SfxPlayer.h>
#import <AudioToolbox/AudioServices.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>


///-----------------------------------------------------------------------------------------------

namespace sound_utils
{

///-----------------------------------------------------------------------------------------------

static BackgroundMusicPlayer* musicPlayer;
static SfxPlayer* sfxPlayer;
static std::string ROOT_SOUND_RES_PATH;

///-----------------------------------------------------------------------------------------------

void Vibrate()
{
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
}

///-----------------------------------------------------------------------------------------------

void PreloadSfx(const std::string& sfxResPath)
{
    NSString* objectiveCresPath = [NSString stringWithCString:(ROOT_SOUND_RES_PATH + sfxResPath).data() encoding:[NSString defaultCStringEncoding]];
    
    if (sfxPlayer != nil)
    {
        [sfxPlayer loadSoundWithName:[NSString stringWithCString:sfxResPath.data() encoding:[NSString defaultCStringEncoding]] filePath:objectiveCresPath];
    }
}

///------------------------------------------------------------------------------------------------

void PlaySound(const std::string& soundResPath, const bool loopedSfxOrUnloopedMusic /* = false */, const float gain /* = 1.0f */, const float pitch /* = 1.0f */)
{
    NSString* objectiveCresPath = [NSString stringWithCString:(ROOT_SOUND_RES_PATH + soundResPath).data() encoding:[NSString defaultCStringEncoding]];
    
    if (strutils::StringStartsWith(soundResPath, "sfx_"))
    {
        if (sfxPlayer != nil)
        {
            [sfxPlayer playSoundWithName:objectiveCresPath gain:gain pitch:pitch shouldLoop:loopedSfxOrUnloopedMusic];
        }
    }
    else
    {
        if (musicPlayer != nil)
        {
            [musicPlayer playMusicWith:objectiveCresPath unloopedMusic:loopedSfxOrUnloopedMusic];
        }
    }
}

///------------------------------------------------------------------------------------------------

void InitAudio()
{
    ROOT_SOUND_RES_PATH = "assets/music/";
    musicPlayer = [[BackgroundMusicPlayer alloc] init];
    sfxPlayer = [[SfxPlayer alloc] init];
}

///------------------------------------------------------------------------------------------------

void ResumeAudio()
{
    if (musicPlayer != nil)
    {
        [musicPlayer resumeAudio];
    }
    
    if (sfxPlayer != nil)
    {
        [sfxPlayer resumeSfx];
    }
}

///------------------------------------------------------------------------------------------------

void PauseMusicOnly()
{
    if (musicPlayer != nil)
    {
        [musicPlayer pauseMusic];
    }
}

///------------------------------------------------------------------------------------------------

void PauseSfxOnly()
{
    if (sfxPlayer != nil)
    {
        [sfxPlayer pauseSfx];
    }
}

///------------------------------------------------------------------------------------------------

void PauseAudio()
{
    PauseMusicOnly();
    PauseSfxOnly();
}

///------------------------------------------------------------------------------------------------

void UpdateAudio(const float dtMillis)
{
    if (musicPlayer != nil)
    {
        [musicPlayer updateAudioWith:dtMillis];
    }
}

///------------------------------------------------------------------------------------------------

void SetAudioEnabled(const bool audioEnabled)
{
    if (musicPlayer != nil)
    {
        [musicPlayer setAudioEnabledWith:audioEnabled];
        [sfxPlayer setAudioEnabledWith:audioEnabled];
    }
}

///------------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------
