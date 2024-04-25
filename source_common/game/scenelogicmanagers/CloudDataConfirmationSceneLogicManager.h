///------------------------------------------------------------------------------------------------
///  CloudDataConfirmationSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 22/01/2024
///------------------------------------------------------------------------------------------------

#ifndef CloudDataConfirmationSceneLogicManager_h
#define CloudDataConfirmationSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class CloudDataConfirmationSceneLogicManager final: public ISceneLogicManager
{
public:
    CloudDataConfirmationSceneLogicManager();
    ~CloudDataConfirmationSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void OnUseCloudDataButtonPressed();
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    bool mTransitioningToSubScene;
};

///------------------------------------------------------------------------------------------------

#endif /* CloudDataConfirmationSceneLogicManager_h */
