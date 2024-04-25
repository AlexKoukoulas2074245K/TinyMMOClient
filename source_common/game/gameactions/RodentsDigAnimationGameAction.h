///------------------------------------------------------------------------------------------------
///  RodentsDigAnimationGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/11/2023
///------------------------------------------------------------------------------------------------

#ifndef RodentsDigAnimationGameAction_h
#define RodentsDigAnimationGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class RodentsDigAnimationGameAction final: public BaseGameAction
{
public:
    static const std::string CARD_INDEX_PARAM;
    static const std::string PLAYER_INDEX_PARAM;
    
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;

private:
    void CreateAnimations();
    
private:
    int mStepsFinished = 0;
    float mSecsAccum = 0.0f;
};

///------------------------------------------------------------------------------------------------

#endif /* RodentsDigAnimationGameAction_h */
