///------------------------------------------------------------------------------------------------
///  SpellKillGameAction.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 11/03/2024
///------------------------------------------------------------------------------------------------

#ifndef SpellKillGameAction_h
#define SpellKillGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class SpellKillGameAction final: public BaseGameAction
{    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
private:
    enum class ActionState
    {
        ANIMATION_STEP_WAIT, // Reusable step for different actions waiting on an animation
        FINISHED             // Cleanup and finishing off behavior
    };
    
    ActionState mAnimationState;
};

///------------------------------------------------------------------------------------------------

#endif /* SpellKillGameAction_h */
