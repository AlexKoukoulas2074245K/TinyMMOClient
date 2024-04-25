///------------------------------------------------------------------------------------------------
///  ZeroCostTimeGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 23/02/2024
///------------------------------------------------------------------------------------------------

#include <game/gameactions/ZeroCostTimeGameAction.h>
#include <game/BoardState.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>

///------------------------------------------------------------------------------------------------

void ZeroCostTimeGameAction::VSetNewGameState()
{
    auto& activePlayerState = mBoardState->GetActivePlayerState();
    activePlayerState.mPlayedCardComboThisTurn++;
    
    if (activePlayerState.mPlayedCardComboThisTurn == 2)
    {
        activePlayerState.mZeroCostTime = true;
        if (mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size() < mBoardState->GetActivePlayerState().mPlayerHeldCards.size())
        {
            mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.resize(mBoardState->GetActivePlayerState().mPlayerHeldCards.size());
        }
            
        for (auto i = 0; i < mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size(); ++i)
        {
            auto& heldCardStatOverride = mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[i];
            if (!heldCardStatOverride.count(CardStatType::WEIGHT))
            {
                heldCardStatOverride[CardStatType::WEIGHT] = CardDataRepository::GetInstance().GetCardData(mBoardState->GetActivePlayerState().mPlayerHeldCards[i], mBoardState->GetActivePlayerIndex()).mCardWeight;
            }
            heldCardStatOverride[CardStatType::WEIGHT] -= game_constants::ZERO_COST_TIME_WEIGHT_VALUE;
        }
        
        events::EventSystem::GetInstance().DispatchEvent<events::ZeroCostTimeEvent>(true, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        
        for (auto i = 0; i < mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size(); ++i)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::ForceSendCardBackToPositionEvent>(i, false, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        }
    }
    else if (activePlayerState.mPlayedCardComboThisTurn == 3)
    {
        activePlayerState.mPlayedCardComboThisTurn = 0;
        activePlayerState.mZeroCostTime = false;
        
        if (mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size() < mBoardState->GetActivePlayerState().mPlayerHeldCards.size())
        {
            mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.resize(mBoardState->GetActivePlayerState().mPlayerHeldCards.size());
        }
            
        for (auto i = 0; i < mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides.size(); ++i)
        {
            auto& heldCardStatOverride = mBoardState->GetActivePlayerState().mPlayerHeldCardStatOverrides[i];
            heldCardStatOverride[CardStatType::WEIGHT] += game_constants::ZERO_COST_TIME_WEIGHT_VALUE;
        }
        
        events::EventSystem::GetInstance().DispatchEvent<events::ZeroCostTimeEvent>(false, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        
        for (auto i = 0; i < mBoardState->GetActivePlayerState().mPlayerHeldCards.size(); ++i)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::ForceSendCardBackToPositionEvent>(i, false, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX);
        }
    }
}

///------------------------------------------------------------------------------------------------

void ZeroCostTimeGameAction::VInitAnimation()
{
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult ZeroCostTimeGameAction::VUpdateAnimation(const float)
{
    return ActionAnimationUpdateResult::FINISHED;
}

///------------------------------------------------------------------------------------------------

bool ZeroCostTimeGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& ZeroCostTimeGameAction::VGetRequiredExtraParamNames() const
{
    static std::vector<std::string> v;
    return v;
}

///------------------------------------------------------------------------------------------------
