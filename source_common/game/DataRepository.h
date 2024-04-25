///------------------------------------------------------------------------------------------------
///  DataRepository.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 08/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef DataRepository_h
#define DataRepository_h

///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/GameConstants.h>
#include <game/StoryMap.h>
#include <game/utils/ValueWithDelayedDisplay.h>
#include <engine/utils/MathUtils.h>
#include <vector>

///------------------------------------------------------------------------------------------------

enum class BattleControlType
{
    REPLAY,
    AI_TOP_BOT,
    AI_TOP_ONLY
};

///------------------------------------------------------------------------------------------------

enum class StoryMapSceneType
{
    STORY_MAP,
    EVENT,
    BATTLE,
    SHOP
};

///------------------------------------------------------------------------------------------------

enum class BattleSubSceneType
{
    BATTLE,
    WHEEL,
    CARD_SELECTION,
    STORY_VICTORY
};

///------------------------------------------------------------------------------------------------

enum class WheelOfFortuneType
{
    ELITE,
    TUTORIAL_BOSS,
    FINAL_BOSS
};

///------------------------------------------------------------------------------------------------

enum class CardLibraryBehaviorType
{
    STORY_CARDS,
    BROWSING_FOR_DELETION,
    CARD_LIBRARY
};

///------------------------------------------------------------------------------------------------

enum class ShopBehaviorType
{
    STORY_SHOP,
    PERMA_SHOP
};

///------------------------------------------------------------------------------------------------

enum class CardPackType
{
    NONE,
    NORMAL,
    GOLDEN
};

///------------------------------------------------------------------------------------------------

enum class ForeignCloudDataFoundType
{
    NONE,
    OPTIONAL,
    MANDATORY
};

///------------------------------------------------------------------------------------------------

enum class GiftCodeClaimedResultType
{
    SUCCESS,
    FAILURE_USED_ALREADY,
    FAILURE_INVALID_CODE,
    FAILURE_INVALID_PRODUCT
};

///------------------------------------------------------------------------------------------------

struct QuickPlayData
{
    BattleControlType mBattleControlType;
    std::vector<int> mTopPlayerDeck;
    std::vector<int> mBotPlayerDeck;
    int mMutationLevel = 0;
};

///------------------------------------------------------------------------------------------------

class StoryDeserializer;
class StorySerializer;
class PersistentAccountDataSerializer;
class PersistentAccountDataDeserializer;
class DataRepository final
{
public:
    static DataRepository& GetInstance();
    ~DataRepository() = default;
    
    DataRepository(const DataRepository&) = delete;
    DataRepository(DataRepository&&) = delete;
    const DataRepository& operator = (const DataRepository&) = delete;
    DataRepository& operator = (DataRepository&&) = delete;
    
    void ResetStoryData();
    void ReloadProgressionDataFromFile();
    void FlushStateToFile();
    
    const std::unordered_map<CardStatType, int>& GetStoryPlayerCardStatModifiers() const;
    void SetStoryPlayerCardStatModifier(const CardStatType statType, const int statModifier);
    void ClearStoryPlayerCardStatModifiers();
    
    const std::unordered_map<int, bool>& GetGoldenCardIdMap() const;
    void SetGoldenCardMapEntry(const int cardId, const bool goldenCardEnabled);
    void ClearGoldenCardIdMap();
    
    const std::vector<CardPackType>& GetPendingCardPacks() const;
    void AddPendingCardPack(const CardPackType cardPackType);
    CardPackType PopFrontPendingCardPack();
    
    QuickPlayData* GetQuickPlayData() const;
    void SetQuickPlayData(std::unique_ptr<QuickPlayData> quickPlayData);
    
    ValueWithDelayedDisplay<long long>& CurrencyCoins();
    ValueWithDelayedDisplay<int>& StoryCurrentHealth();
    
    BattleControlType GetNextBattleControlType() const;
    void SetNextBattleControlType(const BattleControlType nextBattleControlType);
    
    StoryMapSceneType GetCurrentStoryMapSceneType() const;
    void SetCurrentStoryMapSceneType(const StoryMapSceneType currentStoryMapSceneType);
    
    BattleSubSceneType GetCurrentBattleSubSceneType() const;
    void SetCurrentBattleSubSceneType(const BattleSubSceneType currentBattleSubSceneType);
    
    CardLibraryBehaviorType GetCurrentCardLibraryBehaviorType() const;
    void SetCurrentCardLibraryBehaviorType(const CardLibraryBehaviorType currentCardLibraryBehaviorType);
    
    ShopBehaviorType GetCurrentShopBehaviorType() const;
    void SetCurrentShopBehaviorType(const ShopBehaviorType currentShopBehaviorType);
    
    StoryMapType GetCurrentStoryMapType() const;
    void SetCurrentStoryMapType(const StoryMapType currentStoryMapType);
    
    WheelOfFortuneType GetCurrentWheelOfFortuneType() const;
    void SetCurrentWheelOfFortuneType(const WheelOfFortuneType currentWheelOfFortuneType);
    
    GiftCodeClaimedResultType GetCurrentGiftCodeClaimedResultType() const;
    void SetCurrentGiftCodeClaimedResultType(const GiftCodeClaimedResultType currentGiftCodeClaimedResultType);
    
    const std::vector<int>& GetUnlockedCardIds() const;
    void SetUnlockedCardIds(const std::vector<int>& unlockedCardIds);
    
    const std::vector<int>& GetCurrentStoryPlayerDeck() const;
    void SetCurrentStoryPlayerDeck(const std::vector<int>& deck);
    
    const std::vector<int>& GetNextTopPlayerDeck() const;
    void SetNextTopPlayerDeck(const std::vector<int>& deck);
    
    const std::vector<int>& GetNextBotPlayerDeck() const;
    void SetNextBotPlayerDeck(const std::vector<int>& deck);
    
    const std::vector<int>& GetNewCardIds() const;
    void SetNewCardIds(const std::vector<int>& newCardIds);
    
    const std::vector<int>& GetSeenOpponentSpellCardIds() const;
    void SetSeenOpponentSpellCardIds(const std::vector<int>& seenOpponentSpellCardIds);
    
    bool HasSeenTutorial(const strutils::StringId& tutorial) const;
    const std::vector<strutils::StringId>& GetSeenTutorials() const;
    void SetSeenTutorials(const std::vector<strutils::StringId>& seenTutorials);
    
    bool HasUnlockedAchievement(const strutils::StringId& achievement) const;
    const std::vector<strutils::StringId>& GetUnlockedAchievements() const;
    void SetUnlockedAchievements(const std::vector<strutils::StringId>& unlockedAchievements);
    
    const std::vector<int>& GetStoryDeletedCardIds() const;
    void SetStoryDeletedCardIds(const std::vector<int>& storyDeletedCardIds);
    
    const std::vector<int>& GetAllMutationLevelVictoryCounts() const;
    int GetMaxMutationLevelWithAtLeastOneVictory() const;
    const int& GetMutationLevelVictories(const int mutationLevel) const;
    void SetAllMutationLevelVictoryCounts(const std::vector<int>& mutationLevelVictoryCounts);
    void SetMutationLevelVictories(const int mutationLevel, const int victoryCount);
    
    const std::vector<int>& GetAllMutationLevelBestTimes() const;
    const int& GetMutationLevelBestTime(const int mutationLevel) const;
    void SetAllMutationLevelBestTimes(const std::vector<int>& mutationLevelBestTimes);
    void SetMutationLevelBestTime(const int mutationLevel, const int bestTimeSecs);
    
    const std::vector<std::string>& GetSuccessfulTransactionIds() const;
    void SetSuccessfulTransactionIds(const std::vector<std::string>& successfulTransactionIds);
    
    const std::vector<std::string>& GetGiftCodesClaimed() const;
    void SetGiftCodesClaimed(const std::vector<std::string>& giftCodesClaimed);
    
    const int& GetGamesFinishedCount() const;
    void SetGamesFinishedCount(const int gamesFinishedCount);
    
    const int& GetStoryMaxHealth() const;
    void SetStoryMaxHealth(const int storyMaxHealth);
    
    const int& GetStoryMapGenerationSeed() const;
    void SetStoryMapGenerationSeed(const int storyMapGenerationSeed);
    
    const long long& GetStoryStartingGold() const;
    void SetStoryStartingGold(const long long storyStartingGold);
    
    const int& GetCurrentStoryMapNodeSeed() const;
    void SetCurrentStoryMapNodeSeed(const int currentStoryMapNodeSeed);
    
    const int& GetNextCardPackSeed() const;
    void SetNextCardPackSeed(const int nextCardPackSeed);
    
    StoryMap::NodeType GetCurrentStoryMapNodeType() const;
    void SetCurrentStoryMapNodeType(const StoryMap::NodeType currentStoryMapNodeType);
    
    const int& GetCurrentEventScreenIndex() const;
    void SetCurrentEventScreenIndex(const int currentEventScreenIndex);
    
    const int& GetCurrentEventIndex() const;
    void SetCurrentEventIndex(const int currentEventIndex);
        
    const int& GetNextBattleTopPlayerHealth() const;
    void SetNextBattleTopPlayerHealth(const int nextBattleTopPlayerHealth);
    
    const int& GetNextBattleBotPlayerHealth() const;
    void SetNextBattleBotPlayerHealth(const int nextBattleBotPlayerHealth);
    
    const int& GetNextBattleTopPlayerInitWeight() const;
    void SetNextBattleTopPlayerInitWeight(const int nextBattleTopPlayerInitWeight);
    
    const int& GetNextBattleBotPlayerInitWeight() const;
    void SetNextBattleBotPlayerInitWeight(const int nextBattleBotPlayerInitWeight);
    
    const int& GetNextBattleTopPlayerWeightLimit() const;
    void SetNextBattleTopPlayerWeightLimit(const int nextBattleTopPlayerWeightLimit);
    
    const int& GetNextBattleBotPlayerWeightLimit() const;
    void SetNextBattleBotPlayerWeightLimit(const int nextBattleBotPlayerWeightLimit);
    
    const int& GetNextStoryOpponentDamage() const;
    void SetNextStoryOpponentDamage(const int nextStoryOpponentDamage);
    
    const int& GetCurrentStorySecondsPlayed() const;
    void SetCurrentStorySecondPlayed(const int currentStorySecondsPlayed);
    
    const int& GetTotalSecondsPlayed() const;
    void SetTotalSecondsPlayed(const int totalSecondsPlayed);
    
    const int& GetNextUnseenSpellCardId() const;
    void SetNextUnseenSpellCardId(const int nextUnseenSpellCardId);
    
    const int& GetNextInspectedCardId() const;
    void SetNextInspectedCardId(const int nextInspectedCardId);
    
    const int& GetGoldCartsIgnored() const;
    void SetGoldCartsIgnored(const int goldCartsIgnored);
    
    const int& GetCurrentStoryMutationLevel() const;
    bool DoesCurrentStoryHaveMutation(const int storyMutationLevel) const;
    void SetCurrentStoryMutationLevel(const int storyMutationLevel);
    
    const std::vector<std::pair<int, int>>& GetCurrentShopBoughtProductCoordinates() const;
    void ClearShopBoughtProductCoordinates();
    void SetShopBoughtProductCoordinates(const std::vector<std::pair<int, int>>& shopBoughtProductCoordinates);
    void AddShopBoughtProductCoordinates(const std::pair<int, int>& shopBoughtProductCoordinates);
    
    const std::vector<std::pair<strutils::StringId, int>>& GetCurrentStoryArtifacts() const;
    int GetStoryArtifactCount(const strutils::StringId& storyArtifact) const;
    void SetStoryArtifactCount(const strutils::StringId& storyArtifact, int newCount);
    void ClearCurrentStoryArtifacts();
    void AddStoryArtifact(const strutils::StringId& storyArtifact);
    void SetCurrentStoryArtifacts(const std::vector<std::pair<strutils::StringId, int>>& storyArtifacts);
    
    const glm::ivec2& GetCurrentStoryMapNodeCoord() const;
    void SetCurrentStoryMapNodeCoord(const glm::ivec2& currentStoryMapNodeCoord);
    
    const glm::ivec2& GetPreBossMidMapNodeCoord() const;
    void SetPreBossMidMapNodeCoord(const glm::ivec2& preBossMidMapNodeCoord);
    
    const StoryMap::NodeData* GetSelectedStoryMapNodeData() const;
    void SetSelectedStoryMapNodeData(const StoryMap::NodeData* selectedStoryMapNodeData);
    
    const glm::vec3& GetSelectedStoryMapNodePosition() const;
    void SetSelectedStoryMapNodePosition(const glm::vec3& selectedStoryMapNodePosition);
    
    const std::string& GetNextStoryOpponentTexturePath() const;
    void SetNextStoryOpponentTexturePath(const std::string& nextStoryOpponentTexturePath);
    
    const std::string& GetNextStoryOpponentName() const;
    void SetNextStoryOpponentName(const std::string& nextStoryOpponentName);
    
    const std::string& GetCloudDataDeviceNameAndTime() const;
    void SetCloudDataDeviceNameAndTime(const std::string& cloudDataDeviceNameAndTime);
    
    const std::string& GetPermaShopProductNameToPurchase() const;
    void SetPermaShopProductNameToPurchase(const std::string& permaShopProductNameToPurchase);
    
    const bool& IsCurrentlyPlayingStoryMode() const;
    void SetIsCurrentlyPlayingStoryMode(const bool isCurrentlyPlayingStoryMode);
    
    const bool& CanSurfaceCloudDataScene() const;
    void SetCanSurfaceCloudDataScene(const bool canSurfaceCloudDataScene);
    
    const bool& IsAudioEnabled() const;
    void SetAudioEnabled(const bool musicEnabled);
    
    const bool& AreTutorialsEnabled() const;
    void SetTutorialsEnabled(const bool tutorialsEnabled);
    
    const bool& HasSeenMountainOfGoldEvent() const;
    void SetHasSeenMountainOfGoldEvent(const bool hasSeenMountainOfGoldEvent);
    
    ForeignCloudDataFoundType GetForeignProgressionDataFound() const;
    void SetForeignProgressionDataFound(const ForeignCloudDataFoundType foreignProgressionDataFound);
    
private:
    DataRepository();
    
private:
    std::unordered_map<CardStatType, int> mStoryPlayerCardStatModifiers;
    std::unordered_map<int, bool> mGoldenCardIdMap;
    std::unique_ptr<PersistentAccountDataDeserializer> mPersistentDataDeserializer;
    std::unique_ptr<PersistentAccountDataSerializer> mPersistentDataSerializer;
    std::unique_ptr<StoryDeserializer> mStoryDataDeserializer;
    std::unique_ptr<StorySerializer> mStoryDataSerializer;
    std::unique_ptr<QuickPlayData> mQuickPlayData;
    BattleControlType mNextBattleControlType;
    StoryMapSceneType mCurrentStoryMapSceneType;
    BattleSubSceneType mCurrentBattleSubSceneType;
    CardLibraryBehaviorType mCurrentCardLibraryBehaviorType;
    ShopBehaviorType mCurrentShopBehaviorType;
    StoryMapType mCurrentStoryMapType;
    WheelOfFortuneType mCurrentWheelOfFortuneType;
    GiftCodeClaimedResultType mCurrentGiftCodeClaimedResultType;
    ForeignCloudDataFoundType mForeignProgressionDataFound = ForeignCloudDataFoundType::NONE;
    std::vector<int> mUnlockedCardIds;
    std::vector<int> mCurrentStoryPlayerDeck;
    std::vector<int> mNextTopPlayerDeck;
    std::vector<int> mNextBotPlayerDeck;
    std::vector<int> mStoryDeletedCards;
    std::vector<int> mNewCardIds;
    std::vector<int> mSeenOpponentSpellCardIds;
    std::vector<int> mStoryMutationLevelVictories;
    std::vector<int> mStoryMutationLevelBestTimes;
    std::vector<strutils::StringId> mSeenTutorials;
    std::vector<strutils::StringId> mUnlockedAchievements;
    std::vector<std::string> mSuccessfulTransactionIds;
    std::vector<std::string> mGiftCodesClaimed;
    std::vector<std::pair<int, int>> mCurrentShopBoughtProductCoordinates;
    std::vector<std::pair<strutils::StringId, int>> mCurrentStoryArtifacts;
    std::vector<CardPackType> mPendingCardPacks;
    std::string mNextStoryOpponentTexturePath;
    std::string mNextStoryOpponentName;
    std::string mCloudDataDeviceAndTime;
    std::string mPermaShopProductNameToPurchase;
    glm::vec3 mSelectedStoryMapNodePosition = {};
    glm::ivec2 mCurrentStoryMapNodeCoord = game_constants::STORY_MAP_INIT_COORD;
    glm::ivec2 mPreBossMidMapNodeCoord = game_constants::STORY_MAP_INIT_COORD;
    StoryMap::NodeType mCurrentStoryMapNodeType = StoryMap::NodeType::NORMAL_ENCOUNTER;
    const StoryMap::NodeData* mSelectedStoryMapNodeData = nullptr;
    ValueWithDelayedDisplay<int> mStoryCurrentHealth;
    ValueWithDelayedDisplay<long long> mCurrencyCoins;
    int mGamesFinishedCount = 0;
    int mStoryMaxHealth = 0;
    int mStoryMapGenerationSeed = 0;
    int mCurrentStoryMapNodeSeed = 0;
    int mNextCardPackSeed = 0;
    int mCurrentEventScreenIndex = 0;
    int mCurrentStoryMutationLevel = 0;
    int mCurrentEventIndex = 0;
    int mNextBattleTopPlayerHealth = 0;
    int mNextBattleBotPlayerHealth = 0;
    int mNextBattleTopPlayerInitWeight = 0;
    int mNextBattleBotPlayerInitWeight = 0;
    int mNextBattleTopPlayerWeightLimit = 0;
    int mNextBattleBotPlayerWeightLimit = 0;
    int mNextStoryOpponentDamage = 0;
    int mCurrentStorySecondsPlayed = 0;
    int mTotalSecondsPlayed = 0;
    int mNextUnseenSpellCardId = 0;
    int mNextInspectedCardId = 0;
    int mGoldCartsIgnored = 0;
    long long mStoryStartingGold = 0;
    bool mIsCurrentlyPlayingStoryMode = false;
    bool mCanSurfaceCloudDataScene = false;
    bool mAudioEnabled = true;
    bool mHasSeenMountainOfGoldEvent = false;
    bool mTutorialsEnabled = true;
};

///------------------------------------------------------------------------------------------------

#endif /* DataRepository_h */
