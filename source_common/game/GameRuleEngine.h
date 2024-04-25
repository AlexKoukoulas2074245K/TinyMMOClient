///------------------------------------------------------------------------------------------------
///  GameRuleEngine.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 26/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameRuleEngine_h
#define GameRuleEngine_h

///------------------------------------------------------------------------------------------------

#include <memory>

///------------------------------------------------------------------------------------------------

class BoardState;
struct CardData;
class GameRuleEngine final
{
public:
    GameRuleEngine(BoardState* boardState);
    
    bool CanCardBePlayed(const CardData* cardData, const size_t cardIndex, const size_t forPlayerIndex, BoardState* customBoardStateOverride = nullptr) const;
    
private:
    BoardState* mBoardState;
};

///------------------------------------------------------------------------------------------------

#endif /* GameRuleEngine_h */
