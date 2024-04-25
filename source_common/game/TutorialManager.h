///------------------------------------------------------------------------------------------------
///  TutorialManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/03/2024
///------------------------------------------------------------------------------------------------

#ifndef TutorialManager_h
#define TutorialManager_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <game/events/EventSystem.h>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace tutorials
{
    inline const strutils::StringId SELECT_DECK_1_TUTORIAL = strutils::StringId("select_deck_1");
    inline const strutils::StringId SELECT_DECK_2_TUTORIAL = strutils::StringId("select_deck_2");
    inline const strutils::StringId SELECT_DECK_3_TUTORIAL = strutils::StringId("select_deck_3");
    inline const strutils::StringId STORY_MAP_1_TUTORIAL = strutils::StringId("story_map_1");
    inline const strutils::StringId BATTLE_1_TUTORIAL = strutils::StringId("battle_1");
    inline const strutils::StringId BATTLE_2_TUTORIAL = strutils::StringId("battle_2");
    inline const strutils::StringId BATTLE_3_TUTORIAL = strutils::StringId("battle_3");
    inline const strutils::StringId BATTLE_ARMOR_TUTORIAL = strutils::StringId("battle_armor");
    inline const strutils::StringId BATTLE_DREW_NORMAL_CARD_TUTORIAL = strutils::StringId("battle_drew_normal_card");
    inline const strutils::StringId BATTLE_DREW_SPELL_TUTORIAL = strutils::StringId("battle_drew_spell_card");
    inline const strutils::StringId BATTLE_DREW_SINGLE_USE_SPELL_TUTORIAL = strutils::StringId("battle_drew_single_use_spell_card");
    inline const strutils::StringId BATTLE_HISTORY_TUTORIAL = strutils::StringId("battle_history");
    inline const strutils::StringId BATTLE_END_TURN_TUTORIAL = strutils::StringId("battle_end_turn");
    inline const strutils::StringId BATTLE_HOW_TO_PLAY_A_CARD_TUTORIAL = strutils::StringId("battle_how_to_play_a_card");
    inline const strutils::StringId BATTLE_VICTORY_TUTORIAL = strutils::StringId("battle_victory");
    inline const strutils::StringId BATTLE_CARD_SELECTION_REWARD_TUTORIAL = strutils::StringId("battle_card_selection_reward");
    inline const strutils::StringId BATTLE_WHEEL_REWARD_TUTORIAL = strutils::StringId("battle_wheel_reward");
    inline const strutils::StringId EVENT_TUTORIAL = strutils::StringId("event");
    inline const strutils::StringId NEW_ARTIFACT_IN_BAG_TUTORIAL = strutils::StringId("new_artifact_in_bag");
    inline const strutils::StringId NEW_CARD_IN_DECK_TUTORIAL = strutils::StringId("new_card_in_deck");
    inline const strutils::StringId STORY_SHOP_TUTORIAL = strutils::StringId("story_shop");
    inline const strutils::StringId PERMA_SHOP_TUTORIAL = strutils::StringId("perma_shop");
    inline const strutils::StringId CARD_LIBRARY_TUTORIAL = strutils::StringId("card_library");
    inline const strutils::StringId MUTATIONS_TUTORIAL = strutils::StringId("mutations");
}

///------------------------------------------------------------------------------------------------

struct TutorialDefinition
{
    TutorialDefinition(const strutils::StringId& tutorialName, const std::string& tutorialDescription, const bool showArrow)
        : mTutorialName(tutorialName)
        , mTutorialDescription(tutorialDescription)
        , mShowArrow(showArrow)
    {
    }
    
    const strutils::StringId mTutorialName;
    const std::string mTutorialDescription;
    const bool mShowArrow;
};

///------------------------------------------------------------------------------------------------

namespace scene { class SceneObject; }
class AnimatedButton;
class TutorialManager final : public events::IListener
{
    friend class Game;
public:
    TutorialManager();
    ~TutorialManager();
    TutorialManager(const TutorialManager&) = delete;
    TutorialManager(TutorialManager&&) = delete;
    const TutorialManager& operator = (const TutorialManager&) = delete;
    TutorialManager& operator = (TutorialManager&&) = delete;
    
    const std::unordered_map<strutils::StringId, TutorialDefinition, strutils::StringIdHasher>& GetTutorialDefinitions() const;
    bool HasAnyActiveTutorial() const;
    bool IsTutorialActive(const strutils::StringId& tutorialName) const;
    void LoadTutorialDefinitions();
    void Update(const float dtMillis);
    
private:
    void CreateTutorial();
    void FadeOutTutorial();
    void DestroyTutorial();
    void UpdateActiveTutorial(const float dtMillis);
    void OnTutorialTrigger(const events::TutorialTriggerEvent&);
    void ToggleCheckbox();
    void SetCheckboxValue(const bool checkboxValue);
    
private:
    std::vector<events::TutorialTriggerEvent> mActiveTutorials;
    std::unordered_map<strutils::StringId, TutorialDefinition, strutils::StringIdHasher> mTutorialDefinitions;
    std::vector<std::shared_ptr<scene::SceneObject>> mTutorialSceneObjects;
    std::unique_ptr<AnimatedButton> mContinueButton;
};

///------------------------------------------------------------------------------------------------

#endif /* TutorialManager_h */
