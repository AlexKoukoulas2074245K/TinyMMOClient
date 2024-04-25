///------------------------------------------------------------------------------------------------
///  DrawCardGameAction.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef DrawCardGameAction_h
#define DrawCardGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class DrawCardGameAction final: public BaseGameAction
{
public:
    static const std::string DRAW_SPELL_ONLY_PARAM;
    
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
private:
    int mPendingAnimations;
};

///------------------------------------------------------------------------------------------------

#endif /* DrawCardGameAction_h */
