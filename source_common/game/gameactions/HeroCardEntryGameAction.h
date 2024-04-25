///------------------------------------------------------------------------------------------------
///  HeroCardEntryGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/01/2024
///------------------------------------------------------------------------------------------------

#ifndef HeroCardEntryGameAction_h
#define HeroCardEntryGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class HeroCardEntryGameAction final: public BaseGameAction
{
public:
    static const std::string LAST_PLAYED_CARD_INDEX_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    enum class AnimationState
    {
        ANIMATING_HERO_CARD,
        INITIALIZE_HEALTH_CRYSTAL_ANIMATION,
        ANIMATING_HEALTH_CRYSTAL,
        COMPLETE
    };
    
    AnimationState mAnimationState;
    int mHeroCardId;
    glm::vec3 mTargetHealthCrystalBasePosition;
    glm::vec3 mTargetHealthCrystalValuePosition;
    glm::vec3 mTargetHealthCrystalBaseScale;
    glm::vec3 mTargetHealthCrystalValueScale;
};

///------------------------------------------------------------------------------------------------

#endif /* HeroCardEntryGameAction_h */
