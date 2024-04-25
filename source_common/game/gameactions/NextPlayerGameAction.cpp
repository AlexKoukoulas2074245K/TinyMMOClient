///------------------------------------------------------------------------------------------------
///  NextPlayerGameAction.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <game/AchievementManager.h>
#include <game/ArtifactProductIds.h>
#include <game/events/EventSystem.h>
#include <game/DataRepository.h>
#include <game/GameConstants.h>
#include <game/gameactions/CardAttackGameAction.h>
#include <game/gameactions/CardDestructionGameAction.h>
#include <game/gameactions/CardHistoryEntryAdditionGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/NextPlayerGameAction.h>
#include <game/gameactions/PoisonStackApplicationGameAction.h>
#include <numeric>
#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

///------------------------------------------------------------------------------------------------


static const strutils::StringId CARD_ATTACK_GAME_ACTION_NAME = strutils::StringId("CardAttackGameAction");
static const strutils::StringId DRAW_CARD_GAME_ACTION_NAME = strutils::StringId("DrawCardGameAction");
static const strutils::StringId POST_NEXT_PLAYER_GAME_ACTION_NAME = strutils::StringId("PostNextPlayerGameAction");
static const strutils::StringId CARD_DESTRUCTION_GAME_ACTION_NAME = strutils::StringId("CardDestructionGameAction");
static const strutils::StringId CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME = strutils::StringId("CardHistoryEntryAdditionGameAction");
static const strutils::StringId POISON_STACK_APPLICATION_GAME_ACTION_NAME = strutils::StringId("PoisonStackApplicationGameAction");
static const strutils::StringId TUTORIAL_HOW_TO_PLAY_A_CARD_GAME_ACTION_NAME = strutils::StringId("HowToPlayACardTutorialGameAction");

///------------------------------------------------------------------------------------------------

void NextPlayerGameAction::VSetNewGameState()
{
    int& activePlayerIndex = mBoardState->GetActivePlayerIndex();
    const auto previousPlayerIndex = activePlayerIndex;
    activePlayerIndex = (activePlayerIndex + 1) % mBoardState->GetPlayerCount();
    
    mBoardState->GetTurnCounter()++;
    
    auto& targetPlayerState = mBoardState->GetPlayerStates()[mBoardState->GetTurnCounter() % mBoardState->GetPlayerCount()];
    targetPlayerState.mPlayerTotalWeightAmmo = math::Min(targetPlayerState.mPlayerWeightAmmoLimit, targetPlayerState.mPlayerTotalWeightAmmo + 1);
    targetPlayerState.mPlayerCurrentWeightAmmo = targetPlayerState.mPlayerTotalWeightAmmo;
    
    if (mBoardState->GetTurnCounter() == 1 && targetPlayerState.mPlayerCurrentWeightAmmo == 12)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::REACH_12_STARTING_WEIGHT);
    }
    
    // Potentially generate card attack/destruction/poison stacks for player whose turn was just ended
    if (previousPlayerIndex != -1)
    {
        // First story opponent turn is skipped
        if (mBoardState->GetTurnCounter() != 1 || mBoardState->GetPlayerStates()[previousPlayerIndex].mHasHeroCard == false)
        {
            auto& boardCards = mBoardState->GetPlayerStates()[previousPlayerIndex].mPlayerBoardCards;
            for (size_t i = 0; i < boardCards.size(); ++i)
            {
                mGameActionEngine->AddGameAction(CARD_ATTACK_GAME_ACTION_NAME,
                {
                    { CardAttackGameAction::PLAYER_INDEX_PARAM, std::to_string(previousPlayerIndex) },
                    { CardAttackGameAction::CARD_INDEX_PARAM, std::to_string(i) }
                });
            }
        }
        
        // Destroy all held cards
        auto& playerHeldCards = mBoardState->GetPlayerStates()[previousPlayerIndex].mPlayerHeldCards;
        if (!playerHeldCards.empty())
        {
            std::vector<int> cardIndices(playerHeldCards.size());
            std::iota(cardIndices.begin(), cardIndices.end(), 0);
            
            mGameActionEngine->AddGameAction(CARD_DESTRUCTION_GAME_ACTION_NAME,
            {
                { CardDestructionGameAction::CARD_INDICES_PARAM, strutils::VecToString(cardIndices)},
                { CardDestructionGameAction::PLAYER_INDEX_PARAM, std::to_string(previousPlayerIndex)},
                { CardDestructionGameAction::IS_BOARD_CARD_PARAM, "false"},
                { CardDestructionGameAction::IS_TRAP_TRIGGER_PARAM, "false"}
            });
        }
    }
    
    mGameActionEngine->AddGameAction(CARD_HISTORY_ENTRY_ADDITION_GAME_ACTION_NAME,
    {
        { CardHistoryEntryAdditionGameAction::PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex()) },
        { CardHistoryEntryAdditionGameAction::CARD_INDEX_PARAM, "0" },
        { CardHistoryEntryAdditionGameAction::ENTRY_TYPE_TEXTURE_FILE_NAME_PARAM, "" },
        { CardHistoryEntryAdditionGameAction::IS_TURN_COUNTER_PARAM, "true"}
    });
    
    if (mBoardState->GetTurnCounter() != 0 || mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasHeroCard == false)
    {
        mBoardState->GetActivePlayerState().mCardsDrawnThisTurn = 0;
        
        mGameActionEngine->AddGameAction(POISON_STACK_APPLICATION_GAME_ACTION_NAME, {});
        mGameActionEngine->AddGameAction(POST_NEXT_PLAYER_GAME_ACTION_NAME);
        
        // Apply continual weight reduction effects
        if ((mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION) != 0)
        {
            mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers[CardStatType::WEIGHT]--;
        }
        
        // Both players get 4 cards
        mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
        mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
        mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
        mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
        
        // Bot player gets + 1 card
        if (previousPlayerIndex == 0)
        {
            mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
            
            for (int i = 0; i < DataRepository::GetInstance().GetStoryArtifactCount(artifacts::SLEAZY_SLEEVES); ++i)
            {
                mGameActionEngine->AddGameAction(DRAW_CARD_GAME_ACTION_NAME);
            }
            
            mGameActionEngine->AddGameAction(TUTORIAL_HOW_TO_PLAY_A_CARD_GAME_ACTION_NAME);
        }
    }
}

///------------------------------------------------------------------------------------------------

void NextPlayerGameAction::VInitAnimation()
{
    if ((mBoardState->GetTurnCounter() != 0 && mBoardState->GetTurnCounter() != 1) || mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasHeroCard == false)
    {
        mPendingAnimations = 1;
        
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
        auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
        auto turnPointerSo = scene->FindSceneObject(game_constants::TURN_POINTER_SCENE_OBJECT_NAME);
        bool localPlayerActive = mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX;
        
        auto targetRotation = glm::vec3(0.0f, 0.0f, turnPointerSo->mRotation.z + (localPlayerActive ? math::PI/2 : -math::PI/2));
        animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>
        (
            turnPointerSo,
            targetRotation,
            game_constants::TURN_POINTER_ANIMATION_DURATION_SECS,
            animation_flags::NONE, 0.0f,
            math::ElasticFunction,
            math::TweeningMode::EASE_IN
        ), [=]()
        {
            mPendingAnimations--;
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
            auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
            
            auto turnPointerHighlighterSo = scene->FindSceneObject(game_constants::TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME);
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(turnPointerHighlighterSo, 0.0f, game_constants::TURN_POINTER_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [](){});
        });
    }
    else
    {
        mPendingAnimations = 0;
    }
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult NextPlayerGameAction::VUpdateAnimation(const float)
{
    return mPendingAnimations == 0 ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool NextPlayerGameAction::VShouldBeSerialized() const
{
    return true;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& NextPlayerGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
