///------------------------------------------------------------------------------------------------
///  PlayCardGameAction.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/TrapTriggeredAnimationGameAction.h>
#include <game/gameactions/CardEffectGameAction.h>
#include <game/gameactions/CardHistoryEntryAdditionGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/PlayCardGameAction.h>
#include <game/gameactions/InsectDuplicationGameAction.h>
#include <game/GameRuleEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <game/TutorialManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string PlayCardGameAction::LAST_PLAYED_CARD_INDEX_PARAM = "lastPlayedCardIndex";
const std::string CARD_PLAY_SFX = "sfx_card_play";

static const strutils::StringId UNSEEN_SPELL_SCENE_NAME = strutils::StringId("unseen_spell_scene");
static const strutils::StringId CARD_EFFECT_GAME_ACTION_NAME = strutils::StringId("CardEffectGameAction");
static const strutils::StringId TRAP_TRIGGERED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("TrapTriggeredAnimationGameAction");
static const strutils::StringId GOLDEN_CARD_PLAYED_EFFECT_GAME_ACTION_NAME = strutils::StringId("GoldenCardPlayedEffectGameAction");
static const strutils::StringId CARD_PLAYED_PARTICLE_EFFECT_GAME_ACTION_NAME = strutils::StringId("CardPlayedParticleEffectGameAction");
static const strutils::StringId INSECT_DUPLICATION_GAME_ACTION_NAME = strutils::StringId("InsectDuplicationGameAction");
static const strutils::StringId NEXT_DINO_DAMAGE_DOUBLING_GAME_ACTION_NAME = strutils::StringId("NextDinoDamageDoublingGameAction");
static const strutils::StringId CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME = strutils::StringId("CardHistoryEntryAdditionGameAction");
static const strutils::StringId CARD_PLAY_PARTICLE_NAME = strutils::StringId("card_play");
static const strutils::StringId HEAL_NEXT_DINO_DAMAGE_GAME_ACTION_NAME = strutils::StringId("HealNextDinoDamageGameAction");
static const strutils::StringId INSECT_VIRUS_GAME_ACTION_NAME = strutils::StringId("InsectVirusGameAction");
static const strutils::StringId SPELL_KILL_GAME_ACTION_NAME = strutils::StringId("SpellKillGameAction");
static const strutils::StringId ZERO_COST_TIME_GAME_ACTION_NAME = strutils::StringId("ZeroCostTimeGameAction");
static const strutils::StringId HISTORY_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("history_button");
static const strutils::StringId END_TURN_TUTORIAL_GAME_ACTION_NAME = strutils::StringId("EndTurnTutorialGameAction");

static const float CARD_CAMERA_SHAKE_DURATION = 0.25f;
static const float CARD_CAMERA_SHAKE_STRENGTH = 0.005f;
static const float CARD_PLAY_PARTICLE_EMITTER_Z = 0.01f;
static const float IN_GAME_PLAYED_CARD_ANIMATION_DURATION = 0.4f;
static const float CARD_PLAY_PROTRUDED_Y_OFFSET = 0.06f;
static const float CARDS_MAKING_SPACE_Y_OFFSET = 0.025f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    PlayCardGameAction::LAST_PLAYED_CARD_INDEX_PARAM
};

///------------------------------------------------------------------------------------------------

void PlayCardGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    assert(!activePlayerState.mPlayerHeldCards.empty());
    assert(mExtraActionParams.count(LAST_PLAYED_CARD_INDEX_PARAM) != 0);
    
    auto lastPlayedCardIndex = std::stoi(mExtraActionParams.at(LAST_PLAYED_CARD_INDEX_PARAM));
    auto cardId = activePlayerState.mPlayerHeldCards[lastPlayedCardIndex];
    auto cardData = CardDataRepository::GetInstance().GetCardData(cardId, mBoardState->GetActivePlayerIndex());
    
    // Tried to overplay?
    mAborted = mGameRuleEngine && !mGameRuleEngine->CanCardBePlayed(&cardData, lastPlayedCardIndex, mBoardState->GetActivePlayerIndex());
    if (mAborted)
    {
        return;
    }
    
    auto cardWeight = cardData.mCardWeight;
    const auto& cardStatOverrides = activePlayerState.mPlayerHeldCardStatOverrides;
    
    if (static_cast<int>(cardStatOverrides.size()) > lastPlayedCardIndex)
    {
        cardWeight = math::Max(0, cardStatOverrides[lastPlayedCardIndex].count(CardStatType::WEIGHT) ? cardStatOverrides[lastPlayedCardIndex].at(CardStatType::WEIGHT) : cardData.mCardWeight);
    }
    if (!cardData.IsSpell() && activePlayerState.mBoardModifiers.mGlobalCardStatModifiers.count(CardStatType::WEIGHT))
    {
        cardWeight = math::Max(0, cardWeight + activePlayerState.mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::WEIGHT));
    }
    
    // Transfer held card stat override to the new board position
    if (static_cast<int>(mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size()) > lastPlayedCardIndex)
    {
        if (!mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[lastPlayedCardIndex].empty())
        {
            mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.resize(activePlayerState.mPlayerBoardCards.size() + 1);
            mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides[activePlayerState.mPlayerBoardCards.size()] = mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[lastPlayedCardIndex];
        }
        
        mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.erase(mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.begin() + lastPlayedCardIndex);
    }
    
    activePlayerState.mPlayerBoardCards.push_back(cardId);
    activePlayerState.mPlayerHeldCards.erase(activePlayerState.mPlayerHeldCards.begin() + lastPlayedCardIndex);
    activePlayerState.mPlayerCurrentWeightAmmo -= cardWeight;
    
    // Card-specific particle animation
    if (!cardData.mParticleEffect.isEmpty())
    {
        mGameActionEngine->AddGameAction(CARD_PLAYED_PARTICLE_EFFECT_GAME_ACTION_NAME);
    }
    
    if (cardData.IsSpell())
    {
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::SPELL_KILL_NEXT) != 0)
        {
            mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
            {
                { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex()) },
                { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) },
                { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_DEATH },
                { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
            });
            
            mGameActionEngine->AddGameAction(SPELL_KILL_GAME_ACTION_NAME);
            activePlayerState.mBoardModifiers.mBoardModifierMask &= (~effects::board_modifier_masks::SPELL_KILL_NEXT);
            return;
        }
        
        mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
        {
            { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex()) },
            { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) },
            { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_EFFECT },
            { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
        });
        
        mGameActionEngine->AddGameAction(CARD_EFFECT_GAME_ACTION_NAME);
    }
    else // normal card
    {
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::KILL_NEXT) != 0)
        {
            mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
            {
                { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex()) },
                { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) },
                { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_DEATH },
                { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
            });
            
            mGameActionEngine->AddGameAction(TRAP_TRIGGERED_ANIMATION_GAME_ACTION_NAME,
            {
                { TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_PARAM, TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_KILL },
                { TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_PARAM, TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_BEAR_TRAP }
            });
            activePlayerState.mBoardModifiers.mBoardModifierMask &= (~effects::board_modifier_masks::KILL_NEXT);
            return;
        }
        
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DEMON_KILL_NEXT) != 0)
        {
            mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
            {
                { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex()) },
                { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, std::to_string(activePlayerState.mPlayerBoardCards.size() - 1) },
                { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_DEATH },
                { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "false"}
            });
            
            mGameActionEngine->AddGameAction(TRAP_TRIGGERED_ANIMATION_GAME_ACTION_NAME,
            {
                { TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_PARAM, TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_KILL },
                { TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_PARAM, TrapTriggeredAnimationGameAction::KILL_TRAP_TYPE_DEMON_TRAP }
            });
            
            activePlayerState.mBoardModifiers.mBoardModifierMask &= (~effects::board_modifier_masks::DEMON_KILL_NEXT);
            return;
        }
        
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE) != 0 &&
            cardData.mCardFamily == game_constants::DINOSAURS_FAMILY_NAME)
        {
            mGameActionEngine->AddGameAction(NEXT_DINO_DAMAGE_DOUBLING_GAME_ACTION_NAME, {});
            activePlayerState.mBoardModifiers.mBoardModifierMask &= (~effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE);
        }
        
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE) != 0 &&
            cardData.mCardFamily == game_constants::DINOSAURS_FAMILY_NAME)
        {
            mGameActionEngine->AddGameAction(HEAL_NEXT_DINO_DAMAGE_GAME_ACTION_NAME, {});
            activePlayerState.mBoardModifiers.mBoardModifierMask &= (~effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE);
        }
        
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::BOARD_SIDE_DEBUFF) != 0)
        {
            mGameActionEngine->AddGameAction(TRAP_TRIGGERED_ANIMATION_GAME_ACTION_NAME,
            {
                { TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_PARAM, TrapTriggeredAnimationGameAction::TRAP_TRIGGER_TYPE_DEBUFF }
            });
        }
        
        if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::DUPLICATE_NEXT_INSECT) != 0 &&
            cardData.mCardFamily == game_constants::INSECTS_FAMILY_NAME)
        {
            mGameActionEngine->AddGameAction(INSECT_DUPLICATION_GAME_ACTION_NAME, {});
            activePlayerState.mBoardModifiers.mBoardModifierMask &= (~effects::board_modifier_masks::DUPLICATE_NEXT_INSECT);
        }
        
    }
    
    if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::INSECT_VIRUS) != 0)
    {
        mGameActionEngine->AddGameAction(INSECT_VIRUS_GAME_ACTION_NAME, {});
    }
    
    if ((activePlayerState.mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST) != 0)
    {
        mGameActionEngine->AddGameAction(ZERO_COST_TIME_GAME_ACTION_NAME, {});
    }
    
    if (mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX)
    {
        bool shouldSendEndGameTutorialTrigger = true;
        if (!activePlayerState.mPlayerHeldCards.empty())
        {
            for (int i = 0; i < activePlayerState.mPlayerHeldCards.size(); ++i)
            {
                const auto& cardData = CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[i], game_constants::LOCAL_PLAYER_INDEX);
                if (mGameRuleEngine && mGameRuleEngine->CanCardBePlayed(&cardData, i, game_constants::LOCAL_PLAYER_INDEX))
                {
                    shouldSendEndGameTutorialTrigger = false;
                    break;
                }
            }
        }
        
        if (shouldSendEndGameTutorialTrigger)
        {
            mGameActionEngine->AddGameAction(END_TURN_TUTORIAL_GAME_ACTION_NAME);
        }
    }
}

///------------------------------------------------------------------------------------------------

void PlayCardGameAction::VInitAnimation()
{
    mPendingAnimations = 0;
    mHasFinalizedCardPlay = false;
    
    const auto lastPlayedCardIndex = std::stoi(mExtraActionParams.at(LAST_PLAYED_CARD_INDEX_PARAM));
    auto lastPlayedCardSoWrapper = mBattleSceneLogicManager->GetHeldCardSoWrappers()[mBoardState->GetActivePlayerIndex()].at(lastPlayedCardIndex);
    
    if (mAborted)
    {
        return;
    }

    if (DataRepository::GetInstance().GetNextBattleControlType() == BattleControlType::AI_TOP_ONLY && mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX)
    {
        AnimatedCardToBoard(lastPlayedCardSoWrapper);
    }
    else
    {
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        
        // Other cards make space for this card to be played
        for (auto i = 0; i < mBattleSceneLogicManager->GetHeldCardSoWrappers()[mBoardState->GetActivePlayerIndex()].size(); ++i)
        {
            if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
            {
                if (i == lastPlayedCardIndex)
                {
                    continue;
                }
            }
            
            auto& cardSoWrapper = mBattleSceneLogicManager->GetHeldCardSoWrappers()[mBoardState->GetActivePlayerIndex()][i];
            auto targetPosition = cardSoWrapper->mSceneObject->mPosition;
            targetPosition.y += mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? CARDS_MAKING_SPACE_Y_OFFSET : -CARDS_MAKING_SPACE_Y_OFFSET;
            
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, targetPosition, cardSoWrapper->mSceneObject->mScale, IN_GAME_PLAYED_CARD_ANIMATION_DURATION/2, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
            {
            });
        }
        
        auto targetPosition = lastPlayedCardSoWrapper->mSceneObject->mPosition;
        targetPosition.y += mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? -CARD_PLAY_PROTRUDED_Y_OFFSET : CARD_PLAY_PROTRUDED_Y_OFFSET;
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(lastPlayedCardSoWrapper->mSceneObject, targetPosition, lastPlayedCardSoWrapper->mSceneObject->mScale, IN_GAME_PLAYED_CARD_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
        {
            mPendingAnimations--;
            AnimatedCardToBoard(lastPlayedCardSoWrapper);
        });
        
        mPendingAnimations++;
    }
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(CARD_PLAY_SFX);
}

///------------------------------------------------------------------------------------------------

void PlayCardGameAction::AnimatedCardToBoard(std::shared_ptr<CardSoWrapper> lastPlayedCardSoWrapper)
{
    const auto lastPlayedCardIndex = std::stoi(mExtraActionParams.at(LAST_PLAYED_CARD_INDEX_PARAM));
    const auto boardCardIndex = static_cast<int>(mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1);
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    // For remote plays, the front face card also needs to be created
    if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX)
    {
        scene->RemoveSceneObject(lastPlayedCardSoWrapper->mSceneObject->mName);
        lastPlayedCardSoWrapper = card_utils::CreateCardSoWrapper
        (
            &lastPlayedCardSoWrapper->mCardData,
            lastPlayedCardSoWrapper->mSceneObject->mPosition,
            game_constants::TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX + std::to_string(mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1),
            CardOrientation::FRONT_FACE,
            card_utils::GetCardRarity(lastPlayedCardSoWrapper->mCardData.mCardId, mBoardState->GetActivePlayerIndex(), *mBoardState),
            false,
            true,
            true,
            (static_cast<int>(mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.size()) > boardCardIndex ? mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides.at(boardCardIndex) : CardStatOverrides()), // held card stat overrides have moved to board card stat overrides from the setstate above
            mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers,
            *sceneManager.FindScene(game_constants::BATTLE_SCENE)
        );
        events::EventSystem::GetInstance().DispatchEvent<events::HeldCardSwapEvent>(lastPlayedCardSoWrapper, lastPlayedCardIndex, true);
    }
    
    if (!mHasFinalizedCardPlay)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::LastCardPlayedFinalizedEvent>(lastPlayedCardIndex);
        mHasFinalizedCardPlay = true;
    }
    
    // Rename played card components
    lastPlayedCardSoWrapper->mSceneObject->mName = strutils::StringId((mBoardState->GetActivePlayerIndex() == 0 ? game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX) + std::to_string(mBoardState->GetActivePlayerState().mPlayerBoardCards.size() - 1));
    
    // Animate played card to board
    const auto& boardCards = mBoardState->GetActivePlayerState().mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetActivePlayerState().mBoardCardIndicesToDestroy;
    const auto nonDeadBoardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    
    auto targetPosition = card_utils::CalculateBoardCardPosition(nonDeadBoardCardCount - 1, nonDeadBoardCardCount, mBoardState->GetActivePlayerIndex() == 0);
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(lastPlayedCardSoWrapper->mSceneObject, targetPosition, lastPlayedCardSoWrapper->mSceneObject->mScale * game_constants::IN_GAME_PLAYED_CARD_SCALE_FACTOR, IN_GAME_PLAYED_CARD_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        mPendingAnimations--;
        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(CARD_CAMERA_SHAKE_DURATION, CARD_CAMERA_SHAKE_STRENGTH);
        
        events::EventSystem::GetInstance().DispatchEvent<events::WeightChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        
        card_utils::PlayCardPlaySfx(&lastPlayedCardSoWrapper->mCardData);
        
        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
        (
            CARD_PLAY_PARTICLE_NAME,
            glm::vec3(targetPosition.x, targetPosition.y, CARD_PLAY_PARTICLE_EMITTER_Z),
            *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)
        );
        
        lastPlayedCardSoWrapper->mSceneObject->mShaderBoolUniformValues[game_constants::IS_HELD_CARD_UNIFORM_NAME] = false;
        
        if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX && !lastPlayedCardSoWrapper->mCardData.IsSpell())
        {
            auto historyButtonPosition = scene->FindSceneObject(HISTORY_BUTTON_SCENE_OBJECT_NAME)->mPosition;
            historyButtonPosition.x *= game_constants::GAME_BOARD_GUI_DISTANCE_FACTOR;
            historyButtonPosition.x -= 0.003f;
            
            auto arrowOriginPosition = historyButtonPosition;
            arrowOriginPosition.y += 0.05f;
            
            events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_HISTORY_TUTORIAL, arrowOriginPosition, historyButtonPosition);
        }
        
        const auto& seenSpellCardIds = DataRepository::GetInstance().GetSeenOpponentSpellCardIds();
        if (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX && lastPlayedCardSoWrapper->mCardData.IsSpell() && std::find(seenSpellCardIds.begin(), seenSpellCardIds.end(), lastPlayedCardSoWrapper->mCardData.mCardId) == seenSpellCardIds.end())
        {
            DataRepository::GetInstance().SetNextUnseenSpellCardId(lastPlayedCardSoWrapper->mCardData.mCardId);
            events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(UNSEEN_SPELL_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
        }
    });
    mPendingAnimations++;
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult PlayCardGameAction::VUpdateAnimation(const float)
{
    return mPendingAnimations == 0 ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool PlayCardGameAction::VShouldBeSerialized() const
{
    return true;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& PlayCardGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
