///------------------------------------------------------------------------------------------------
///  TrapTriggeredAnimationGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 17/11/2023
///------------------------------------------------------------------------------------------------

#ifndef TrapTriggeredAnimationGameAction_h
#define TrapTriggeredAnimationGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class TrapTriggeredAnimationGameAction final: public BaseGameAction
{
public:
    static const std::string TRAP_TRIGGER_TYPE_PARAM;
    static const std::string TRAP_TRIGGER_TYPE_KILL;
    static const std::string TRAP_TRIGGER_TYPE_DEBUFF;
    static const std::string KILL_TRAP_TYPE_PARAM;
    static const std::string KILL_TRAP_TYPE_BEAR_TRAP;
    static const std::string KILL_TRAP_TYPE_DEMON_TRAP;
    
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
        ANIMATION_STEP_2,    // Step for introducing new animations
        FINISHED             // Cleanup and finishing off behavior
    };
    
    ActionState mAnimationState;
};

///------------------------------------------------------------------------------------------------

#endif /* TrapTriggeredAnimationGameAction_h */
