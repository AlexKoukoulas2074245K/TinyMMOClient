///------------------------------------------------------------------------------------------------
///  CardInspectionSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/03/2024
///------------------------------------------------------------------------------------------------

#ifndef CardInspectionSceneLogicManager_h
#define CardInspectionSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class CardTooltipController;
struct CardSoWrapper;
class CardInspectionSceneLogicManager final: public ISceneLogicManager
{
public:
    CardInspectionSceneLogicManager();
    ~CardInspectionSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    std::unique_ptr<CardTooltipController> mCardTooltipController;
    std::shared_ptr<CardSoWrapper> mCardSoWrapper;
    std::vector<std::unique_ptr<AnimatedButton>> mAnimatedButtons;
    bool mTransitioning;
};

///------------------------------------------------------------------------------------------------

#endif /* CardInspectionSceneLogicManager_h */
