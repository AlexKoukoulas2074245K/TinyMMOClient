///------------------------------------------------------------------------------------------------
///  CardPackRewardSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 24/01/2024
///------------------------------------------------------------------------------------------------

#ifndef CardPackRewardSceneLogicManager_h
#define CardPackRewardSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <game/DataRepository.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <memory>

///------------------------------------------------------------------------------------------------

class AnimatedButton;
class CardTooltipController;
class CardPackRewardSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    CardPackRewardSceneLogicManager();
    ~CardPackRewardSceneLogicManager();
    
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void RegisterForEvents();
    void OnWindowResize(const events::WindowResizeEvent& event);
    void UpdatePackVertices(const float dtMillis, std::shared_ptr<scene::Scene> scene);
    void PreparePackVertexVelocities(std::shared_ptr<scene::Scene> scene);
    void CardPackShakeStep(std::shared_ptr<scene::Scene> scene);
    void CreateCardRewards(std::shared_ptr<scene::Scene> scene);
    void CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText, const size_t cardIndex, std::shared_ptr<scene::Scene> scene);
    void DestroyCardTooltip(std::shared_ptr<scene::Scene> scene);
    
private:
    enum class SceneState
    {
        PENDING_PACK_OPENING,
        PACK_ROTATING,
        PACK_SHAKING,
        PACK_EXPLODING,
        CARD_REWARDS_INSPECTION,
        LEAVING_SCENE
    };
    
    std::unique_ptr<AnimatedButton> mOpenButton;
    std::unique_ptr<AnimatedButton> mContinueButton;
    std::unique_ptr<CardTooltipController> mCardTooltipController;
    std::vector<std::shared_ptr<CardSoWrapper>> mCardRewards;
    std::vector<std::shared_ptr<scene::SceneObject>> mCardRewardFamilyStamps;
    std::vector<glm::vec3> mCardPackVertexVelocities;
    CardPackType mCardPackType;
    SceneState mSceneState;
    float mGoldenCardLightPosX;
    int mCardPackShakeStepsRemaining;
};

///------------------------------------------------------------------------------------------------

#endif /* CardPackRewardSceneLogicManager_h */
