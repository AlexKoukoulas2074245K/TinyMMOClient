///------------------------------------------------------------------------------------------------
///  FirstGameLockSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#ifndef FirstGameLockSceneLogicManager_h
#define FirstGameLockSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class FirstGameLockSceneLogicManager final: public ISceneLogicManager
{
public:
    FirstGameLockSceneLogicManager();
    ~FirstGameLockSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    bool mTransitioning;
};

///------------------------------------------------------------------------------------------------

#endif /* FirstGameLockSceneLogicManager_h */
