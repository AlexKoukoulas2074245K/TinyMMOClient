///------------------------------------------------------------------------------------------------
///  PlayCardGameAction.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef PlayCardGameAction_h
#define PlayCardGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class PlayCardGameAction final: public BaseGameAction
{
public:
    static const std::string LAST_PLAYED_CARD_INDEX_PARAM;

public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    void AnimatedCardToBoard(std::shared_ptr<CardSoWrapper> lastPlayedCardSoWrapper);
    
private:
    int mPendingAnimations;
    bool mAborted;
    bool mHasFinalizedCardPlay;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayCardGameAction_h */
