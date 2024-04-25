///------------------------------------------------------------------------------------------------
///  PlayerActionGenerationEngine.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef PlayerActionGenerationEngine_h
#define PlayerActionGenerationEngine_h

///------------------------------------------------------------------------------------------------

class GameRuleEngine;
class GameActionEngine;
class BoardState;
struct CardData;
class PlayerActionGenerationEngine final
{
public:
    enum class ActionGenerationType
    {
        FULLY_DETERMINISTIC, OPTIMISED
    };
    
    PlayerActionGenerationEngine(GameRuleEngine* gameRuleEngine, GameActionEngine* gameActionEngine, ActionGenerationType actionGenerationType);
    
    void DecideAndPushNextActions(BoardState* currentBoardState);

private:
    bool IsCardHighPriority(const CardData& cardData, BoardState* currentBoardState) const;
    
private:
    struct LastPlayedCardData
    {
        int mPlayerIndex = -1;
        int mCardId = -1;
    };
    
    GameRuleEngine* mGameRuleEngine;
    GameActionEngine* mGameActionEngine;
    const ActionGenerationType mActionGenerationType;
    LastPlayedCardData mLastPlayedCard;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerActionGenerationEngine_h */
