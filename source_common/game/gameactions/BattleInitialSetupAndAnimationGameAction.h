///------------------------------------------------------------------------------------------------
///  BattleInitialSetupAndAnimationGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#ifndef BattleInitialSetupAndAnimationGameAction_h
#define BattleInitialSetupAndAnimationGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class BattleInitialSetupAndAnimationGameAction final: public BaseGameAction
{
public:
    static const std::string CURRENT_BATTLE_SUBSCENE_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;

private:
    int mPendingAnimations;

};

///------------------------------------------------------------------------------------------------

#endif /* BattleInitialSetupAndAnimationGameAction_h */
