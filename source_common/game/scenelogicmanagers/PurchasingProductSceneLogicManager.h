///------------------------------------------------------------------------------------------------
///  PurchasingProductSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 30/01/2024
///------------------------------------------------------------------------------------------------

#ifndef PurchasingProductSceneLogicManager_h
#define PurchasingProductSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class PurchasingProductSceneLogicManager final: public ISceneLogicManager
{
public:
    PurchasingProductSceneLogicManager();
    ~PurchasingProductSceneLogicManager();
    
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
        MAIN,
        PURCHASE_SUCCESSFUL,
        PURCHASE_UNSUCCESSFUL
    };
    
private:
    void InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene);
    void TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene);
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    SubSceneType mActiveSubScene;
    SubSceneType mNextSubScene;
    float mMinTimeBeforeTransitioningToSubSceneSecs;
    bool mTransitioningToSubScene;
    bool mShouldTriggerPurchaseEndedEvent;
};

///------------------------------------------------------------------------------------------------

#endif /* PurchasingProductSceneLogicManager_h */
