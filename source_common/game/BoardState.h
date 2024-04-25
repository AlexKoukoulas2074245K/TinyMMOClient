///------------------------------------------------------------------------------------------------
///  BoardState.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef BoardState_h
#define BoardState_h

///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardEffectComponents.h>
#include <unordered_map>
#include <vector>


///------------------------------------------------------------------------------------------------

struct BoardModifiers
{
    effects::EffectBoardModifierMask mBoardModifierMask = effects::board_modifier_masks::NONE;
    CardStatOverrides mGlobalCardStatModifiers;
};

///------------------------------------------------------------------------------------------------

struct PlayerState
{
    std::vector<int> mPlayerDeckCards;
    std::vector<int> mPlayerHeldCards;
    std::vector<int> mPlayerBoardCards;
    std::vector<int> mPlayerInitialDeckCards;
    std::vector<int> mGoldenCardIds;
    std::unordered_set<int> mHeldCardIndicesToDestroy;
    std::unordered_set<int> mBoardCardIndicesToDestroy;
    std::vector<CardStatOverrides> mPlayerBoardCardStatOverrides;
    std::vector<CardStatOverrides> mPlayerHeldCardStatOverrides;
    BoardModifiers mBoardModifiers;
    int mPlayerHealth = 30;
    int mPlayerCurrentArmor = 0;
    int mPlayerArmorRecharge = 0;
    int mPlayerPoisonStack = 0;
    int mPlayerTotalWeightAmmo = 0;
    int mPlayerCurrentWeightAmmo = 0;
    int mPlayerWeightAmmoLimit = 0;
    int mPlayedCardComboThisTurn = 0;
    int mCardsDrawnThisTurn = 0;
    bool mZeroCostTime = false;
    bool mHasHeroCard = false;
    bool mHasResurrectionActive = false;
};

///------------------------------------------------------------------------------------------------

class BoardState
{
public:
    const std::vector<PlayerState>& GetPlayerStates() const { return mPlayerStates; }
    std::vector<PlayerState>& GetPlayerStates() { return mPlayerStates; }
    const PlayerState& GetActivePlayerState() const { return mActivePlayerIndex == -1 ? mPlayerStates.at(1) : mPlayerStates.at(mActivePlayerIndex); }
    PlayerState& GetActivePlayerState() { return mActivePlayerIndex == -1 ? mPlayerStates[1] : mPlayerStates[mActivePlayerIndex]; }
    const PlayerState& GetInactivePlayerState() const { return mActivePlayerIndex == -1 ? mPlayerStates.at(0) : mPlayerStates.at((mActivePlayerIndex + 1) % GetPlayerCount()); }
    PlayerState& GetInactivePlayerState() { return mActivePlayerIndex == -1 ? mPlayerStates[0] : mPlayerStates[(mActivePlayerIndex + 1) % GetPlayerCount()]; }
    int GetActivePlayerIndex() const { return mActivePlayerIndex; }
    int& GetActivePlayerIndex() { return mActivePlayerIndex; }
    int& GetTurnCounter() { return mTurnCounter; }
    int GetTurnCounter() const { return mTurnCounter; }
    size_t GetPlayerCount() const { return mPlayerStates.size(); }
    
private:
    std::vector<PlayerState> mPlayerStates;
    int mActivePlayerIndex = -1;
    int mTurnCounter = -1;
};

///------------------------------------------------------------------------------------------------

#endif /* BoardState_h */
