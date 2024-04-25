///------------------------------------------------------------------------------------------------
///  CardHistoryEntryAdditionGameAction.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/12/2023
///------------------------------------------------------------------------------------------------

#ifndef CardHistoryEntryAdditionGameAction_h
#define CardHistoryEntryAdditionGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/BaseGameAction.h>

///------------------------------------------------------------------------------------------------

class CardHistoryEntryAdditionGameAction final: public BaseGameAction
{
public:
    static const std::string PLAYER_INDEX_PARAM;
    static const std::string CARD_INDEX_PARAM;
    static const std::string IS_TURN_COUNTER_PARAM;
    static const std::string ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM;
    static const std::string ENTRY_TYPE_TEXTURE_FILE_NAME_BATTLE;
    static const std::string ENTRY_TYPE_TEXTURE_FILE_NAME_EFFECT;
    static const std::string ENTRY_TYPE_TEXTURE_FILE_NAME_DEATH;
    
    void VSetNewGameState() override;
    
    void VInitAnimation() override;
    
    ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) override;
    
    bool VShouldBeSerialized() const override;
    
    const std::vector<std::string>& VGetRequiredExtraParamNames() const override;
};

///------------------------------------------------------------------------------------------------

#endif /* CardHistoryEntryAdditionGameAction_h */
