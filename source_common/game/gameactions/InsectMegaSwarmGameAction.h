///------------------------------------------------------------------------------------------------
///  InsectMegaSwarmGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/02/2024
///------------------------------------------------------------------------------------------------

#ifndef InsectMegaSwarmGameAction_h
#define InsectMegaSwarmGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class InsectMegaSwarmGameAction final: public BaseGameAction
{
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

#endif /* InsectMegaSwarmGameAction_h */
