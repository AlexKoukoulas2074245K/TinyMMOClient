///------------------------------------------------------------------------------------------------
///  IGameAction.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef IGameAction_h
#define IGameAction_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <vector>

///------------------------------------------------------------------------------------------------

enum class ActionAnimationUpdateResult
{
    ONGOING,
    FINISHED
};

///------------------------------------------------------------------------------------------------

class IGameAction
{
public:
    virtual ~IGameAction() = default;
    
    virtual const strutils::StringId& VGetName() const = 0;
    
    // To be called directly by the engine. This
    // needs to set the final board/game state post this action
    // (before the animations actually run) for game integrity purposes.
    virtual void VSetNewGameState() = 0;
    
    virtual void VInitAnimation() = 0;
    
    virtual ActionAnimationUpdateResult VUpdateAnimation(const float dtMillis) = 0;
    
    // Used to deferentiate between actions that are automatically created on the fly
    // by other actions (win conditions, attacks) and which themselves should not be serialized
    // to a game file otherwise they would be duplicated by the replay flow
    virtual bool VShouldBeSerialized() const = 0;
    
    // To be used primarily by IMGUI debug widgets for properly generating
    // actions that require extra params
    virtual const std::vector<std::string>& VGetRequiredExtraParamNames() const = 0;
};

///------------------------------------------------------------------------------------------------

#endif /* IGameAction_h */
