///------------------------------------------------------------------------------------------------
///  ZeroCostTimeGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 23/02/2024
///------------------------------------------------------------------------------------------------

#ifndef ZeroCostTimeGameAction_h
#define ZeroCostTimeGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class ZeroCostTimeGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* ZeroCostTimeGameAction_h */
