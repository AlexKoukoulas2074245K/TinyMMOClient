///------------------------------------------------------------------------------------------------
///  GoldenCardPlayedEffectGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 20/11/2023
///------------------------------------------------------------------------------------------------

#ifndef GoldenCardPlayedEffectGameAction_h
#define GoldenCardPlayedEffectGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class GoldenCardPlayedEffectGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;

private:
    bool mFinished;
    float mGoldenCardPlayedLightEffectX;
};

///------------------------------------------------------------------------------------------------

#endif /* GoldenCardPlayedEffectGameAction_h */
