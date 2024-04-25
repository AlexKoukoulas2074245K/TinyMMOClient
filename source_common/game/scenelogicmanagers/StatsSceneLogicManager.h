///------------------------------------------------------------------------------------------------
///  StatsSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 08/03/2024
///------------------------------------------------------------------------------------------------

#ifndef StatsSceneLogicManager_h
#define StatsSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class StatsSceneLogicManager final: public ISceneLogicManager
{
public:
    StatsSceneLogicManager();
    ~StatsSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void FadeOutMutationVictoriesAndBestTimesScreen(std::shared_ptr<scene::Scene> scene);
    void CreateMutationVictoriesAndBestTimesScreen(std::shared_ptr<scene::Scene> scene);
    void CreateTotalStatsScreen(std::shared_ptr<scene::Scene> scene);
    
private:
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::vector<std::shared_ptr<scene::SceneObject>> mTextSceneObjects;
    bool mTransitioning;
    bool mHasShownTotalStatsScreen;
};

///------------------------------------------------------------------------------------------------

#endif /* StatsSceneLogicManager_h */
