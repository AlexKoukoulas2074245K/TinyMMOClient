///------------------------------------------------------------------------------------------------
///  CardBuffedDebuffedAnimationGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/11/2023
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM = "cardIndex";
const std::string CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM = "playerIndex";
const std::string CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM = "isBoardCard";
const std::string CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM = "scaleFactor";
const std::string CardBuffedDebuffedAnimationGameAction::PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM = "particleEmitterNameToRemove";
const std::string CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX = "cardBuffedRepeatIndex";

static const std::string BUFF_SFX = "sfx_power_up";

static const float CARD_SCALE_ANIMATION_MIN_DURATION_SECS = 0.6f;
static const float CARD_SCALE_ANIMATION_MIN_SCALE_FACTOR = 1.5f;
static const float CARD_SCALE_ANIMATION_TARGET_Z = 10.0f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM,
    CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM,
    CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM,
    CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM,
};

///------------------------------------------------------------------------------------------------

void CardBuffedDebuffedAnimationGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void CardBuffedDebuffedAnimationGameAction::VInitAnimation()
{
    mFinished = false;
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(BUFF_SFX);
    
    const auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
    const auto playerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
    const auto isBoardCard = mExtraActionParams.at(IS_BOARD_CARD_PARAM) == "true";
    const auto scaleFactor = std::stof(mExtraActionParams.at(SCALE_FACTOR_PARAM));
    const auto particleEmitterNameToRemove =
        mExtraActionParams.count(PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM) ?
        strutils::StringId(mExtraActionParams.at(PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM)) :
        strutils::StringId();
    
    auto cardSoWrapper = isBoardCard ?
        mBattleSceneLogicManager->GetBoardCardSoWrappers().at(playerIndex).at(cardIndex):
        mBattleSceneLogicManager->GetHeldCardSoWrappers().at(playerIndex).at(cardIndex);
    
    auto targetDuration = CARD_SCALE_ANIMATION_MIN_DURATION_SECS + math::Max(0.0f, (scaleFactor - CARD_SCALE_ANIMATION_MIN_SCALE_FACTOR)/2);
    auto originalScale = cardSoWrapper->mSceneObject->mScale;
    auto originalPosition = cardSoWrapper->mSceneObject->mPosition;
    auto targetPosition = originalPosition;
    targetPosition.z += CARD_SCALE_ANIMATION_TARGET_Z;
    
    if (scaleFactor > 1.0f)
    {
        auto extraPitchPerRepeat = mExtraActionParams.count(CARD_BUFFED_REPEAT_INDEX) ? 0.1f * std::stoi(mExtraActionParams.at(CARD_BUFFED_REPEAT_INDEX)) : 0.0f;
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(BUFF_SFX, false, 1.0f, 1.0f + extraPitchPerRepeat);
    }
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, targetPosition, originalScale * scaleFactor, targetDuration/2, animation_flags::IGNORE_X_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        if (!particleEmitterNameToRemove.isEmpty())
        {
            CoreSystemsEngine::GetInstance().GetParticleManager().RemoveParticleEmitterFlag(particle_flags::CONTINUOUS_PARTICLE_GENERATION, particleEmitterNameToRemove, *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE));
        }
        
        events::EventSystem::GetInstance().DispatchEvent<events::CardBuffedDebuffedEvent>(static_cast<int>(cardIndex), isBoardCard, playerIndex == game_constants::REMOTE_PLAYER_INDEX);
        
        auto cardSoWrapper = isBoardCard ?
            mBattleSceneLogicManager->GetBoardCardSoWrappers().at(playerIndex).at(cardIndex):
            mBattleSceneLogicManager->GetHeldCardSoWrappers().at(playerIndex).at(cardIndex);
        
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, originalPosition, originalScale, targetDuration/2, animation_flags::IGNORE_X_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
        {
            mFinished = true;
        });
    });
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult CardBuffedDebuffedAnimationGameAction::VUpdateAnimation(const float)
{
    return mFinished ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool CardBuffedDebuffedAnimationGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& CardBuffedDebuffedAnimationGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
