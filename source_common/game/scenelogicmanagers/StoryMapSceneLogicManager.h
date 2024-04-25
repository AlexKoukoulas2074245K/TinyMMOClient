///------------------------------------------------------------------------------------------------
///  StoryMapSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 18/12/2023
///------------------------------------------------------------------------------------------------

#ifndef StoryMapSceneLogicManager_h
#define StoryMapSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <game/StoryMap.h>
#include <engine/rendering/Camera.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class AnimatedStatContainer;
class GuiObjectManager;
class StoryMapSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    StoryMapSceneLogicManager();
    ~StoryMapSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    void VCreateDebugWidgets() override;
    
private:
    void RegisterForEvents();
    void OnPopSceneModal(const events::PopSceneModalEvent& event);
    void OnWindowResize(const events::WindowResizeEvent& event);
    void ResetSwipeData();
    void SetMapPositionTo(const glm::vec3& position);
    void MoveMapBy(const glm::vec3& delta);
    void MoveGUIBy(const glm::vec3& delta);
    void ResetSelectedMapNode();
    
private:
    enum class MapUpdateState
    {
        NAVIGATING,
        MOVING_TO_NODE,
        FRESH_MAP_ANIMATION
    };
    
    std::unique_ptr<StoryMap> mStoryMap;
    std::unique_ptr<MapCoord> mSelectedMapCoord;
    std::shared_ptr<scene::Scene> mScene;
    std::shared_ptr<GuiObjectManager> mGuiManager;
    std::shared_ptr<StoryMap::NodeData> mTappedMapNodeData;
    std::unordered_set<std::shared_ptr<scene::SceneObject>> mExcludedSceneObjectsFromFrustumCulling;
    rendering::Camera mSwipeCamera;
    glm::vec3 mSwipeVelocity;
    glm::vec3 mSwipeCurrentPos;
    glm::vec3 mCameraTargetPos;
    glm::vec3 mPreviousDirectionToTargetNode;
    glm::vec3 mFreshMapCameraAnimationInitPosition;
    glm::vec3 mTappedNodeInitCameraPosition;
    glm::vec2 mMapSwipeXBounds;
    glm::vec2 mMapSwipeYBounds;
    MapUpdateState mMapUpdateState;
    bool mHasStartedSwipe;
};

///------------------------------------------------------------------------------------------------

#endif /* StoryMapSceneLogicManager_h */
