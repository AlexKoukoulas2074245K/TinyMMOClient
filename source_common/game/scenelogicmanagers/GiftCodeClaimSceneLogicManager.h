///------------------------------------------------------------------------------------------------
///  GiftCodeClaimSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/02/2024
///------------------------------------------------------------------------------------------------

#ifndef GiftCodeClaimSceneLogicManager_h
#define GiftCodeClaimSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class GiftCodeClaimSceneLogicManager final: public ISceneLogicManager
{
public:
    GiftCodeClaimSceneLogicManager();
    ~GiftCodeClaimSceneLogicManager();
    
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

#endif /* GiftCodeClaimSceneLogicManager_h */
