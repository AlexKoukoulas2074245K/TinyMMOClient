///------------------------------------------------------------------------------------------------
///  DinoDamageReversalGameAction.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 10/03/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/GameConstants.h>
#include <game/GameRuleEngine.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
#include <game/gameactions/DinoDamageReversalGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("CardBuffedDebuffedAnimationGameAction");
static const strutils::StringId CARD_SPELL_EFFECT_PARTICLE_NAME = strutils::StringId("card_spell_effect");
static const strutils::StringId CARD_EFFECT_PARTICLE_EMITTER_NAME = strutils::StringId("card_effect_emitter");

static const std::string BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX = "card_effect_emitter_";

static const float CARD_SCALE_UP_FACTOR = 1.5f;
static const float CARD_SCALE_DOWN_FACTOR = 0.5f;
static const float CARD_EFFECT_PARTICLE_EMITTER_Z_OFFSET = 21.0f;
static const float REVERSAL_SPARKLES_LIFETIME_SECS = 1.0f;

///------------------------------------------------------------------------------------------------

void DinoDamageReversalGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    std::vector<int> applicableHeldCardIndices;
    
    for (auto i = 0; i < activePlayerState.mPlayerHeldCards.size(); ++i)
    {
        const auto& cardData = CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[i], mBoardState->GetActivePlayerIndex());
        if (cardData.mCardFamily == game_constants::DINOSAURS_FAMILY_NAME && !cardData.IsSpell())
        {
            applicableHeldCardIndices.push_back(i);
        }
    }
    
    if (applicableHeldCardIndices.size() < 2)
    {
        return;
    }
    
    std::sort(applicableHeldCardIndices.begin(), applicableHeldCardIndices.end(), [&](const int heldCardIndexLhs, const int heldCardIndexRhs)
    {
        return CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[heldCardIndexLhs], mBoardState->GetActivePlayerIndex()).mCardDamage < CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[heldCardIndexRhs], mBoardState->GetActivePlayerIndex()).mCardDamage;
    });
    
    mLowestDamageHeldCardIndex = applicableHeldCardIndices.front();
    mHighestDamageHeldCardIndex = applicableHeldCardIndices.back();
    
    auto lowestDamage = CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[mLowestDamageHeldCardIndex], mBoardState->GetActivePlayerIndex()).mCardDamage;
    auto highestDamage = CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[mHighestDamageHeldCardIndex], mBoardState->GetActivePlayerIndex()).mCardDamage;
    
    bool shouldAnimateCardBuffing = lowestDamage != highestDamage;
    
    auto largestHeldCardIndex = math::Max(mLowestDamageHeldCardIndex, mHighestDamageHeldCardIndex);
    
    // Make sure the held card stat overrides have sufficient size
    if (static_cast<int>(mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size()) <= largestHeldCardIndex)
    {
        mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.resize(largestHeldCardIndex + 1);
    }
    
    // If lowest damage card has damage overrides, set the lowest damage to those instead
    if (mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[mLowestDamageHeldCardIndex].count(CardStatType::DAMAGE) != 0)
    {
        lowestDamage = mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[mLowestDamageHeldCardIndex][CardStatType::DAMAGE];
    }
    
    // // If highest damage card has damage overrides, set the highest damage to those instead
    if (mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[mHighestDamageHeldCardIndex].count(CardStatType::DAMAGE) != 0)
    {
        highestDamage = mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[mHighestDamageHeldCardIndex][CardStatType::DAMAGE];
    }
    
    mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[mLowestDamageHeldCardIndex][CardStatType::DAMAGE] = highestDamage;
    mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[mHighestDamageHeldCardIndex][CardStatType::DAMAGE] = lowestDamage;
    
    // Skip animation for held cards for opponent
    if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX || !shouldAnimateCardBuffing)
    {
        return;
    }
        
    events::EventSystem::GetInstance().DispatchEvent<events::ForceSendCardBackToPositionEvent>(static_cast<int>(mLowestDamageHeldCardIndex), false, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
    
    events::EventSystem::GetInstance().DispatchEvent<events::ForceSendCardBackToPositionEvent>(static_cast<int>(mHighestDamageHeldCardIndex), false, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
    
    mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
    {
        { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(mLowestDamageHeldCardIndex)},
        { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
        { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "false" },
        { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_UP_FACTOR) },
        { CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX, "0" },
        { CardBuffedDebuffedAnimationGameAction::PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM, BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX + std::to_string(mLowestDamageHeldCardIndex) }
    });
    
    mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
    {
        { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, std::to_string(mHighestDamageHeldCardIndex)},
        { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())},
        { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "false" },
        { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_DOWN_FACTOR) },
        { CardBuffedDebuffedAnimationGameAction::CARD_BUFFED_REPEAT_INDEX, "0" },
        { CardBuffedDebuffedAnimationGameAction::PARTICLE_EMITTER_NAME_TO_REMOVE_PARAM, BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX + std::to_string(mHighestDamageHeldCardIndex) }
    });
}

///------------------------------------------------------------------------------------------------

void DinoDamageReversalGameAction::VInitAnimation()
{
    mFinished = false;
    
    if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
    {
        mFinished = true;
        return;
    }
    
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    const auto& scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    const auto& heldCards = activePlayerState.mPlayerHeldCards;
    const auto& deadHeldCardIndices = activePlayerState.mHeldCardIndicesToDestroy;
    
    for (size_t i = 0U; i < heldCards.size(); ++i)
    {
        if (i != mLowestDamageHeldCardIndex && i != mHighestDamageHeldCardIndex)
        {
            continue;
        }
        
        auto cardSoWrapper = mBattleSceneLogicManager->GetHeldCardSoWrappers().at(mBoardState->GetActivePlayerIndex()).at(i);
        auto targetPosition = card_utils::CalculateHeldCardPosition(static_cast<int>(i), card_utils::CalculateNonDeadCardsCount(activePlayerState.mPlayerHeldCards, deadHeldCardIndices), mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, scene->GetCamera());
        
        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
        (
            CARD_SPELL_EFFECT_PARTICLE_NAME,
            glm::vec3(targetPosition.x, targetPosition.y, CARD_EFFECT_PARTICLE_EMITTER_Z_OFFSET), // pos
            *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE), // scene
            strutils::StringId(BUFFED_CARD_PARTICLE_EMITTER_NAME_PREFIX + std::to_string(i))
        );
    }
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(REVERSAL_SPARKLES_LIFETIME_SECS), [=]()
    {
        mFinished = true;
    });
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult DinoDamageReversalGameAction::VUpdateAnimation(const float)
{
    return mFinished ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool DinoDamageReversalGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& DinoDamageReversalGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
