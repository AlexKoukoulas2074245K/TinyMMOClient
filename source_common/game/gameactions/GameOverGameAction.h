///------------------------------------------------------------------------------------------------
///  GameOverGameAction.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameOverGameAction_h
#define GameOverGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class GameOverGameAction final: public BaseGameAction
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
        DEFEAT,
        EXPLOSIONS,
        DISSOLVE,
        FINISHED
    };
     
    AnimationState mAnimationState;
    float mExplosionDelaySecs;
    int mExplosionCounter;
};

///------------------------------------------------------------------------------------------------

#endif /* GameOverGameAction_h */
