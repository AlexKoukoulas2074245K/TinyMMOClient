///------------------------------------------------------------------------------------------------
///  InsectDuplicationGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/11/2023
///------------------------------------------------------------------------------------------------

#ifndef InsectDuplicationGameAction_h
#define InsectDuplicationGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class InsectDuplicationGameAction final: public BaseGameAction
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

#endif /* InsectDuplicationGameAction_h */
