///------------------------------------------------------------------------------------------------
///  HoundSummoningGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 20/02/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/HoundSummoningGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string HoundSummoningGameAction::NUMBER_OF_HOUNDS_PARAM = "numberOfHounds";

static const strutils::StringId CARD_PLAY_PARTICLE_NAME = strutils::StringId("card_play");
static const strutils::StringId TOP_PLAYER_HEALTH_CONTAINER_BASE = strutils::StringId("health_crystal_top_base");
static const strutils::StringId TOP_PLAYER_HEALTH_CONTAINER_VALUE = strutils::StringId("health_crystal_top_value");
static const strutils::StringId CARD_PLAY_PARTICLE_EMITTER_NAME = strutils::StringId("card_play_emitter");

static const float DUPLICATED_CARD_Z_OFFSET = -0.01f;
static const float DUPLICATED_CARD_INIT_SCALE_FACTOR = 0.01f;
static const float DUPLICATION_ANIMATION_SECS_DURATION = 1.0f;
static const float CARD_CAMERA_SHAKE_DURATION = 0.25f;
static const float CARD_CAMERA_SHAKE_STRENGTH = 0.005f;
static const float CARD_PLAY_PARTICLE_EMITTER_Z = 0.01f;
static const glm::vec3 NEW_CARD_TARGET_SCALE = {-0.091f, 0.084f, 0.666f};

static const std::string ROAR_SFX = "sfx_roar";

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    HoundSummoningGameAction::NUMBER_OF_HOUNDS_PARAM
};

///------------------------------------------------------------------------------------------------

void HoundSummoningGameAction::VSetNewGameState()
{
    assert(mExtraActionParams.count(NUMBER_OF_HOUNDS_PARAM) == 1);
    const auto numberOfHounds = std::stoi(mExtraActionParams.at(NUMBER_OF_HOUNDS_PARAM));
    
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    
    std::vector<int> houndCardIds;
    const auto& genericDemonCardIds = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::DEMONS_GENERIC_FAMILY_NAME);
    
    for (int i = 0; i < numberOfHounds; ++i)
    {
        auto randomCardId = genericDemonCardIds[math::ControlledRandomInt() % genericDemonCardIds.size()];
        auto cardData = CardDataRepository::GetInstance().GetCardData(randomCardId, mBoardState->GetActivePlayerIndex());
        
        while (!strutils::StringEndsWith(cardData.mCardName.GetString(), "Hound"))
        {
            randomCardId = genericDemonCardIds[math::ControlledRandomInt() % genericDemonCardIds.size()];
            cardData = CardDataRepository::GetInstance().GetCardData(randomCardId, mBoardState->GetActivePlayerIndex());
        }
        
        activePlayerState.mPlayerBoardCards.push_back(randomCardId);
    }
}

///------------------------------------------------------------------------------------------------

void HoundSummoningGameAction::VInitAnimation()
{
    mFinished = false;
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    const auto numberOfHounds = std::stoi(mExtraActionParams.at(NUMBER_OF_HOUNDS_PARAM));
    const auto& boardCards = mBoardState->GetActivePlayerState().mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetActivePlayerState().mBoardCardIndicesToDestroy;
    const auto nonDeadBoardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(ROAR_SFX);
    
    systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(DUPLICATION_ANIMATION_SECS_DURATION/2), [=]()
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(ROAR_SFX);
    });
    
    std::vector<std::shared_ptr<CardSoWrapper>> newCardSoWrappers;
    for (auto i = mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - numberOfHounds; i < mBoardState->GetActivePlayerState().mPlayerBoardCards.size(); ++i)
    {
        const auto& cardData = CardDataRepository::GetInstance().GetCardData(boardCards[i], mBoardState->GetActivePlayerIndex());
        
        auto targetPosition = card_utils::CalculateBoardCardPosition(static_cast<int>(i), nonDeadBoardCardCount, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);

        newCardSoWrappers.emplace_back(card_utils::CreateCardSoWrapper
        (
            &cardData,
            glm::vec3(0.0f, 1.0f, 0.0f),
            (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX) + std::to_string(mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1),
            CardOrientation::FRONT_FACE,
            card_utils::GetCardRarity(cardData.mCardId, mBoardState->GetActivePlayerIndex(), *mBoardState),
            true,
            mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX,
            true,
            (mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.size() > i ? mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.at(i) : CardStatOverrides()), // held card stat overrides have moved to board card stat overrides from the setstate above
            mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers,
            *scene
        ));
        newCardSoWrappers.back()->mSceneObject->mPosition.z += DUPLICATED_CARD_Z_OFFSET;
        newCardSoWrappers.back()->mSceneObject->mScale *= DUPLICATED_CARD_INIT_SCALE_FACTOR;
        
        systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(newCardSoWrappers.back()->mSceneObject, targetPosition, NEW_CARD_TARGET_SCALE, DUPLICATION_ANIMATION_SECS_DURATION, animation_flags::NONE, i * DUPLICATION_ANIMATION_SECS_DURATION/3, math::LinearFunction, math::TweeningMode::EASE_IN), [=]()
        {
            card_utils::PlayCardPlaySfx(&cardData);
            
            CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(CARD_CAMERA_SHAKE_DURATION, CARD_CAMERA_SHAKE_STRENGTH);
            CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
            (
                CARD_PLAY_PARTICLE_NAME,
                glm::vec3(targetPosition.x, targetPosition.y, CARD_PLAY_PARTICLE_EMITTER_Z),
                *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE),
                CARD_PLAY_PARTICLE_EMITTER_NAME
            );
            
            if (i == mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1)
            {
                mFinished = true;
            }
        });
    }
    
    events::EventSystem::GetInstance().DispatchEvent<events::CardSummoningEvent>(newCardSoWrappers);
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult HoundSummoningGameAction::VUpdateAnimation(const float)
{
    return mFinished ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool HoundSummoningGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& HoundSummoningGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
