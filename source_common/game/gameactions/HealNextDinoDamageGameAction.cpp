///------------------------------------------------------------------------------------------------
///  HealNextDinoDamageGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 15/02/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/HealNextDinoDamageGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
};

///------------------------------------------------------------------------------------------------

void HealNextDinoDamageGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    assert(!activePlayerState.mPlayerBoardCards.empty());
    const auto& playerBoardCards = activePlayerState.mPlayerBoardCards;
    
    auto dinoDamage = CardDataRepository::GetInstance().GetCardData(playerBoardCards.back(), mBoardState->GetActivePlayerIndex()).mCardDamage;
    auto& cardStatOverrides = mBoardState->GetActivePlayerState().mPlayerBoardCardStatOverrides;
    
    if (!cardStatOverrides.empty() && static_cast<int>(cardStatOverrides.size()) > playerBoardCards.size() - 1 && cardStatOverrides[playerBoardCards.size() - 1].count(CardStatType::DAMAGE))
    {
        dinoDamage = math::Max(0, cardStatOverrides[playerBoardCards.size() - 1].at(CardStatType::DAMAGE));
    }
    
    if (mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.count(CardStatType::DAMAGE))
    {
        dinoDamage = math::Max(0, dinoDamage + mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::DAMAGE));
    }
    
    if (dinoDamage > 0)
    {
        auto oldHealthValue = mBoardState->GetActivePlayerState().mPlayerHealth;
        
        if (mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX)
        {
            mBoardState->GetActivePlayerState().mPlayerHealth = math::Min(mBoardState->GetActivePlayerState().mPlayerHealth + dinoDamage, DataRepository::GetInstance().GetStoryMaxHealth());
        }
        else
        {
            mBoardState->GetActivePlayerState().mPlayerHealth += dinoDamage;
        }
        
        if (oldHealthValue != mBoardState->GetActivePlayerState().mPlayerHealth)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        }
        
        events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX, false, effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE);
    }
}

///------------------------------------------------------------------------------------------------

void HealNextDinoDamageGameAction::VInitAnimation()
{
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult HealNextDinoDamageGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool HealNextDinoDamageGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& HealNextDinoDamageGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
