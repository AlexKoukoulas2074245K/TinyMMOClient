///------------------------------------------------------------------------------------------------
///  CardLibrarySceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 13/01/2024
///------------------------------------------------------------------------------------------------

#ifndef CardLibrarySceneLogicManager_h
#define CardLibrarySceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <game/SwipeableContainer.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class CardTooltipController;
class CardLibrarySceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    CardLibrarySceneLogicManager();
    ~CardLibrarySceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void OnWindowResize(const events::WindowResizeEvent&);
    void CreateCardEntriesAndContainer();
    void CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText);
    void DestroyCardTooltip();
    void SelectCard();
    void DeleteCard();
    void DeselectCard();
    void ToggleFilterCheckbox(std::shared_ptr<scene::SceneObject> filterCheckboxSceneObject);
    void ToggleGoldenCheckbox();
    void SetGoldenCheckboxValue(const bool checkboxValue);
    
private:
    enum class SceneState
    {
        BROWSING_CARDS,
        SELECTED_CARD_FOR_DELETION,
        SELECTED_CARD_IN_CARD_LIBRARY,
        DISSOLVING_DELETED_CARD
    };
    
    struct CardEntry
    {
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
        std::shared_ptr<CardSoWrapper> mCardSoWrapper;
    };
    
    std::shared_ptr<scene::Scene> mScene;
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::unique_ptr<SwipeableContainer<CardEntry>> mCardContainer;
    std::unique_ptr<CardTooltipController> mCardTooltipController;
    glm::vec3 mSelectedCardInitialPosition;
    SceneState mSceneState;
    float mCoinAnimationValue;
    int mSelectedCardIndex;
    bool mTransitioning;
    bool mAnimatingCoinValue;
    bool mHasSentTutorialTrigger;
};

///------------------------------------------------------------------------------------------------

#endif /* CardLibrarySceneLogicManager_h */
