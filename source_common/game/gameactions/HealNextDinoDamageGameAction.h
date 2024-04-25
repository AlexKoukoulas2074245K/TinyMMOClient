///------------------------------------------------------------------------------------------------
///  HealNextDinoDamageGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 15/02/2024
///------------------------------------------------------------------------------------------------

#ifndef HealNextDinoDamageGameAction_h
#define HealNextDinoDamageGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class HealNextDinoDamageGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* HealNextDinoDamageGameAction_h */
