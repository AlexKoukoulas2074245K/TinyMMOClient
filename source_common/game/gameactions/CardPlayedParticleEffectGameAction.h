///------------------------------------------------------------------------------------------------
///  CardPlayedParticleEffectGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 23/11/2023
///------------------------------------------------------------------------------------------------

#ifndef CardPlayedParticleEffectGameAction_h
#define CardPlayedParticleEffectGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class CardPlayedParticleEffectGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;

};

///------------------------------------------------------------------------------------------------

#endif /* CardPlayedParticleEffectGameAction_h */
