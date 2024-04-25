///------------------------------------------------------------------------------------------------
///  DisconnectedSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/01/2024
///------------------------------------------------------------------------------------------------

#ifndef DisconnectedSceneLogicManager_h
#define DisconnectedSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class DisconnectedSceneLogicManager final: public ISceneLogicManager
{
public:
    DisconnectedSceneLogicManager();
    ~DisconnectedSceneLogicManager();
    
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

#endif /* DisconnectedSceneLogicManager_h */
