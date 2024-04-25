///------------------------------------------------------------------------------------------------
///  DinoDamageReversalGameAction.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 10/03/2024
///------------------------------------------------------------------------------------------------

#ifndef DinoDamageReversalGameAction_h
#define DinoDamageReversalGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class DinoDamageReversalGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    bool mFinished;
    int mLowestDamageHeldCardIndex;
    int mHighestDamageHeldCardIndex;
};

///------------------------------------------------------------------------------------------------

#endif /* DinoDamageReversalGameAction_h */
