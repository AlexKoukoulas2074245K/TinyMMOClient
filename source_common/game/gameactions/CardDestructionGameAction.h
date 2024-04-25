///------------------------------------------------------------------------------------------------
///  CardDestructionGameAction.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef CardDestructionGameAction_h
#define CardDestructionGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------


class CardDestructionGameAction final: public BaseGameAction
{
public:
    static const std::string CARD_INDICES_PARAM;
    static const std::string PLAYER_INDEX_PARAM;
    static const std::string IS_BOARD_CARD_PARAM;
    static const std::string IS_TRAP_TRIGGER_PARAM;
    static const std::string IS_SINGLE_CARD_USED_COPY_PARAM;
    
public:
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* CardDestructionGameAction_h */
