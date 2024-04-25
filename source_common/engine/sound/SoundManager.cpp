///------------------------------------------------------------------------------------------------
///  SoundManager.cpp
///  Predators
///
///  Created by Alex Koukoulas on 15/02/2024.
///------------------------------------------------------------------------------------------------

#include <engine/sound/SoundManager.h>
#include <engine/utils/PlatformMacros.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleSoundUtils.h>
#define PLATFORM_CALL(func) (sound_utils::func)
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

namespace sound
{

///------------------------------------------------------------------------------------------------

SoundManager::SoundManager()
{
    
}

///------------------------------------------------------------------------------------------------

SoundManager::~SoundManager()
{
}

///------------------------------------------------------------------------------------------------

void SoundManager::Initialize()
{
    PLATFORM_CALL(InitAudio());
}

///------------------------------------------------------------------------------------------------

void SoundManager::Update(const float dtMillis)
{
    PLATFORM_CALL(UpdateAudio(dtMillis));
}

///------------------------------------------------------------------------------------------------

void SoundManager::Vibrate()
{
    PLATFORM_CALL(Vibrate());
}

///------------------------------------------------------------------------------------------------

void SoundManager::PreloadSfx(const std::string& sfxResPath)
{
    PLATFORM_CALL(PreloadSfx(sfxResPath));
}

///------------------------------------------------------------------------------------------------

void SoundManager::PlaySound(const std::string& soundResPath, const bool loopedSfxOrUnloopedMusic /* = false */, const float gain /* = 1.0f */, const float pitch /* = 1.0f */)
{
    PLATFORM_CALL(PlaySound(soundResPath, loopedSfxOrUnloopedMusic, gain, pitch));
}

///------------------------------------------------------------------------------------------------

void SoundManager::ResumeAudio()
{
    PLATFORM_CALL(ResumeAudio());
}

///------------------------------------------------------------------------------------------------

void SoundManager::PauseMusicOnly()
{
    PLATFORM_CALL(PauseMusicOnly());
}

///------------------------------------------------------------------------------------------------

void SoundManager::PauseSfxOnly()
{
    PLATFORM_CALL(PauseSfxOnly());
}

///------------------------------------------------------------------------------------------------

void SoundManager::PauseAudio()
{
    PLATFORM_CALL(PauseAudio());
}

///------------------------------------------------------------------------------------------------

void SoundManager::SetAudioEnabled(const bool enabled)
{
    PLATFORM_CALL(SetAudioEnabled(enabled));
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
