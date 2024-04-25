///------------------------------------------------------------------------------------------------
///  VictorySceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/02/2024
///------------------------------------------------------------------------------------------------

#ifndef VictorySceneLogicManager_h
#define VictorySceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class VictorySceneLogicManager final: public ISceneLogicManager
{
public:
    VictorySceneLogicManager();
    ~VictorySceneLogicManager();
    
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
        INTRO,
        RESULTS
    };
    
private:
    void InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene);
    void TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene);
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    SubSceneType mActiveSubScene;
    float mInitialSurfacingDelaySecs;
    bool mTransitioningToSubScene;
    bool mInitialSurfacingHappened;
};

///------------------------------------------------------------------------------------------------

#endif /* VictorySceneLogicManager_h */
