///------------------------------------------------------------------------------------------------
///  SpellKillGameAction.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 11/03/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/SpellKillGameAction.h>
#include <game/gameactions/CardDestructionGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string TRAP_TRIGGERED_SFX = "sfx_trap_triggered";

static const strutils::StringId CARD_DESTRUCTION_GAME_ACTION_NAME = strutils::StringId("CardDestructionGameAction");

static const glm::vec3 TARGET_EFFECT_SCALE = {0.15f, 0.15f, 0.15f};

static const float ANIMATION_STEP_DURATION = 1.5f;
static const float ANIMATION_MAX_ALPHA = 0.8f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
};

///------------------------------------------------------------------------------------------------

void SpellKillGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    assert(!activePlayerState.mPlayerBoardCards.empty());
    
    mGameActionEngine->AddGameAction(CARD_DESTRUCTION_GAME_ACTION_NAME,
    {
        { CardDestructionGameAction::CARD_INDICES_PARAM, {"[" + std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) + "]"}},
        { CardDestructionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
        { CardDestructionGameAction::IS_BOARD_CARD_PARAM, "true"},
        { CardDestructionGameAction::IS_TRAP_TRIGGER_PARAM, "true"},
    });
}

///------------------------------------------------------------------------------------------------

void SpellKillGameAction::VInitAnimation()
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    const auto lastPlayedBoardCardIndex = mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1;
    auto lastPlayedCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[mBoardState->GetActivePlayerIndex()].at(lastPlayedBoardCardIndex);
    
    mAnimationState = ActionState::ANIMATION_STEP_WAIT;
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(TRAP_TRIGGERED_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(TRAP_TRIGGERED_SFX);
    
    std::shared_ptr<scene::SceneObject> killEffectSceneObject = nullptr;
    
    killEffectSceneObject = scene->FindSceneObject(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::SPELL_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::SPELL_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    animationManager.StopAllAnimationsPlayingForSceneObject(killEffectSceneObject->mName);
    
    auto targetPosition = killEffectSceneObject->mPosition;
    targetPosition.z = lastPlayedCardSoWrapper->mSceneObject->mPosition.z + 0.1f;
    
    auto targetScale = TARGET_EFFECT_SCALE;
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(killEffectSceneObject, ANIMATION_MAX_ALPHA, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(killEffectSceneObject, targetPosition, targetScale, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        mAnimationState = ActionState::FINISHED;
    });
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult SpellKillGameAction::VUpdateAnimation(const float)
{
    switch (mAnimationState)
    {
        case ActionState::ANIMATION_STEP_WAIT:
        {
            
        } break;
         
        case ActionState::FINISHED:
        {
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, false,  effects::board_modifier_masks::SPELL_KILL_NEXT);
            return ActionAnimationUpdateResult::FINISHED;
        }
    }
    
    return ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool SpellKillGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& SpellKillGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
