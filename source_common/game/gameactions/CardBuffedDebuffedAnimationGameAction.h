///------------------------------------------------------------------------------------------------
///  CardBuffedDebuffedAnimationGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/11/2023
///------------------------------------------------------------------------------------------------

#ifndef CardBuffedDebuffedAnimationGameAction_h
#define CardBuffedDebuffedAnimationGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class CardBuffedDebuffedAnimationGameAction final: public BaseGameAction
{
public:
    static const std::string CARD_INDEX_PARAM;
    static const std::string PLAYER_INDEX_PARAM;
    static const std::string IS_BOARD_CARD_PARAM;
    static const std::string SCALE_FACTOR_PARAM;
    static const std::string PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM;
    static const std::string CARD_BUFFED_REPEAT_INDEX;
    
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
    
private:
    bool mFinished;
};

///------------------------------------------------------------------------------------------------

#endif /* CardBuffedDebuffedAnimationGameAction_h */
