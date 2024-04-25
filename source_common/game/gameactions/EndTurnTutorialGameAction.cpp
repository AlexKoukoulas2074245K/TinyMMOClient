///------------------------------------------------------------------------------------------------
///  EndTurnTutorialGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/scene/Scene.h>
#include <engine/scene/SceneManager.h>
#include <game/gameactions/EndTurnTutorialGameAction.h>
#include <game/BoardState.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/TutorialManager.h>

///------------------------------------------------------------------------------------------------

void EndTurnTutorialGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void EndTurnTutorialGameAction::VInitAnimation()
{
    auto battleScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    auto turnPointerSceneObject = battleScene->FindSceneObject(game_constants::TURN_POINTER_SCENE_OBJECT_NAME);
    auto turnPointerPosition = turnPointerSceneObject->mPosition;
    turnPointerPosition.x *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
    
    auto tutorialArrowOriginPosition = turnPointerPosition;
    tutorialArrowOriginPosition.y -= 0.1f;
    
    events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_END_TURN_TUTORIAL, tutorialArrowOriginPosition, turnPointerPosition);
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult EndTurnTutorialGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool EndTurnTutorialGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& EndTurnTutorialGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
