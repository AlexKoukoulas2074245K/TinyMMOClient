///------------------------------------------------------------------------------------------------
///  GuiObjectManager.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GuiObjectManager_h
#define GuiObjectManager_h

///------------------------------------------------------------------------------------------------

#include <game/events/EventSystem.h>
#include <engine/utils/MathUtils.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

enum class GuiUpdateInteractionResult
{
    CLICKED_GUI_BUTTONS,
    DID_NOT_CLICK_GUI_BUTTONS
};

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }

class AnimatedStatContainer;
class AnimatedButton;
class GuiObjectManager final: public events::IListener
{
public:
    GuiObjectManager(std::shared_ptr<scene::Scene> scene);
    ~GuiObjectManager();
    
    GuiUpdateInteractionResult Update(const float dtMillis, const bool allowButtonInput = true);
    void OnWindowResize();
    void ResetDisplayedCurrencyCoins();
    void ForceSetStoryHealthValue(const int storyHealthValue);
    void StopRewardAnimation();
    int GetStoryHealthContainerCurrentValue() const;
    
private:
    enum class StatParticleType
    {
        COINS,
        HEALTH
    };
    
    enum class StatGainParticleType
    {
        MAX_HEALTH,
        DAMAGE,
        WEIGHT
    };
    
    void AnimateStatParticlesFlyingToGui(const glm::vec3& originPosition, const StatParticleType statParticleType, const long long coinAmount);
    void AnimateStatGainParticles(const glm::vec3& originPosition, const StatGainParticleType statGainParticleType);
    void SetCoinValueText();
    void OnSettingsButtonPressed();
    void OnStoryCardsButtonPressed();
    void OnInventoryButtonPressed();
    void OnCoinReward(const events::CoinRewardEvent&);
    void OnHealthRefillReward(const events::HealthRefillRewardEvent&);
    void OnMaxHealthGainReward(const events::MaxHealthGainRewardEvent&);
    void OnRareItemCollected(const events::RareItemCollectedEvent&);
    
private:
    std::unordered_map<strutils::StringId, float, strutils::StringIdHasher> mParticleEmitterTimeAccums;
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::unique_ptr<AnimatedStatContainer> mHealthStatContainer;
    std::shared_ptr<scene::Scene> mScene;
    float mRewardAnimationSecsLeft;
    bool mBattleLootHealthRefillCase;
};

///------------------------------------------------------------------------------------------------

#endif /* GuiObjectManager_h */
