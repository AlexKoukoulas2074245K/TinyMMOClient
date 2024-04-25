///------------------------------------------------------------------------------------------------
///  WheelOfFortuneSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 08/01/2024
///------------------------------------------------------------------------------------------------

#ifndef WheelOfFortuneSceneLogicManager_h
#define WheelOfFortuneSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class WheelOfFortuneController;
class WheelOfFortuneSceneLogicManager final: public ISceneLogicManager
{
public:
    WheelOfFortuneSceneLogicManager();
    ~WheelOfFortuneSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void OnWheelItemSelected(const int itemIndex, const std::shared_ptr<scene::SceneObject> itemSceneObject);
    
private:
    std::shared_ptr<scene::Scene> mScene;
    std::vector<strutils::StringId> mWheelRewards;
    std::unique_ptr<AnimatedButton> mSpinButton;
    std::unique_ptr<AnimatedButton> mContinueButton;
    std::unique_ptr<WheelOfFortuneController> mWheelController;
    bool mHasSpinnedWheel;
    bool mFinalBossFlow;
};

///------------------------------------------------------------------------------------------------

#endif /* MainMenuSceneLogicManager_h */
