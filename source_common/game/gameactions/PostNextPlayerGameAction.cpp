///------------------------------------------------------------------------------------------------
///  PostNextPlayerGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/11/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/utils/PlatformMacros.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/gameactions/CardBuffedDebuffedAnimationGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/PostNextPlayerGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("CardBuffedDebuffedAnimationGameAction");

static const float TURN_POINTER_ANIMATION_DURATION_SECS = 0.66f;
static const float DORMANT_CARDS_REEMERGE_ANIMATION_DURATION_SECS = 0.5f;
static const float CARD_SCALE_UP_FACTOR = 1.5f;

///------------------------------------------------------------------------------------------------

void PostNextPlayerGameAction::VSetNewGameState()
{
    std::vector<int> remainingBoardCards;
    std::vector<CardStatOverrides> remainingBoardCardStatOverrides;
    
    auto& boardCards = mBoardState->GetInactivePlayerState().mPlayerBoardCards;
    auto& boardCardIndicesToDestroy = mBoardState->GetInactivePlayerState().mBoardCardIndicesToDestroy;
    
    for (int i = static_cast<int>(boardCards.size()) - 1; i >= 0; --i)
    {
        if (boardCardIndicesToDestroy.count(i) == 0)
        {
            remainingBoardCards.insert(remainingBoardCards.begin(), boardCards[i]);
            
            if (static_cast<int>(mBoardState->GetInactivePlayerState().mPlayerBoardCardStatOverrides.size()) > i)
            {
                remainingBoardCardStatOverrides.insert(remainingBoardCardStatOverrides.begin(), mBoardState->GetInactivePlayerState().mPlayerBoardCardStatOverrides[i]);
            }
        }
        else
        {
            std::vector<std::string> idx = { std::to_string(i) };
            events::EventSystem::GetInstance().DispatchEvent<events::EndOfTurnCardDestructionEvent>(idx, true, mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
        }
    }
    
    auto& heldCards = mBoardState->GetInactivePlayerState().mPlayerHeldCards;
    
    for (int i = static_cast<int>(heldCards.size()) - 1; i >= 0; --i)
    {
        std::vector<std::string> idx = { std::to_string(i) };
        events::EventSystem::GetInstance().DispatchEvent<events::EndOfTurnCardDestructionEvent>(idx, false, mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
    }
    
    mBoardState->GetInactivePlayerState().mPlayerBoardCards = remainingBoardCards;
    mBoardState->GetInactivePlayerState().mPlayerBoardCardStatOverrides = remainingBoardCardStatOverrides;
    mBoardState->GetInactivePlayerState().mPlayerHeldCards.clear();
    mBoardState->GetInactivePlayerState().mPlayerHeldCardStatOverrides.clear();
    
    
    // Here we keep track of permanent board & card modifiers and save them
    // to be reintroduced after the original data is cleared a few lines later
    CardStatOverrides globalCardStatModifiersBuilder;
    effects::EffectBoardModifierMask boardModifierMaskBuilder = effects::board_modifier_masks::NONE;
    
    if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION) != 0)
    {
        globalCardStatModifiersBuilder[CardStatType::WEIGHT] = mBoardState->GetInactivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::WEIGHT);
        boardModifierMaskBuilder |= effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION;
    }
    
    if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST) != 0)
    {
        boardModifierMaskBuilder |= effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST;
    }
    
    if ((mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::INSECT_VIRUS) != 0)
    {
        boardModifierMaskBuilder |= effects::board_modifier_masks::INSECT_VIRUS;
    }
    
    bool clearedDebuff = !mBoardState->GetInactivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.empty();
    mBoardState->GetInactivePlayerState().mBoardModifiers.mGlobalCardStatModifiers = globalCardStatModifiersBuilder;
    mBoardState->GetInactivePlayerState().mBoardModifiers.mBoardModifierMask = boardModifierMaskBuilder;
    
    // For hero cards only
    if (mBoardState->GetInactivePlayerState().mHasHeroCard && clearedDebuff)
    {
        mGameActionEngine->AddGameAction(CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME,
        {
            { CardBuffedDebuffedAnimationGameAction::CARD_INDEX_PARAM, "0"},
            { CardBuffedDebuffedAnimationGameAction::PLAYER_INDEX_PARAM, std::to_string(game_constants::REMOTE_PLAYER_INDEX)},
            { CardBuffedDebuffedAnimationGameAction::IS_BOARD_CARD_PARAM, "true" },
            { CardBuffedDebuffedAnimationGameAction::SCALE_FACTOR_PARAM, std::to_string(CARD_SCALE_UP_FACTOR) }
        });
    }
    
    mBoardState->GetInactivePlayerState().mPlayedCardComboThisTurn = 0;
    if (mBoardState->GetInactivePlayerState().mZeroCostTime)
    {
        mBoardState->GetInactivePlayerState().mZeroCostTime = false;
        events::EventSystem::GetInstance().DispatchEvent<events::ZeroCostTimeEvent>(false, mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
    }
    
    mBoardState->GetInactivePlayerState().mBoardCardIndicesToDestroy.clear();
    mBoardState->GetInactivePlayerState().mHeldCardIndicesToDestroy.clear();
    mBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask &= ~(effects::board_modifier_masks::DOUBLE_POISON_ATTACKS);
    
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::BOARD_SIDE_DEBUFF);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::KILL_NEXT);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::DEMON_KILL_NEXT);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::SPELL_KILL_NEXT);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::DUPLICATE_NEXT_INSECT);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::DIG_NO_FAIL);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::RODENT_LIFESTEAL);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, true, effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE);
    events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, true, effects::board_modifier_masks::DOUBLE_POISON_ATTACKS);
    
    // Armor gain
    if (mBoardState->GetActivePlayerState().mPlayerArmorRecharge > 0)
    {
        mBoardState->GetActivePlayerState().mPlayerCurrentArmor += mBoardState->GetActivePlayerState().mPlayerArmorRecharge;
        events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, mBoardState->GetActivePlayerState().mPlayerCurrentArmor);
    }
    
    events::EventSystem::GetInstance().DispatchEvent<events::WeightChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
}

///------------------------------------------------------------------------------------------------

void PostNextPlayerGameAction::VInitAnimation()
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    // Any surviving board cards for player whose turn has ended need to be repositioned at this point
    for (auto i = 0U; i < mBoardState->GetInactivePlayerState().mPlayerBoardCards.size(); ++i)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::ForceSendCardBackToPositionEvent>(i, true, mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
    }
    
    // .. and any surviving non-hero board cards from previous turn of the active player need to re-emerge out again
    for (auto i = 0U; i < mBoardState->GetActivePlayerState().mPlayerBoardCards.size(); ++i)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::CardBuffedDebuffedEvent>(i, true, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(mBoardState->GetActivePlayerIndex()).at(i);
        
        if (!mBoardState->GetActivePlayerState().mHasHeroCard || i > 0)
        {
            cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::DORMANT_CARD_VALUE_UNIFORM_NAME] = 1.0f;
            animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::DORMANT_CARD_VALUE_UNIFORM_NAME], 0.0f, DORMANT_CARDS_REEMERGE_ANIMATION_DURATION_SECS),[=](){});
        }
    }
    
    if ((mBoardState->GetTurnCounter() != 0 && mBoardState->GetTurnCounter() != 1) || mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasHeroCard == false)
    {
        mPendingAnimations = 1;
        
        auto turnPointerSo = scene->FindSceneObject(game_constants::TURN_POINTER_SCENE_OBJECT_NAME);
        bool localPlayerActive = mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX;
        
        auto targetRotation = glm::vec3(0.0f, 0.0f, turnPointerSo->mRotation.z + (localPlayerActive ? math::PI/2 : -math::PI/2));
        animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(turnPointerSo, targetRotation, TURN_POINTER_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=]()
        {
            mPendingAnimations--;
            
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
            auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
            bool localPlayerActive = mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX;
            if (localPlayerActive)
            {
                auto turnPointerHighlighterSo = scene->FindSceneObject(game_constants::TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME);
                animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(turnPointerHighlighterSo, 1.0f, TURN_POINTER_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), []()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::LocalPlayerTurnStarted>();
                });
            }
        });
    }
    else
    {
        mPendingAnimations = 0;
        auto turnPointerHighlighterSo = scene->FindSceneObject(game_constants::TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME);
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(turnPointerHighlighterSo, 1.0f, TURN_POINTER_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), []()
        {
            events::EventSystem::GetInstance().DispatchEvent<events::LocalPlayerTurnStarted>();
        });
    }
}
    

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult PostNextPlayerGameAction::VUpdateAnimation(const float)
{
    return mPendingAnimations == 0 ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool PostNextPlayerGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& PostNextPlayerGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
