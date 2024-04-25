///------------------------------------------------------------------------------------------------
///  InsectVirusGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/02/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameOverResurrectionCheckGameAction.h>
#include <game/gameactions/InsectVirusGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId GAME_OVER_CHECK_GAME_ACTION_NAME = strutils::StringId("GameOverResurrectionCheckGameAction");

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
};

///------------------------------------------------------------------------------------------------

void InsectVirusGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    
    if (activePlayerState.mPlayerCurrentArmor > 0)
    {
        activePlayerState.mPlayerCurrentArmor--;
        events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, mBoardState->GetActivePlayerState().mPlayerCurrentArmor);
    }
    else
    {
        activePlayerState.mPlayerHealth--;
        events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
    }
    
    int oldHealth = mBoardState->GetInactivePlayerState().mPlayerHealth;
    if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
    {
        mBoardState->GetInactivePlayerState().mPlayerHealth = math::Min(mBoardState->GetInactivePlayerState().mPlayerHealth + 1, DataRepository::GetInstance().GetStoryMaxHealth());
    }
    else
    {
        mBoardState->GetInactivePlayerState().mPlayerHealth++;
    }
    
    if (oldHealth != mBoardState->GetInactivePlayerState().mPlayerHealth)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
    }
    
    if (activePlayerState.mPlayerHealth <= 0)
    {
        activePlayerState.mPlayerHealth = 0;
        mGameActionEngine->AddGameAction(GAME_OVER_CHECK_GAME_ACTION_NAME,
        {
            { GameOverResurrectionCheckGameAction::VICTORIOUS_PLAYER_INDEX_PARAM, std::to_string((mBoardState->GetActivePlayerIndex() + 1) % mBoardState->GetPlayerCount())}
        });
    }
}

///------------------------------------------------------------------------------------------------

void InsectVirusGameAction::VInitAnimation()
{
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult InsectVirusGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool InsectVirusGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& InsectVirusGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
