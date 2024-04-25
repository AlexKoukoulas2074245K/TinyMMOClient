///------------------------------------------------------------------------------------------------
///  HoundSummoningGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 20/02/2024
///------------------------------------------------------------------------------------------------

#ifndef HoundSummoningGameAction_h
#define HoundSummoningGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class HoundSummoningGameAction final: public BaseGameAction
{
public:
    static const std::string NUMBER_OF_HOUNDS_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
private:
    bool mFinished;
};

///------------------------------------------------------------------------------------------------

#endif /* HoundSummoningGameAction_h */
