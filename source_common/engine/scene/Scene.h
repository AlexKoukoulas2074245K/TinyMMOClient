///------------------------------------------------------------------------------------------------
///  Scene.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 25/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef Scene_h
#define Scene_h

///------------------------------------------------------------------------------------------------

#include <engine/rendering/Camera.h>
#include <engine/scene/SceneObject.h>
#include <engine/utils/StringUtils.h>
#include <memory>
#include <unordered_set>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene
{

///------------------------------------------------------------------------------------------------

struct SceneObjectComparator
{
public:
    bool operator () (const std::shared_ptr<scene::SceneObject>& lhs, const std::shared_ptr<scene::SceneObject>& rhs) const
    {
        return lhs->mPosition.z > rhs->mPosition.z;
    }
};

///------------------------------------------------------------------------------------------------

struct SceneObject;
class Scene final
{
public:
    Scene(const strutils::StringId& sceneName);
    ~Scene();
    
    [[nodiscard]] std::shared_ptr<SceneObject> CreateSceneObject(const strutils::StringId sceneObjectName = strutils::StringId());
    [[nodiscard]] std::shared_ptr<SceneObject> FindSceneObject(const strutils::StringId& sceneObjectName) const ;
    [[nodiscard]] std::vector<std::shared_ptr<SceneObject>> FindSceneObjectsWhoseNameStartsWith(const std::string& sceneObjectNamePrefix) const;
    
    void RecalculatePositionOfEdgeSnappingSceneObject(std::shared_ptr<SceneObject> sceneObject, const math::Frustum& cameraFrustum);
    void RecalculatePositionOfEdgeSnappingSceneObjects();
    void RemoveSceneObject(const strutils::StringId& sceneObjectName);
    void RemoveAllSceneObjectsWithName(const strutils::StringId& sceneObjectName);
    void RemoveAllSceneObjectsWithNameEndingIn(const std::string& sceneObjectNamePostfix);
    void RemoveAllSceneObjectsWithNameStartingWith(const std::string& sceneObjectNamePrefix);
    void RemoveAllSceneObjectsButTheOnesNamed(const std::unordered_set<strutils::StringId, strutils::StringIdHasher>& sceneObjectNames);
    void RemoveAllParticleEffects();
    
    [[nodiscard]] std::size_t GetSceneObjectCount() const;
    [[nodiscard]] const std::vector<std::shared_ptr<SceneObject>>& GetSceneObjects() const;
    [[nodiscard]] std::vector<std::shared_ptr<SceneObject>>& GetSceneObjects();
    [[nodiscard]] rendering::Camera& GetCamera();
    [[nodiscard]] const rendering::Camera& GetCamera() const;
    [[nodiscard]] const strutils::StringId& GetName() const;
    [[nodiscard]] float GetUpdateTimeSpeedFactor() const;
    [[nodiscard]] float& GetUpdateTimeSpeedFactor();
    [[nodiscard]] bool IsLoaded() const;
    [[nodiscard]] bool HasLoadedPredefinedObjects() const;
    
    void SetLoaded(const bool loaded);
    void SetHasLoadedPredefinedObjects(const bool hasLoadedPredefinedObjects);
    
private:
    const strutils::StringId mSceneName;
    std::vector<std::shared_ptr<SceneObject>> mSceneObjects;
    rendering::Camera mCamera;
    float mUpdateTimeSpeedFactor;
    bool mLoaded;
    bool mHasLoadedPredefinedObjects;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* Scene_h */
