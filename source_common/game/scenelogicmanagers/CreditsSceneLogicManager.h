///------------------------------------------------------------------------------------------------
///  CreditsSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 07/03/2024
///------------------------------------------------------------------------------------------------

#ifndef CreditsSceneLogicManager_h
#define CreditsSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <game/SwipeableContainer.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class CreditsSceneLogicManager final: public ISceneLogicManager
{
public:
    CreditsSceneLogicManager();
    ~CreditsSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::vector<std::shared_ptr<scene::SceneObject>> mTextSceneObjects;
    bool mTransitioning;
};

///------------------------------------------------------------------------------------------------

#endif /* CreditsSceneLogicManager_h */
