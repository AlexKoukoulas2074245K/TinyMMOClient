///------------------------------------------------------------------------------------------------
///  NextDinoDamageDoublingGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/11/2023
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
#include <game/gameactions/NextDinoDamageDoublingGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("CardBuffedDebuffedAnimationGameAction");
static const float CARD_SCALE_FACTOR = 2.2f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
};

///------------------------------------------------------------------------------------------------

void NextDinoDamageDoublingGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    assert(!activePlayerState.mPlayerBoardCards.empty());
    
    auto& playerBoardCardStatOverrides = activePlayerState.mPlayerBoardCardStatOverrides;
    const auto& playerBoardCards = activePlayerState.mPlayerBoardCards;
    
    if (playerBoardCardStatOverrides.size() > playerBoardCards.size() - 1 && playerBoardCardStatOverrides[playerBoardCards.size() -1].count(CardStatType::DAMAGE))
    {
        playerBoardCardStatOverrides[playerBoardCards.size() - 1][CardStatType::DAMAGE] *= 2;
    }
    else
    {
        playerBoardCardStatOverrides.resize(playerBoardCards.size());
        playerBoardCardStatOverrides[playerBoardCards.size() -1][CardStatType::DAMAGE] = CardDataRepository::GetInstance().GetCardData(playerBoardCards.back(), mBoardState->GetActivePlayerIndex()).mCardDamage * 2;
    }
    
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, false, effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE);
    
    mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
    {
        { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(playerBoardCards.size() - 1)},
        { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
        { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "true" },
        { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_FACTOR) }
    });
}

///------------------------------------------------------------------------------------------------

void NextDinoDamageDoublingGameAction::VInitAnimation()
{
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult NextDinoDamageDoublingGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool NextDinoDamageDoublingGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& NextDinoDamageDoublingGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
