///------------------------------------------------------------------------------------------------
///  HowToPlayACardTutorialGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#include <game/gameactions/HowToPlayACardTutorialGameAction.h>
#include <game/BoardState.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/TutorialManager.h>

///------------------------------------------------------------------------------------------------

void HowToPlayACardTutorialGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void HowToPlayACardTutorialGameAction::VInitAnimation()
{
    events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_HOW_TO_PLAY_A_CARD_TUTORIAL);
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult HowToPlayACardTutorialGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool HowToPlayACardTutorialGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& HowToPlayACardTutorialGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
