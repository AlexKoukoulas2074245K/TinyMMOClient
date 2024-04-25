///------------------------------------------------------------------------------------------------
///  PlayerActionGenerationEngine.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/BoardState.h>
#include <game/CardEffectComponents.h>
#include <game/Cards.h>
#include <game/GameConstants.h>
#include <game/GameRuleEngine.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/PlayCardGameAction.h>
#include <game/gameactions/PlayerActionGenerationEngine.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId PLAY_CARD_GAME_ACTION_NAME = strutils::StringId("PlayCardGameAction");
static const strutils::StringId NEXT_PLAYER_GAME_ACTION_NAME = strutils::StringId("NextPlayerGameAction");

///------------------------------------------------------------------------------------------------

PlayerActionGenerationEngine::PlayerActionGenerationEngine(GameRuleEngine* gameRuleEngine, GameActionEngine* gameActionEngine, ActionGenerationType actionGenerationType)
    : mGameRuleEngine(gameRuleEngine)
    , mGameActionEngine(gameActionEngine)
    , mActionGenerationType(actionGenerationType)
{
    
}

///------------------------------------------------------------------------------------------------

void PlayerActionGenerationEngine::DecideAndPushNextActions(BoardState* currentBoardState)
{
    BoardState boardStateCopy = *currentBoardState;
    
    if (boardStateCopy.GetTurnCounter() == 0 && boardStateCopy.GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasHeroCard)
    {
        mGameActionEngine->AddGameAction(NEXT_PLAYER_GAME_ACTION_NAME);
        return;
    }
    
    auto& currentHeldCards = boardStateCopy.GetActivePlayerState().mPlayerHeldCards;
    auto& currentBoardCards = boardStateCopy.GetActivePlayerState().mPlayerBoardCards;
    
    auto currentHeldCardsCopySorted = currentHeldCards;
    
    // Sort all held cards by descending damage
    const auto& cardRepository = CardDataRepository::GetInstance();
    std::sort(currentHeldCardsCopySorted.begin(), currentHeldCardsCopySorted.end(), [&](const int& lhs, const int& rhs)
    {
        const auto& cardDataLhs = cardRepository.GetCardData(lhs, boardStateCopy.GetActivePlayerIndex());
        const auto& cardDataRhs = cardRepository.GetCardData(rhs, boardStateCopy.GetActivePlayerIndex());
        
        bool isLhsCardHighPriority = IsCardHighPriority(cardDataLhs, &boardStateCopy);
        bool isRhsCardHighPriority = IsCardHighPriority(cardDataRhs, &boardStateCopy);
        
        if (mActionGenerationType == ActionGenerationType::OPTIMISED)
        {
            isLhsCardHighPriority &= (cardDataLhs.mCardId != mLastPlayedCard.mCardId || boardStateCopy.GetActivePlayerIndex() != mLastPlayedCard.mPlayerIndex);
            isRhsCardHighPriority &= (cardDataRhs.mCardId != mLastPlayedCard.mCardId || boardStateCopy.GetActivePlayerIndex() != mLastPlayedCard.mPlayerIndex);
        }
        
        if (isLhsCardHighPriority && isRhsCardHighPriority)
        {
            return lhs < rhs;
        }
        else if (isLhsCardHighPriority && !isRhsCardHighPriority)
        {
            return true;
        }
        else if (!isLhsCardHighPriority && isRhsCardHighPriority)
        {
            return false;
        }
        
        return cardDataLhs.mCardDamage >
               cardDataRhs.mCardDamage;
    });

    // Play every card possible (from highest weights to lowest)
    bool shouldWaitForFurtherActions = false;
    for (auto iter = currentHeldCardsCopySorted.cbegin(); iter != currentHeldCardsCopySorted.cend();)
    {
        const auto cardData = cardRepository.GetCardData(*iter, boardStateCopy.GetActivePlayerIndex());
        
        // Find index of card in original vector
        auto originalHeldCardIter = std::find(currentHeldCards.cbegin(), currentHeldCards.cend(), cardData.mCardId);
        const auto cardIndex = originalHeldCardIter - currentHeldCards.cbegin();
        
        if (mGameRuleEngine->CanCardBePlayed(&cardData, cardIndex, boardStateCopy.GetActivePlayerIndex(), &boardStateCopy))
        {
            assert(originalHeldCardIter != currentHeldCards.cend());
            
            mGameActionEngine->AddGameAction(PLAY_CARD_GAME_ACTION_NAME, {{PlayCardGameAction::LAST_PLAYED_CARD_INDEX_PARAM, std::to_string(cardIndex)}});
            mLastPlayedCard.mCardId = *iter;
            mLastPlayedCard.mPlayerIndex = boardStateCopy.GetActivePlayerIndex();
            
            // Simulate card play effects on copy of board state
            auto cardWeight = cardData.mCardWeight;
            const auto& cardStatOverrides = boardStateCopy.GetActivePlayerState().mPlayerHeldCardStatOverrides;
            if (static_cast<int>(cardStatOverrides.size()) > cardIndex)
            {
                cardWeight = math::Max(0, cardStatOverrides[cardIndex].count(CardStatType::WEIGHT) ? cardStatOverrides[cardIndex].at(CardStatType::WEIGHT) : cardData.mCardWeight);
            }
            if (!cardData.IsSpell() && boardStateCopy.GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.count(CardStatType::WEIGHT))
            {
                cardWeight = math::Max(0, cardWeight + boardStateCopy.GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::WEIGHT));
            }
            
            boardStateCopy.GetActivePlayerState().mPlayerCurrentWeightAmmo -= cardWeight;
            currentBoardCards.push_back(cardData.mCardId);
            
            currentHeldCards.erase(originalHeldCardIter);
            iter = currentHeldCardsCopySorted.erase(iter);
            
            shouldWaitForFurtherActions = IsCardHighPriority(cardData, &boardStateCopy) || cardData.mIsSingleUse;
            if (shouldWaitForFurtherActions)
            {
                break;
            }
        }
        else
        {
            iter++;
        }
    }
    
    if (!shouldWaitForFurtherActions)
    {
        mGameActionEngine->AddGameAction(NEXT_PLAYER_GAME_ACTION_NAME);
    }
}

///------------------------------------------------------------------------------------------------

bool PlayerActionGenerationEngine::IsCardHighPriority(const CardData& cardData, BoardState* currentBoardState) const
{
    if (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DRAW) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_FAMILY) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_RANDOM_HAND_BUFF_ATTACK)
    ) return true;
        
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_TRIPPLES_LOWEST_ATTACK_ON_HAND)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_SWAP_MIN_MAX_DAMAGE)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_CLEAR_EFFECTS) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DUPLICATE_INSECT)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_SPELL_KILL)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_ADD_POISON_STACKS)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DOUBLE_NEXT_DINO_DAMAGE)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_HEAL_NEXT_DINO_DAMAGE)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_PERMANENT_CONTINUAL_WEIGHT_REDUCTION) &&
        ((currentBoardState->GetActivePlayerState().mBoardModifiers.mBoardModifierMask & effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION) == 0 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_CARD_TOKEN)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_ENEMY_BOARD_DEBUFF) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DOUBLE_POISON_ATTACKS)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DIG_NO_FAIL)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DRAW_RANDOM_SPELL)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_ARMOR)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DEMON_KILL) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_TOXIC_BOMB)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_INSECT_MEGASWARM)
    ) return true;
        
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_METEOR)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_HOUND_SUMMONING) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_DEMON_PUNCH) &&
        (math::RandomInt(0, 1) == 1 || mActionGenerationType != ActionGenerationType::OPTIMISED)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_INSECT_VIRUS)
    ) return true;
    
    else if
    (
        cardData.IsSpell() &&
        strutils::StringContains(cardData.mCardEffect, effects::EFFECT_COMPONENT_RODENT_LIFESTEAL_ON_ATTACKS)
    ) return true;
    
    return false;
}

///------------------------------------------------------------------------------------------------
