///------------------------------------------------------------------------------------------------
///  MainMenuSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 12/12/2023
///------------------------------------------------------------------------------------------------

#ifndef MainMenuSceneLogicManager_h
#define MainMenuSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <game/events/EventSystem.h>
#include <game/SwipeableContainer.h>
#include <memory>
#include <unordered_set>
#include <stack>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
struct QuickPlayData;
class MainMenuSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    MainMenuSceneLogicManager();
    ~MainMenuSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    enum class SubSceneType
    {
        NONE,
        MAIN,
        STORY_MODE,
        NEW_STORY_CONFIRMATION,
        NEW_STORY_DECK_SELECTION,
        MUTATION_SELECTION,
        EXTRAS
    };
    
    struct CardFamilyEntry
    {
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
        strutils::StringId mCardFamilyName;
    };
    
private:
    void InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene);
    void TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene);
    void BattleModeSelected(const strutils::StringId& buttonName);
    void DeckSelected(const int selectedDeckIndex, const bool forTopPlayer, std::shared_ptr<scene::Scene> scene);
    void GoToPreviousSubScene(std::shared_ptr<scene::Scene> mainScene);
    void StartNewStory();
    void OnEnterGiftCodeButtonPressed();
    bool IsDisconnected() const;
    void CreateMutationObject(std::shared_ptr<scene::Scene> scene);
    void SetMutationLevel(const int mutationLevel, std::shared_ptr<scene::Scene> scene);
    void OnWindowResize(const events::WindowResizeEvent& event);
    void CheckForCardCompletion();
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::unique_ptr<SwipeableContainer<CardFamilyEntry>> mCardFamilyContainerTop;
    std::unique_ptr<SwipeableContainer<CardFamilyEntry>> mCardFamilyContainerBot;
    std::unique_ptr<QuickPlayData> mQuickPlayData;
    std::vector<std::shared_ptr<scene::SceneObject>> mDeckSelectionSceneObjects;
    SubSceneType mActiveSubScene;
    std::stack<SubSceneType> mPreviousSubSceneStack;
    bool mTransitioningToSubScene;
    bool mNeedToSetBoardPositionAndZoomFactor;
    bool mShouldPushToPreviousSceneStack;
};

///------------------------------------------------------------------------------------------------

#endif /* MainMenuSceneLogicManager_h */
