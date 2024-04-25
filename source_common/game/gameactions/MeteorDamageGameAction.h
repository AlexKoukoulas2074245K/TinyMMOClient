///------------------------------------------------------------------------------------------------
///  MeteorDamageGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 22/02/2024
///------------------------------------------------------------------------------------------------

#ifndef MeteorDamageGameAction_h
#define MeteorDamageGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class MeteorDamageGameAction final: public BaseGameAction
{
public:
    static const std::string METEOR_DAMAGE_PARAM;
    
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    bool mFinished;
    int mPendingDamage;
    int mAmountOfArmorDamaged;
    int mAmountOfHealthDamaged;
};

///------------------------------------------------------------------------------------------------

#endif /* MeteorDamageGameAction_h */
