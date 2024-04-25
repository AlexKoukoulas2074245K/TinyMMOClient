///------------------------------------------------------------------------------------------------
///  InsectMegaSwarmGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/02/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/InsectMegaSwarmGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

static const float DUPLICATED_CARD_Z_OFFSET = -0.01f;
static const float DUPLICATED_CARD_INIT_SCALE_FACTOR = 0.01f;
static const float DUPLICATION_ANIMATION_SECS_DURATION = 1.0f;

static const glm::vec3 NEW_CARD_TARGET_SCALE = {-0.091f, 0.084f, 0.666f};

static const std::string SPRING_SFX = "sfx_spring";

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
};

///------------------------------------------------------------------------------------------------

void InsectMegaSwarmGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    
    for (int i = 0; i < 3; ++i)
    {
        auto randomCardId = activePlayerState.mPlayerDeckCards[math::ControlledRandomInt() % activePlayerState.mPlayerDeckCards.size()];
        auto cardData = CardDataRepository::GetInstance().GetCardData(randomCardId, mBoardState->GetActivePlayerIndex());
        
        while (cardData.IsSpell())
        {
            randomCardId = activePlayerState.mPlayerDeckCards[math::ControlledRandomInt() % activePlayerState.mPlayerDeckCards.size()];
            cardData = CardDataRepository::GetInstance().GetCardData(randomCardId, mBoardState->GetActivePlayerIndex());
        }
        
        activePlayerState.mPlayerBoardCards.push_back(randomCardId);
    }
}

///------------------------------------------------------------------------------------------------

void InsectMegaSwarmGameAction::VInitAnimation()
{
    mFinished = false;
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    const auto& boardCards = mBoardState->GetActivePlayerState().mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetActivePlayerState().mBoardCardIndicesToDestroy;
    const auto nonDeadBoardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(SPRING_SFX);
    
    std::vector<std::shared_ptr<CardSoWrapper>> newCardSoWrappers;
    for (auto i = mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 3; i < mBoardState->GetActivePlayerState().mPlayerBoardCards.size(); ++i)
    {
        const auto& cardData = CardDataRepository::GetInstance().GetCardData(boardCards[i], mBoardState->GetActivePlayerIndex());
        
        auto targetPosition = card_utils::CalculateBoardCardPosition(static_cast<int>(i), nonDeadBoardCardCount, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);

        newCardSoWrappers.emplace_back(card_utils::CreateCardSoWrapper
        (
            &cardData,
            targetPosition,
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
        
        if (i == 0)
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(SPRING_SFX);
        }
        else
        {
            systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(i * DUPLICATION_ANIMATION_SECS_DURATION/3), [=]()
            {
                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(SPRING_SFX, false, 1.0f, 1.0f + i * 0.2f);
            });
        }
        
        systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(newCardSoWrappers.back()->mSceneObject, targetPosition, NEW_CARD_TARGET_SCALE, DUPLICATION_ANIMATION_SECS_DURATION, animation_flags::NONE, i * DUPLICATION_ANIMATION_SECS_DURATION/3, math::ElasticFunction, math::TweeningMode::EASE_IN), [=]()
        {
            if (i == mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1)
            {
                mFinished = true;
            }
        });
    }
    
    events::EventSystem::GetInstance().DispatchEvent<events::CardSummoningEvent>(newCardSoWrappers);
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult InsectMegaSwarmGameAction::VUpdateAnimation(const float)
{
    return mFinished ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool InsectMegaSwarmGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& InsectMegaSwarmGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
