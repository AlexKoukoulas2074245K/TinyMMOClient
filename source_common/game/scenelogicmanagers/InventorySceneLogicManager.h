///------------------------------------------------------------------------------------------------
///  InventorySceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/02/2024
///------------------------------------------------------------------------------------------------

#ifndef InventorySceneLogicManager_h
#define InventorySceneLogicManager_h

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
class InventorySceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    InventorySceneLogicManager();
    ~InventorySceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    void VCreateDebugWidgets() override;
    
private:
    struct ItemEntry
    {
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
        strutils::StringId mArtifactOrMutationName;
    };
    
private:
    void UpdateItemContainer(const float dtMillis, std::unique_ptr<SwipeableContainer<ItemEntry>>& itemContainer);
    void OnWindowResize(const events::WindowResizeEvent&);
    void CreateItemEntriesAndContainer();
    void CreateItemTooltip(const glm::vec3& itemOriginPostion, const std::string& tooltipText);
    void DestroyItemTooltip();
    void UpdateMutationInteraction(const float dtMillis, std::shared_ptr<scene::Scene> scene);
    
private:
    std::shared_ptr<scene::Scene> mScene;
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::unique_ptr<SwipeableContainer<ItemEntry>> mArtifactsItemContainer;
    std::unique_ptr<CardTooltipController> mItemTooltipController;
    int mToolTipIndex;
    float mToolTipPointeePosY;
    float mToolTipPointeePosX;
    int mSelectedItemIndex;
    bool mTransitioning;
    bool mShowingMutationText;
};

///------------------------------------------------------------------------------------------------

#endif /* InventorySceneLogicManager_h */
