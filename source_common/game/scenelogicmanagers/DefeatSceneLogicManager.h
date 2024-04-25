///------------------------------------------------------------------------------------------------
///  DefeatSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/01/2024
///------------------------------------------------------------------------------------------------

#ifndef DefeatSceneLogicManager_h
#define DefeatSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class DefeatSceneLogicManager final: public ISceneLogicManager
{
public:
    DefeatSceneLogicManager();
    ~DefeatSceneLogicManager();
    
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
    bool mTransitioningToSubScene;
};

///------------------------------------------------------------------------------------------------

#endif /* DefeatSceneLogicManager_h */
