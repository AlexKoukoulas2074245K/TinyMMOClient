///------------------------------------------------------------------------------------------------
///  GameOverResurrectionCheckGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/02/2024
///------------------------------------------------------------------------------------------------

#ifndef GameOverResurrectionCheckGameAction_h
#define GameOverResurrectionCheckGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class GameOverResurrectionCheckGameAction final: public BaseGameAction
{
public:
    static const std::string VICTORIOUS_PLAYER_INDEX_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    enum class AnimationState
    {
        ANIMATING_ARTIFACT,
        FINISHED
    };
     
    AnimationState mAnimationState;
    bool mUsedUpResurrection;
};

///------------------------------------------------------------------------------------------------

#endif /* GameOverResurrectionCheckGameAction_h */
