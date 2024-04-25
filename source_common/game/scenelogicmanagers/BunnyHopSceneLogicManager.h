///------------------------------------------------------------------------------------------------
///  BunnyHopSceneLogicManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/03/2024
///------------------------------------------------------------------------------------------------

#ifndef BunnyHopSceneLogicManager_h
#define BunnyHopSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <atomic>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>

///------------------------------------------------------------------------------------------------

class BunnyHopSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    bool mFinished;
};

///------------------------------------------------------------------------------------------------

#endif /* BunnyHopSceneLogicManager_h */
