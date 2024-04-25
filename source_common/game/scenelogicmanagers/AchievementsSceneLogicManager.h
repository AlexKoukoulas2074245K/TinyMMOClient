///------------------------------------------------------------------------------------------------
///  AchievementsSceneLogicManager.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 19/03/2024
///------------------------------------------------------------------------------------------------

#ifndef AchievementsSceneLogicManager_h
#define AchievementsSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <game/SwipeableContainer.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class CardTooltipController;
class AchievementsSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    AchievementsSceneLogicManager();
    ~AchievementsSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    struct AchievementEntry
    {
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
        strutils::StringId mAchievementName;
    };
    
private:
    void UpdateAchievementContainer(const float dtMillis);
    void OnWindowResize(const events::WindowResizeEvent&);
    void CreateAchievementEntriesAndContainer();
    void CreateAchievementTooltip(const glm::vec3& achievementOriginPostion, const std::string& tooltipText);
    void DestroyAchievementTooltip();
    
private:
    std::shared_ptr<scene::Scene> mScene;
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    std::unique_ptr<SwipeableContainer<AchievementEntry>> mAchievementsContainer;
    std::unique_ptr<CardTooltipController> mAchievementTooltipController;
    int mToolTipIndex;
    float mToolTipPointeePosY;
    float mToolTipPointeePosX;
    float mLightRayPositionX;
    int mSelectedAchievementIndex;
    bool mTransitioning;
};

///------------------------------------------------------------------------------------------------

#endif /* AchievementsSceneLogicManager_h */
