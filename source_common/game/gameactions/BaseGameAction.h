///------------------------------------------------------------------------------------------------
///  BaseGameAction.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef BaseGameAction_h
#define BaseGameAction_h

///------------------------------------------------------------------------------------------------

#include <game/gameactions/IGameAction.h>
#include <game/BoardState.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

class BattleSceneLogicManager;
class GameActionEngine;
class GameRuleEngine;
class BaseGameAction: public IGameAction
{
    friend class GameActionEngine;
    
public:
    virtual ~BaseGameAction() = default;
    
    const strutils::StringId& VGetName() const override { return mName; }
    
protected:
    void SetName(const strutils::StringId& name) { mName = name; }
    void SetDependencies(BoardState* boardState, BattleSceneLogicManager* battleSceneLogicManager, GameRuleEngine* gameRuleEngine, GameActionEngine* gameActionEngine) { mBoardState = boardState; mBattleSceneLogicManager = battleSceneLogicManager; mGameRuleEngine = gameRuleEngine; mGameActionEngine = gameActionEngine; }
    void SetExtraActionParams(const std::unordered_map<std::string, std::string>& extraActionParams) { mExtraActionParams = extraActionParams; };
    
protected:
    std::unordered_map<std::string, std::string> mExtraActionParams;
    strutils::StringId mName = strutils::StringId();
    BoardState* mBoardState = nullptr;
    BattleSceneLogicManager* mBattleSceneLogicManager = nullptr;
    GameRuleEngine* mGameRuleEngine = nullptr;
    GameActionEngine* mGameActionEngine = nullptr;
};

///------------------------------------------------------------------------------------------------

#endif /* BaseGameAction_h */
