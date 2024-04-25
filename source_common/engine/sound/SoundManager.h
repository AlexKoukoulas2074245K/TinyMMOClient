///------------------------------------------------------------------------------------------------
///  SoundManager.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 15/02/2024.
///------------------------------------------------------------------------------------------------

#ifndef SoundManager_h
#define SoundManager_h

///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/StringUtils.h>

///------------------------------------------------------------------------------------------------

namespace sound
{

class SoundManager final
{
    friend struct CoreSystemsEngine::SystemsImpl;
public:
    
    ~SoundManager();
    SoundManager(const SoundManager&) = delete;
    SoundManager(SoundManager&&) = delete;
    const SoundManager& operator = (const SoundManager&) = delete;
    SoundManager& operator = (SoundManager&&) = delete;
    
    void Initialize();
    void Update(const float dtMillis);
    
    void Vibrate();
    void PreloadSfx(const std::string& sfxResPath);
    void PlaySound(const std::string& soundResPath, const bool loopedSfxOrUnloopedMusic = false, const float gain = 1.0f, const float pitch = 1.0f);
    void ResumeAudio();
    void PauseMusicOnly();
    void PauseSfxOnly();
    void PauseAudio();
    void SetAudioEnabled(const bool enabled);
    
private:
    SoundManager();
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* ResourceLoadingService_h */
