///------------------------------------------------------------------------------------------------
///  PoisonStackApplicationGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 24/11/2023
///------------------------------------------------------------------------------------------------

#ifndef PoisonStackApplicationGameAction_h
#define PoisonStackApplicationGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class PoisonStackApplicationGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;

private:
    float mPendingDurationSecs;
    int mAmountOfArmorDamaged;
    int mAmountOfHealthDamaged;
    bool mWaitingForArmorAndHealthReductionTriggers;
};

///------------------------------------------------------------------------------------------------

#endif /* PoisonStackApplicationGameAction_h */
