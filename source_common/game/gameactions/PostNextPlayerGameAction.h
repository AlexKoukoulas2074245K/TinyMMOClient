///------------------------------------------------------------------------------------------------
///  PostNextPlayerGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/11/2023
///------------------------------------------------------------------------------------------------

#ifndef PostNextPlayerGameAction_h
#define PostNextPlayerGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class PostNextPlayerGameAction final: public BaseGameAction
{
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

#endif /* PostNextPlayerGameAction_h */
