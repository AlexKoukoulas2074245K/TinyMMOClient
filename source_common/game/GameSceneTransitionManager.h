///------------------------------------------------------------------------------------------------
///  GameSceneTransitionManager.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 04/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameSceneTransitionManager_h
#define GameSceneTransitionManager_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <game/GameSceneTransitionTypes.h>
#include <game/scenelogicmanagers/ISceneLogicManager.h>
#include <stack>
#include <unordered_map>
#include <vector>

///------------------------------------------------------------------------------------------------

class GameSceneTransitionManager final
{
    friend class Game;
public:
    GameSceneTransitionManager();
    
    template <typename SceneLogicManagerT>
    void RegisterSceneLogicManager()
    {
        static_assert(std::is_default_constructible<SceneLogicManagerT>());
        static_assert(std::is_base_of<ISceneLogicManager, SceneLogicManagerT>());
        
        SceneLogicManagerEntry entry;
        entry.mSceneLogicManager = std::make_unique<SceneLogicManagerT>();
        entry.mSceneLogicManager->mGameSceneTransitionManager = this;
        
        for (const auto& applicableSceneName: entry.mSceneLogicManager->VGetApplicableSceneNames())
        {
            entry.mSceneInitStatusMap[applicableSceneName] = false;
        }
        
        mRegisteredSceneLogicManagers.emplace_back(std::move(entry));
    }
    
    ISceneLogicManager* GetActiveSceneLogicManager();
    ISceneLogicManager* GetSceneLogicManagerResponsibleForScene(const strutils::StringId& sceneName);
    
    void Update(const float dtMillis);
    void ChangeToScene
    (
        const strutils::StringId& sceneName,
        const SceneChangeType sceneChangeType,
        const PreviousSceneDestructionType previousSceneDestructionType
    );
    void PopModalScene();
    void DisableTransitionAnimations();
    
private:
    struct SceneLogicManagerEntry
    {
        std::unique_ptr<ISceneLogicManager> mSceneLogicManager = nullptr;
        std::unordered_map<strutils::StringId, bool, strutils::StringIdHasher> mSceneInitStatusMap;
    };
    
    struct ActiveSceneEntry
    {
        ISceneLogicManager* mActiveSceneLogicManager = nullptr;
        strutils::StringId mActiveSceneName = strutils::StringId();
        bool isModal = false;
    };
    
private:
    const std::vector<SceneLogicManagerEntry>& GetRegisteredSceneLogicManagers() const;
    const std::stack<ActiveSceneEntry> GetActiveSceneStack() const;
    
    void InitializeActiveSceneLogicManager(const SceneChangeType sceneChangeType);
    void DestroyActiveSceneLogicManager();
    
private:
    std::vector<SceneLogicManagerEntry> mRegisteredSceneLogicManagers;
    std::stack<ActiveSceneEntry> mActiveSceneStack;
    float mLoadingScreenMinDelaySecs;
    bool mFirstTimeLoadingScreenMaxAlpha;
    bool mTransitionAnimationsDisabled;
};

///------------------------------------------------------------------------------------------------

#endif /* GameSceneTransitionManager_h */
