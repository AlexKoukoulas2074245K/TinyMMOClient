///------------------------------------------------------------------------------------------------
///  HowToPlayACardTutorialGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#ifndef HowToPlayACardTutorialGameAction_h
#define HowToPlayACardTutorialGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class HowToPlayACardTutorialGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* HowToPlayACardTutorialGameAction_h */
