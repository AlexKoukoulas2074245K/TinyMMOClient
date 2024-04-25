///------------------------------------------------------------------------------------------------
///  SceneManager.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef SceneManager_h
#define SceneManager_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <memory>
#include <unordered_set>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene
{

///------------------------------------------------------------------------------------------------

class Scene;
class SceneManager final
{
public:
    [[nodiscard]] std::shared_ptr<Scene> CreateScene(const strutils::StringId sceneName = strutils::StringId());
    [[nodiscard]] std::shared_ptr<Scene> FindScene(const strutils::StringId& sceneName) const;
    
    void LoadPredefinedObjectsFromDescriptorForScene(std::shared_ptr<Scene> scene);
    void SortSceneObjects(std::shared_ptr<Scene> scene);
    void RemoveScene(const strutils::StringId& sceneName);
    void RepositionSceneToTheEnd(std::shared_ptr<Scene> scene);
    
    [[nodiscard]] std::size_t GetSceneCount() const;
    [[nodiscard]] const std::vector<std::shared_ptr<Scene>>& GetScenes() const;
    
private:
    void CollectTextureResourceIdCandidates(std::shared_ptr<Scene> sceneToRemove);
    void UnloadUnusedTextures();
    
private:
    std::vector<std::shared_ptr<Scene>> mScenes;
    std::unordered_set<resources::ResourceId> mTextureResourceCandidatesToRemove;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* SceneManager_h */
