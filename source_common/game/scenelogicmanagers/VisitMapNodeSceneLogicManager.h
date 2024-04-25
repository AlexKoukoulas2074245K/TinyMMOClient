///------------------------------------------------------------------------------------------------
///  VisitMapNodeSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#ifndef VisitMapNodeSceneLogicManager_h
#define VisitMapNodeSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class VisitMapNodeSceneLogicManager final: public ISceneLogicManager
{
public:
    VisitMapNodeSceneLogicManager();
    ~VisitMapNodeSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void SkipNode();
    void InitializeNodeVisitData();
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    bool mTransitioning;
};

///------------------------------------------------------------------------------------------------

#endif /* VisitMapNodeSceneLogicManager_h */
