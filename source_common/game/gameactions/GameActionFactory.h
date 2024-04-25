///------------------------------------------------------------------------------------------------
///  GameActionFactory.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameActionFactory_h
#define GameActionFactory_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <memory>
#include <unordered_set>

///------------------------------------------------------------------------------------------------

class BaseGameAction;
class GameActionFactory final
{
    friend class GameActionEngine;
    friend class Game;
    
public:
    static const std::vector<strutils::StringId>& GetRegisteredActions();
    
private:
    GameActionFactory() = delete;
    
    static void RegisterGameActions();
    static std::unique_ptr<BaseGameAction> CreateGameAction(const strutils::StringId& actionName);
};

///------------------------------------------------------------------------------------------------

#endif /* GameActionFactory_h */
