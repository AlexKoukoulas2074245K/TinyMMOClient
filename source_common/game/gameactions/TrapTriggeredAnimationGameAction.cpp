///------------------------------------------------------------------------------------------------
///  TrapTriggeredAnimationGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 17/11/2023
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/TrapTriggeredAnimationGameAction.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
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

const std::string TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_PARAM = "trapTriggerType";
const std::string TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_KILL = "trapTriggerTypeKill";
const std::string TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_DEBUFF = "trapTriggerTypeDebuff";
const std::string TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_PARAM = "killTrapType";
const std::string TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_BEAR_TRAP = "killTrapTypeBearTrap";
const std::string TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_DEMON_TRAP = "killTrapTypeDemonTrap";
const std::string TRAP_TRIGGERED_SFX = "sfx_trap_triggered";

static const strutils::StringId CARD_DESTRUCTION_GAME_ACTION_NAME = strutils::StringId("CardDestructionGameAction");
static const strutils::StringId CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("CardBuffedDebuffedAnimationGameAction");

static const float ANIMATION_STEP_DURATION = 0.75f;
static const float ANIMATION_MAX_ALPHA = 0.7f;
static const float ANIMATION_STEP_1_SCALE_FACTOR = 1.5f;
static const float ANIMATION_STEP_2_SCALE_FACTOR = 1.2f;
static const float ANIMATION_STEP_1_ROTATION_INCREMENT = math::PI/5;
static const float ANIMATION_STEP_2_ROTATION_INCREMENT = -math::PI/3;
static const float CARD_DEBUFF_SCALE_DOWN_FACTOR = 0.5f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_PARAM
};

///------------------------------------------------------------------------------------------------

void TrapTriggeredAnimationGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    assert(!activePlayerState.mPlayerBoardCards.empty());
    assert(mExtraActionParams.count(TRAP_TRIGGER_TYPE_PARAM) == 1);
    
    if (mExtraActionParams.at(TRAP_TRIGGER_TYPE_PARAM) == TRAP_TRIGGER_TYPE_KILL)
    {
        assert(mExtraActionParams.count(KILL_TRAP_TYPE_PARAM) == 1);
        mGameActionEngine->AddGameAction(CARD_DESTRUCTION_GAME_ACTION_NAME,
        {
            { CardDestructionGameAction::CARD_INDICES_PARAM, {"[" + std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) + "]"}},
            { CardDestructionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
            { CardDestructionGameAction::IS_BOARD_CARD_PARAM, "true"},
            { CardDestructionGameAction::IS_TRAP_TRIGGER_PARAM, "true"},
        });
    }
    else if (mExtraActionParams.at(TRAP_TRIGGER_TYPE_PARAM) == TRAP_TRIGGER_TYPE_DEBUFF)
    {
        mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
        {
            { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1)},
            { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
            { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "true" },
            { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_DEBUFF_SCALE_DOWN_FACTOR) }
        });
    }
}

///------------------------------------------------------------------------------------------------

void TrapTriggeredAnimationGameAction::VInitAnimation()
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    const auto lastPlayedBoardCardIndex = mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1;
    auto lastPlayedCardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[mBoardState->GetActivePlayerIndex()].at(lastPlayedBoardCardIndex);
    
    if (mExtraActionParams.at(TRAP_TRIGGER_TYPE_PARAM) == TRAP_TRIGGER_TYPE_KILL)
    {
        mAnimationState = ActionState::ANIMATION_STEP_WAIT;
        
        CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(TRAP_TRIGGERED_SFX);
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(TRAP_TRIGGERED_SFX);
        
        auto killTrapType = mExtraActionParams.at(KILL_TRAP_TYPE_PARAM);
        std::shared_ptr<scene::SceneObject> killEffectSceneObject = nullptr;
        
        if (killTrapType == KILL_TRAP_TYPE_BEAR_TRAP)
        {
            killEffectSceneObject = scene->FindSceneObject(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (killTrapType == KILL_TRAP_TYPE_DEMON_TRAP)
        {
            killEffectSceneObject = scene->FindSceneObject(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::DEMON_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DEMON_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        
        animationManager.StopAllAnimationsPlayingForSceneObject(killEffectSceneObject->mName);
        
        auto targetPosition = killEffectSceneObject->mPosition;
        targetPosition.z = lastPlayedCardSoWrapper->mSceneObject->mPosition.z + 0.1f;
        
        auto targetScale = killEffectSceneObject->mScale * ANIMATION_STEP_1_SCALE_FACTOR;
        auto targetRotation = killEffectSceneObject->mRotation;
        targetRotation.z += ANIMATION_STEP_1_ROTATION_INCREMENT;
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(killEffectSceneObject, targetPosition, targetScale, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(TRAP_TRIGGERED_SFX);
            mAnimationState = ActionState::ANIMATION_STEP_2;
        });
        animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(killEffectSceneObject, targetRotation, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::BounceFunction, math::TweeningMode::EASE_IN), [=](){});
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(killEffectSceneObject, ANIMATION_MAX_ALPHA, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    }
    else if (mExtraActionParams.at(TRAP_TRIGGER_TYPE_PARAM) == TRAP_TRIGGER_TYPE_DEBUFF)
    {
        mAnimationState = ActionState::FINISHED;
    }
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult TrapTriggeredAnimationGameAction::VUpdateAnimation(const float)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    switch (mAnimationState)
    {
        case ActionState::ANIMATION_STEP_WAIT:
        {
            
        } break;
            
        case ActionState::ANIMATION_STEP_2:
        {
            auto killTrapType = mExtraActionParams.at(KILL_TRAP_TYPE_PARAM);
            std::shared_ptr<scene::SceneObject> killEffectSceneObject = nullptr;
            
            if (killTrapType == KILL_TRAP_TYPE_BEAR_TRAP)
            {
                killEffectSceneObject = scene->FindSceneObject(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
            }
            else if (killTrapType == KILL_TRAP_TYPE_DEMON_TRAP)
            {
                killEffectSceneObject = scene->FindSceneObject(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::DEMON_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DEMON_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
            }
            
            auto targetScale = killEffectSceneObject->mScale * ANIMATION_STEP_2_SCALE_FACTOR;
            auto targetRotation = killEffectSceneObject->mRotation;
            targetRotation.z += ANIMATION_STEP_2_ROTATION_INCREMENT;
            
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(killEffectSceneObject, killEffectSceneObject->mPosition, targetScale, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
            
            animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(killEffectSceneObject, targetRotation, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::BounceFunction, math::TweeningMode::EASE_IN), [=]()
            {
                mAnimationState = ActionState::FINISHED;
            });
            
            mAnimationState = ActionState::ANIMATION_STEP_WAIT;
        } break;
    
        case ActionState::FINISHED:
        {
            if (mExtraActionParams.at(TRAP_TRIGGER_TYPE_PARAM) == TRAP_TRIGGER_TYPE_KILL)
            {
                auto killTrapType = mExtraActionParams.at(KILL_TRAP_TYPE_PARAM);
                if (killTrapType == KILL_TRAP_TYPE_BEAR_TRAP)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, false,  effects::board_modifier_masks::KILL_NEXT);
                }
                else if (killTrapType == KILL_TRAP_TYPE_DEMON_TRAP)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, false,  effects::board_modifier_masks::DEMON_KILL_NEXT);
                }
            }
            return ActionAnimationUpdateResult::FINISHED;
        }
    }
    
    return ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool TrapTriggeredAnimationGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& TrapTriggeredAnimationGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
