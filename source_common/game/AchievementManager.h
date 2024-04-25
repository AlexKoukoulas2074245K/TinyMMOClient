///------------------------------------------------------------------------------------------------
///  AchievementManager.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 18/03/2024
///------------------------------------------------------------------------------------------------

#ifndef AchievementManager_h
#define AchievementManager_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <game/events/EventSystem.h>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace achievements
{
    inline const strutils::StringId ONE_SHOT_EMERALD_DRAGON = strutils::StringId("one_shot_emerald_dragon");
    inline const strutils::StringId STACK_ARTIFACT_THRICE = strutils::StringId("stack_artifact_thrice");
    inline const strutils::StringId DEAL_10_DAMAGE = strutils::StringId("deal_10_damage");
    inline const strutils::StringId DEAL_20_DAMAGE = strutils::StringId("deal_20_damage");
    inline const strutils::StringId REACH_12_STARTING_WEIGHT = strutils::StringId("reach_12_starting_weight");
    inline const strutils::StringId DRAW_10_CARDS_IN_A_TURN = strutils::StringId("draw_10_cards_in_a_turn");
    inline const strutils::StringId DEFEAT_FINAL_BOSS_WITH_UNUSED_RESURRECTION = strutils::StringId("defeat_final_boss_with_unused_resurrection");
    inline const strutils::StringId NORMAL_COLLECTOR = strutils::StringId("normal_collector");
    inline const strutils::StringId GOLDEN_COLLECTOR = strutils::StringId("golden_collector");
    inline const strutils::StringId DEFEAT_FINAL_BOSS_10_MUTATIONS = strutils::StringId("defeat_final_boss_10_mutations");
    inline const strutils::StringId DEFEAT_FINAL_BOSS_FIRST_TIME = strutils::StringId("defeat_final_boss_first_time");
}

///------------------------------------------------------------------------------------------------

struct AchievementDefinition
{
    AchievementDefinition(const strutils::StringId& achievementName, const std::string& achievementTitle, const std::string& achievementDescription, const std::string& achievementPortraitTextureFileName, const long long achievementBountyReward)
        : mAchievementName(achievementName)
        , mAchievementTitle(achievementTitle)
        , mAchievementDescription(achievementDescription)
        , mAchievementPortraitTextureFileName(achievementPortraitTextureFileName)
        , mAchievementBountyReward(achievementBountyReward)
    {
    }
    
    strutils::StringId mAchievementName;
    std::string mAchievementTitle;
    std::string mAchievementDescription;
    std::string mAchievementPortraitTextureFileName;
    long long mAchievementBountyReward;
};

///------------------------------------------------------------------------------------------------

namespace scene { class SceneObject; }
class AnimatedButton;
class GuiObjectManager;
class AchievementManager final : public events::IListener
{
    friend class Game;
public:
    static AchievementManager& GetInstance();
    ~AchievementManager();
    AchievementManager(const AchievementManager&) = delete;
    AchievementManager(AchievementManager&&) = delete;
    const AchievementManager& operator = (const AchievementManager&) = delete;
    AchievementManager& operator = (AchievementManager&&) = delete;
    
    const std::unordered_map<strutils::StringId, AchievementDefinition, strutils::StringIdHasher>& GetAchievementDefinitions() const;
    bool HasAnyActiveAchievements() const;
    bool IsAchievementActive(const strutils::StringId& tutorialName) const;
    void LoadAchievementDefinitions();
    void Update(const float dtMillis, std::shared_ptr<GuiObjectManager> activeGuiObjectManager);
    
private:
    void CreateAchievement();
    void SwipeOutAchievement();
    void DestroyAchievement();
    void UpdateActiveAchievement(const float dtMillis, std::shared_ptr<GuiObjectManager> activeGuiObjectManager);
    void OnAchievementUnlockedTrigger(const events::AchievementUnlockedTriggerEvent&);
    
private:
    AchievementManager();
    
private:
    std::vector<events::AchievementUnlockedTriggerEvent> mActiveAchievements;
    std::unordered_map<strutils::StringId, AchievementDefinition, strutils::StringIdHasher> mAchievementDefinitions;
    std::vector<std::shared_ptr<scene::SceneObject>> mAchievementSceneObjects;
    std::unique_ptr<AnimatedButton> mContinueButton;
    std::shared_ptr<GuiObjectManager> mLastGuiObjectManager;
};

///------------------------------------------------------------------------------------------------

#endif /* AchievementManager_h */
