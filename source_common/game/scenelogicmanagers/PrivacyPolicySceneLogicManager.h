///------------------------------------------------------------------------------------------------
///  PrivacyPolicySceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/02/2024
///------------------------------------------------------------------------------------------------

#ifndef PrivacyPolicySceneLogicManager_h
#define PrivacyPolicySceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <game/SwipeableContainer.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class PrivacyPolicySceneLogicManager final: public ISceneLogicManager
{
public:
    PrivacyPolicySceneLogicManager();
    ~PrivacyPolicySceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    struct TextEntry
    {
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
    };
    
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::unique_ptr<SwipeableContainer<TextEntry>> mTextContainer;
    bool mTransitioning;
};

///------------------------------------------------------------------------------------------------

#endif /* PrivacyPolicySceneLogicManager_h */
