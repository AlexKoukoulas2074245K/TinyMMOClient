///------------------------------------------------------------------------------------------------
///  GameRuleEngine.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 26/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/BoardState.h>
#include <game/Cards.h>
#include <game/GameRuleEngine.h>
#include <game/GameConstants.h>
#include <game/CardEffectComponents.h>

///------------------------------------------------------------------------------------------------

GameRuleEngine::GameRuleEngine(BoardState* boardState)
    : mBoardState(boardState)
{
}

///------------------------------------------------------------------------------------------------

bool GameRuleEngine::CanCardBePlayed(const CardData* cardData, const size_t cardIndex, const size_t forPlayerIndex, BoardState* customBoardStateOverride /* = nullptr */) const
{
    auto* boardStateToUse = customBoardStateOverride ? customBoardStateOverride : mBoardState;
    auto& activePlayerState = boardStateToUse->GetPlayerStates()[forPlayerIndex];
    
    auto cardWeight = cardData->mCardWeight;
    const auto& cardStatOverrides = activePlayerState.mPlayerHeldCardStatOverrides;
    
    if (cardStatOverrides.size() > cardIndex)
    {
        cardWeight = math::Max(0, cardStatOverrides[cardIndex].count(CardStatType::WEIGHT) ? cardStatOverrides[cardIndex].at(CardStatType::WEIGHT) : cardData->mCardWeight);
    }
    
    if (!cardData->IsSpell() && activePlayerState.mBoardModifiers.mGlobalCardStatModifiers.count(CardStatType::WEIGHT))
    {
        cardWeight = math::Max(0, cardWeight + activePlayerState.mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::WEIGHT));
    }
    
    if (cardData->mCardEffect == effects::EFFECT_COMPONENT_INSECT_MEGASWARM && activePlayerState.mPlayerBoardCards.size() > 1)
    {
        return false;
    }
    
    if (strutils::StringContains(cardData->mCardEffect, effects::EFFECT_COMPONENT_HOUND_SUMMONING))
    {
        auto effectSplitBySpace = strutils::StringSplit(cardData->mCardEffect, ' ');
        auto summonCount = effectSplitBySpace[0] == effects::EFFECT_COMPONENT_HOUND_SUMMONING ? std::stoi(effectSplitBySpace[1]) : std::stoi(effectSplitBySpace[0]);
        
        if (activePlayerState.mPlayerBoardCards.size() + summonCount > game_constants::MAX_BOARD_CARDS)
        {
            return false;
        }
    }
    
    if (strutils::StringContains(cardData->mCardEffect, effects::EFFECT_COMPONENT_METEOR))
    {
        if (std::find_if(activePlayerState.mPlayerHeldCards.begin(), activePlayerState.mPlayerHeldCards.end(), [&](const int cardId)
        {
            const auto& cardData = CardDataRepository::GetInstance().GetCardData(cardId, mBoardState->GetActivePlayerIndex());
            return !cardData.IsSpell() && cardData.mCardFamily == game_constants::DINOSAURS_FAMILY_NAME;
        }) == activePlayerState.mPlayerHeldCards.end())
        {
            return false;
        }
    }
    
    if (strutils::StringContains(cardData->mCardEffect, effects::EFFECT_COMPONENT_SWAP_MIN_MAX_DAMAGE))
    {
        auto applicableCards = 0;
        for (auto i = 0; i < activePlayerState.mPlayerHeldCards.size(); ++i)
        {
            const auto& cardData = CardDataRepository::GetInstance().GetCardData(activePlayerState.mPlayerHeldCards[i], mBoardState->GetActivePlayerIndex());
            if (cardData.mCardFamily == game_constants::DINOSAURS_FAMILY_NAME && !cardData.IsSpell())
            {
                applicableCards++;
            }
        }
        
        if (applicableCards < 2)
        {
            return false;
        }
    }
    
    return (activePlayerState.mPlayerCurrentWeightAmmo >= cardWeight || activePlayerState.mZeroCostTime) && activePlayerState.mPlayerBoardCards.size() < game_constants::MAX_BOARD_CARDS;
}

///------------------------------------------------------------------------------------------------
