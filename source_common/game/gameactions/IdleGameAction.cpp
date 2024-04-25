///------------------------------------------------------------------------------------------------
///  IdleGameAction.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/gameactions/IdleGameAction.h>

///------------------------------------------------------------------------------------------------

void IdleGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void IdleGameAction::VInitAnimation()
{
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult IdleGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool IdleGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& IdleGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
