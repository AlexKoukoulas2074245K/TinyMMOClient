///------------------------------------------------------------------------------------------------
///  EndTurnTutorialGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#ifndef EndTurnTutorialGameAction_h
#define EndTurnTutorialGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class EndTurnTutorialGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* EndTurnTutorialGameAction_h */
