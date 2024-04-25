///------------------------------------------------------------------------------------------------
///  Events.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/11/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef Events_h
#define Events_h

///------------------------------------------------------------------------------------------------

#include <engine/scene/SceneObject.h>
#include <game/CardEffectComponents.h>
#include <game/Cards.h>
#include <game/GameSceneTransitionTypes.h>

///------------------------------------------------------------------------------------------------

namespace events
{

///------------------------------------------------------------------------------------------------

class ApplicationMovedToBackgroundEvent final
{
};

///------------------------------------------------------------------------------------------------

class WindowResizeEvent final
{
};

///------------------------------------------------------------------------------------------------

class LocalPlayerTurnStarted final
{
};

///------------------------------------------------------------------------------------------------

class EmptyDeckCardTokenPlayedEvent final
{
};


///------------------------------------------------------------------------------------------------

class TriggerRequestReviewEvent final
{
};

///------------------------------------------------------------------------------------------------

class SendPlayMessageEvent final
{
};

///------------------------------------------------------------------------------------------------

class ProductPurchaseEndedEvent final
{
public:
    ProductPurchaseEndedEvent(const bool wasSuccessful)
        : mWasSuccessful(wasSuccessful)
    {
    }
    
    const bool mWasSuccessful;
};

///------------------------------------------------------------------------------------------------

class EndOfTurnCardDestructionEvent final
{
public:
    EndOfTurnCardDestructionEvent(const std::vector<std::string> cardIndices, const bool isBoardCard, const bool forRemotePlayer)
        : mCardIndices(cardIndices)
        , mIsBoardCard(isBoardCard)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const std::vector<std::string> mCardIndices;
    const bool mIsBoardCard;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class ImmediateCardDestructionWithRepositionEvent final
{
public:
    ImmediateCardDestructionWithRepositionEvent(const int cardIndex, const bool isBoardCard, const bool forRemotePlayer)
        : mCardIndex(cardIndex)
        , mIsBoardCard(isBoardCard)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const int mCardIndex;
    const bool mIsBoardCard;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class SingleUseHeldCardCopyDestructionWithRepositionEvent final
{
public:
    SingleUseHeldCardCopyDestructionWithRepositionEvent(const std::vector<std::string>& heldCardIndicesToDestroy, const bool forRemotePlayer)
        : mHeldCardIndicesToDestroy(heldCardIndicesToDestroy)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const std::vector<std::string> mHeldCardIndicesToDestroy;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class CardCreationEvent final
{
public:
    CardCreationEvent(std::shared_ptr<CardSoWrapper> cardSoWrapper, const bool forRemotePlayer)
        : mCardSoWrapper(cardSoWrapper)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const std::shared_ptr<CardSoWrapper> mCardSoWrapper;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class CardBuffedDebuffedEvent final
{
public:
    CardBuffedDebuffedEvent(const int cardIndex, const bool boardCard, const bool forRemotePlayer)
        : mCardIndex(cardIndex)
        , mBoardCard(boardCard)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const int mCardIndex;
    const bool mBoardCard;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class ZeroCostTimeEvent final
{
public:
    ZeroCostTimeEvent(const bool zeroCostTimeEnabled, const bool forRemotePlayer)
        : mZeroCostTimeEnabled(zeroCostTimeEnabled)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const bool mZeroCostTimeEnabled;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class FlawlessVictoryTriggerEvent final
{
};

///------------------------------------------------------------------------------------------------

class ForceSendCardBackToPositionEvent final
{
public:
    ForceSendCardBackToPositionEvent(const int cardIndex, const bool boardCard, const bool forRemotePlayer)
        : mCardIdex(cardIndex)
        , mBoardCard(boardCard)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const int mCardIdex;
    const bool mBoardCard;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class BoardSideCardEffectTriggeredEvent final
{
public:
    BoardSideCardEffectTriggeredEvent(const bool forRemotePlayer, const effects::EffectBoardModifierMask effectBoardModifierMask)
        : mForRemotePlayer(forRemotePlayer)
        , mEffectBoardModifierMask(effectBoardModifierMask)
    {
    }
    
    const bool mForRemotePlayer;
    const effects::EffectBoardModifierMask mEffectBoardModifierMask;
};

///------------------------------------------------------------------------------------------------

class BoardSideCardEffectEndedEvent final
{
public:
    BoardSideCardEffectEndedEvent(const bool forRemotePlayer, const bool massClear, const effects::EffectBoardModifierMask effectBoardModifierMask)
        : mForRemotePlayer(forRemotePlayer)
        , mMassClear(massClear)
        , mEffectBoardModifierMask(effectBoardModifierMask)
    {
    }
    
    const bool mForRemotePlayer;
    const bool mMassClear;
    const effects::EffectBoardModifierMask mEffectBoardModifierMask;
};

///------------------------------------------------------------------------------------------------

class HeldCardSwapEvent final
{
public:
    HeldCardSwapEvent(const std::shared_ptr<CardSoWrapper> cardSoWrapper, const int cardIndex, const bool forRemotePlayer)
        : mCardSoWrapper(cardSoWrapper)
        , mCardIndex(cardIndex)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const std::shared_ptr<CardSoWrapper> mCardSoWrapper;
    const int mCardIndex;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class SerializableGameActionEvent final
{
public:
    SerializableGameActionEvent(const strutils::StringId& actionName, const std::unordered_map<std::string, std::string>& extraActionParams)
        : mActionName(actionName)
        , mExtraActionParams(extraActionParams)
    {
    }
    
    const strutils::StringId mActionName;
    const std::unordered_map<std::string, std::string> mExtraActionParams;
};

///------------------------------------------------------------------------------------------------

class NewBoardCardCreatedEvent final
{
public:
    NewBoardCardCreatedEvent(const std::shared_ptr<CardSoWrapper> cardSoWrapper, const int cardIndex, const bool forRemotePlayer)
        : mCardSoWrapper(cardSoWrapper)
        , mCardIndex(cardIndex)
        , mForRemotePlayer(forRemotePlayer)
    {
    }
    
    const std::shared_ptr<CardSoWrapper> mCardSoWrapper;
    const int mCardIndex;
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class CardSummoningEvent final
{
public:
    CardSummoningEvent(const std::vector<std::shared_ptr<CardSoWrapper>> cardSoWrappers)
        : mCardSoWrappers(cardSoWrappers)
    {
    }
    
    const std::vector<std::shared_ptr<CardSoWrapper>> mCardSoWrappers;
};

///------------------------------------------------------------------------------------------------

class HeroCardCreatedEvent final
{
public:
    HeroCardCreatedEvent(const std::shared_ptr<CardSoWrapper> cardSoWrapper)
        : mCardSoWrapper(cardSoWrapper)
    {
    }
    
    const std::shared_ptr<CardSoWrapper> mCardSoWrapper;
};

///------------------------------------------------------------------------------------------------

class CoinRewardEvent final
{
public:
    CoinRewardEvent(const int coinAmount, const glm::vec3& animationOriginPosition)
        : mCoinAmount(coinAmount)
        , mAnimationOriginPosition(animationOriginPosition)
    {
    }
    
    const int mCoinAmount;
    const glm::vec3 mAnimationOriginPosition;
};

///------------------------------------------------------------------------------------------------

class HealthRefillRewardEvent final
{
public:
    HealthRefillRewardEvent(const int healthAmount, const glm::vec3& animationOriginPosition, bool battleLootHealthRefillCase = false)
        : mHealthAmount(healthAmount)
        , mAnimationOriginPosition(animationOriginPosition)
        , mBattleLootHealthRefillCase(battleLootHealthRefillCase)
    {
    }
    
    const int mHealthAmount;
    const glm::vec3 mAnimationOriginPosition;
    const bool mBattleLootHealthRefillCase;
};

///------------------------------------------------------------------------------------------------

class MaxHealthGainRewardEvent final
{
public:
    MaxHealthGainRewardEvent(const int maxHealthGainAmount)
        : mMaxHealthGainAmount(maxHealthGainAmount)
    {
    }
    
    const int mMaxHealthGainAmount;
};

///------------------------------------------------------------------------------------------------

class StoryBattleWonEvent final
{
};

///------------------------------------------------------------------------------------------------

class GuiRewardAnimationFinishedEvent final
{
};

///------------------------------------------------------------------------------------------------

class CardDeletionAnimationFinishedEvent final
{
};

///------------------------------------------------------------------------------------------------

class BlockInteractionWithHeldCardsEvent final
{    
};

///------------------------------------------------------------------------------------------------

class AchievementUnlockedTriggerEvent final
{
public:
    AchievementUnlockedTriggerEvent(const strutils::StringId& achievementName)
        : mAchievementName(achievementName)
    {
    }
    
    strutils::StringId mAchievementName;
};

///------------------------------------------------------------------------------------------------

class TutorialTriggerEvent final
{
public:
    TutorialTriggerEvent(const strutils::StringId& tutorialName, const glm::vec3& arrowOriginPosition = glm::vec3(), const glm::vec3& arrowTargetPosition = glm::vec3())
        : mTutorialName(tutorialName)
        , mArrowOriginPosition(arrowOriginPosition)
        , mArrowTargetPosition(arrowTargetPosition)
    {
    }
    
    strutils::StringId mTutorialName;
    glm::vec3 mArrowOriginPosition;
    glm::vec3 mArrowTargetPosition;
};

///------------------------------------------------------------------------------------------------

class RareItemCollectedEvent final
{
public:
    RareItemCollectedEvent(const strutils::StringId& rareItemProductId, std::shared_ptr<scene::SceneObject> rareItemSceneObject)
        : mRareItemProductId(rareItemProductId)
        , mRareItemSceneObject(rareItemSceneObject)
    {
    }
    
    const strutils::StringId mRareItemProductId;
    const std::shared_ptr<scene::SceneObject> mRareItemSceneObject;
};

///------------------------------------------------------------------------------------------------

class LastCardPlayedFinalizedEvent final
{
public:
    LastCardPlayedFinalizedEvent(const int cardIndex)
        : mCardIndex(cardIndex)
    {
    }
    
    const int mCardIndex;
};

///------------------------------------------------------------------------------------------------

class WeightChangeAnimationTriggerEvent final
{
public:
    WeightChangeAnimationTriggerEvent(const bool forRemotePlayer)
        : mForRemotePlayer(forRemotePlayer)
    {
        
    }
    
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class HealthChangeAnimationTriggerEvent final
{
public:
    HealthChangeAnimationTriggerEvent(const bool forRemotePlayer)
        : mForRemotePlayer(forRemotePlayer)
    {
        
    }
    
    const bool mForRemotePlayer;
};

///------------------------------------------------------------------------------------------------

class PoisonStackChangeChangeAnimationTriggerEvent final
{
public:
    PoisonStackChangeChangeAnimationTriggerEvent(const bool forRemotePlayer, const int newPoisonStackValue)
        : mForRemotePlayer(forRemotePlayer)
        , mNewPoisonStackValue(newPoisonStackValue)
    {
        
    }
    
    const bool mForRemotePlayer;
    const int mNewPoisonStackValue;
};

///------------------------------------------------------------------------------------------------

class ArmorChangeChangeAnimationTriggerEvent final
{
public:
    ArmorChangeChangeAnimationTriggerEvent(const bool forRemotePlayer, const int newArmorValue)
        : mForRemotePlayer(forRemotePlayer)
        , mNewArmorValue(newArmorValue)
    {
        
    }
    
    const bool mForRemotePlayer;
    const int mNewArmorValue;
};

///------------------------------------------------------------------------------------------------

class CardHistoryEntryAdditionEvent final
{
public:
    CardHistoryEntryAdditionEvent(const bool forRemotePlayer, const bool isTurnCounter, const int cardIndex, const std::string& entryTypeTextureFileName)
        : mForRemotePlayer(forRemotePlayer)
        , mIsTurnCounter(isTurnCounter)
        , mCardIndex(cardIndex)
        , mEntryTypeTextureFileName(entryTypeTextureFileName)
    {
    }
    
    const bool mForRemotePlayer;
    const bool mIsTurnCounter;
    const int mCardIndex;
    const std::string mEntryTypeTextureFileName;
};

///------------------------------------------------------------------------------------------------

class SceneChangeEvent final
{
public:
    SceneChangeEvent
    (
        const strutils::StringId& newSceneName,
        const SceneChangeType sceneChangeType,
        const PreviousSceneDestructionType previousSceneDestructionType
     )
        : mNewSceneName(newSceneName)
        , mSceneChangeType(sceneChangeType)
        , mPreviousSceneDestructionType(previousSceneDestructionType)
    {
    }
    
    const strutils::StringId mNewSceneName;
    const SceneChangeType mSceneChangeType;
    const PreviousSceneDestructionType mPreviousSceneDestructionType;
};

///------------------------------------------------------------------------------------------------

class LoadingProgressPrefixTextOverrideEvent final
{
public:
    LoadingProgressPrefixTextOverrideEvent(const std::string& loadingProgressPrefixTextOverride)
        : mLoadingProgressPrefixTextOverride(loadingProgressPrefixTextOverride)
    {
        
    }
    
    const std::string mLoadingProgressPrefixTextOverride;
};

///------------------------------------------------------------------------------------------------

class PopSceneModalEvent final
{
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* Events_h */
