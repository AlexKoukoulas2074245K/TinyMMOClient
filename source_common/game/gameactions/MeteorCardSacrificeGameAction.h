///------------------------------------------------------------------------------------------------
///  MeteorCardSacrificeGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 22/02/2024
///------------------------------------------------------------------------------------------------

#ifndef MeteorCardSacrificeGameAction_h
#define MeteorCardSacrificeGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class MeteorCardSacrificeGameAction final: public BaseGameAction
{
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* MeteorCardSacrificeGameAction_h */
