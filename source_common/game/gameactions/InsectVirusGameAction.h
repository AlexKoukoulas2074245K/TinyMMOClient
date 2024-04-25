///------------------------------------------------------------------------------------------------
///  InsectVirusGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/02/2024
///------------------------------------------------------------------------------------------------

#ifndef InsectVirusGameAction_h
#define InsectVirusGameAction_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------


class InsectVirusGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* InsectVirusGameAction_h */
