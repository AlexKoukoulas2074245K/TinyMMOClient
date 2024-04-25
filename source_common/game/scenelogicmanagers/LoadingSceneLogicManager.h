///------------------------------------------------------------------------------------------------
///  LoadingSceneLogicManager.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 11/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef LoadingSceneLogicManager_h
#define LoadingSceneLogicManager_h

///------------------------------------------------------------------------------------------------

#include <atomic>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>

///------------------------------------------------------------------------------------------------

class LoadingSceneLogicManager final: public ISceneLogicManager, public events::IListener
{
public:
    const std::vector<strutils::StringId>& VGetApplicableSceneNames() const override;
    
    void VInitSceneCamera(std::shared_ptr<scene::Scene> scene) override;
    void VInitScene(std::shared_ptr<scene::Scene> scene) override;
    void VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene) override;
    void VDestroyScene(std::shared_ptr<scene::Scene> scene) override;
    std::shared_ptr<GuiObjectManager> VGetGuiObjectManager() override;
    
private:
    void SetLoadingProgress(const int progressPercent);
    void OnLoadingProgressPrefixTextOverride(const events::LoadingProgressPrefixTextOverrideEvent&);
    
private:
    int mTotalLoadingJobCount;
    std::string mLoadingProgressPrefixText;
};

///------------------------------------------------------------------------------------------------

#endif /* LoadingSceneLogicManager_h */
