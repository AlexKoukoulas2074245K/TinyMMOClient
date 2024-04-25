///------------------------------------------------------------------------------------------------
///  DemonPunchGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 21/02/2024
///------------------------------------------------------------------------------------------------

#ifndef DemonPunchGameAction_h
#define DemonPunchGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class DemonPunchGameAction final: public BaseGameAction
{
public:
    static const std::string DEMON_PUNCH_DAMAGE_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
private:
    enum class ActionState
    {
        ANIMATION_GROWING,
        FINISHED
    };
    
    ActionState mAnimationState;
    int mPendingDamage;
    int mAmountOfArmorDamaged;
    int mAmountOfHealthDamaged;
};

///------------------------------------------------------------------------------------------------

#endif /* DemonPunchGameAction_h */
