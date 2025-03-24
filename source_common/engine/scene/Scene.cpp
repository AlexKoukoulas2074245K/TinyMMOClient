///------------------------------------------------------------------------------------------------
///  Scene.cpp                                                                                        
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 25/09/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/scene/Scene.h>
#include <engine/resloading/MeshResource.h>
#include <engine/resloading/ResourceLoadingService.h>

///------------------------------------------------------------------------------------------------

namespace scene
{

///------------------------------------------------------------------------------------------------

Scene::Scene(const strutils::StringId& sceneName)
    : mSceneName(sceneName)
    , mUpdateTimeSpeedFactor(1.0f)
    , mLoaded(false)
    , mHasLoadedPredefinedObjects(false)
{
}

///------------------------------------------------------------------------------------------------

Scene::~Scene()
{
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<SceneObject> Scene::CreateSceneObject(const strutils::StringId sceneObjectName /* = strutils::StringId() */)
{
    auto newSceneObject = std::make_shared<SceneObject>();
    newSceneObject->mScene = this;
    newSceneObject->mName = sceneObjectName;
    mSceneObjects.push_back(newSceneObject);
    return newSceneObject;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<SceneObject> Scene::FindSceneObject(const strutils::StringId& sceneObjectName) const
{
    auto findIter = std::find_if(mSceneObjects.begin(), mSceneObjects.end(), [&](const std::shared_ptr<SceneObject>& sceneObject)
    {
        return sceneObject->mName == sceneObjectName;
    });
    
    return findIter != mSceneObjects.end() ? *findIter : nullptr;
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<SceneObject>> Scene::FindSceneObjectsWhoseNameStartsWith(const std::string& sceneObjectNamePrefix) const
{
    std::vector<std::shared_ptr<SceneObject>> result;
    for (auto& sceneObject: mSceneObjects)
    {
        if (strutils::StringStartsWith(sceneObject->mName.GetString(), sceneObjectNamePrefix))
        {
            result.emplace_back(sceneObject);
        }
    }
    
    return result;
}

///------------------------------------------------------------------------------------------------

void Scene::RecalculatePositionOfEdgeSnappingSceneObject(std::shared_ptr<SceneObject> sceneObject, const math::Frustum& cameraFrustum)
{
    static const float positionIncrements = 0.0001f;
    
    if (sceneObject->mSnapToEdgeBehavior == SnapToEdgeBehavior::NONE)
    {
        return;
    }
    
    auto sceneObjectMeshDimensions = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::MeshResource>(sceneObject->mMeshResourceId).GetDimensions();
    sceneObjectMeshDimensions.z = 0.0f;
    
    int breachedSideIndex = 0;
    
    // Pull inside frustum
    while (!math::IsMeshFullyInsideFrustum(sceneObject->mPosition, sceneObject->mScale, sceneObjectMeshDimensions, cameraFrustum, breachedSideIndex))
    {
        // Breach on left side
        if (breachedSideIndex == 0)
        {
            sceneObject->mPosition.x += positionIncrements;
        }
        // Breach on right side
        else if (breachedSideIndex == 1)
        {
            sceneObject->mPosition.x -= positionIncrements;
        }
        // Breach on bottom side
        else if (breachedSideIndex == 2)
        {
            sceneObject->mPosition.y += positionIncrements;
        }
        // Breach on top side
        else
        {
            sceneObject->mPosition.y -= positionIncrements;
        }
    }
    
    // Push to respective edge
    switch (sceneObject->mSnapToEdgeBehavior)
    {
        case SnapToEdgeBehavior::SNAP_TO_LEFT_EDGE:
        {
            while (math::IsMeshFullyInsideFrustum(sceneObject->mPosition, sceneObject->mScale, sceneObjectMeshDimensions, cameraFrustum, breachedSideIndex))
            {
                sceneObject->mPosition.x -= positionIncrements;
            }
            sceneObject->mPosition.x += sceneObject->mScale.x * sceneObject->mSnapToEdgeScaleOffsetFactor;
        } break;
            
        case SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE:
        {
            while (math::IsMeshFullyInsideFrustum(sceneObject->mPosition, sceneObject->mScale, sceneObjectMeshDimensions, cameraFrustum, breachedSideIndex))
            {
                sceneObject->mPosition.x += positionIncrements;
            }
            sceneObject->mPosition.x -= sceneObject->mScale.x * sceneObject->mSnapToEdgeScaleOffsetFactor;
        } break;
            
        case SnapToEdgeBehavior::SNAP_TO_TOP_EDGE:
        {
            while (math::IsMeshFullyInsideFrustum(sceneObject->mPosition, sceneObject->mScale, sceneObjectMeshDimensions, cameraFrustum, breachedSideIndex))
            {
                sceneObject->mPosition.y += positionIncrements;
            }
            sceneObject->mPosition.y -= sceneObject->mScale.y * sceneObject->mSnapToEdgeScaleOffsetFactor;
        } break;
            
        case SnapToEdgeBehavior::SNAP_TO_BOT_EDGE:
        {
            while (math::IsMeshFullyInsideFrustum(sceneObject->mPosition, sceneObject->mScale, sceneObjectMeshDimensions, cameraFrustum, breachedSideIndex))
            {
                sceneObject->mPosition.y -= positionIncrements;
            }
            sceneObject->mPosition.y += sceneObject->mScale.y * sceneObject->mSnapToEdgeScaleOffsetFactor;
        } break;
            
        default: break;
    }
}

///------------------------------------------------------------------------------------------------

void Scene::RecalculatePositionOfEdgeSnappingSceneObjects()
{
    const auto& frustum = mCamera.CalculateFrustum();
    
    for (auto& sceneObject: mSceneObjects)
    {
        RecalculatePositionOfEdgeSnappingSceneObject(sceneObject, frustum);
    }
}

///------------------------------------------------------------------------------------------------

void Scene::RemoveSceneObject(const strutils::StringId& sceneObjectName)
{
    if (mSceneObjects.empty())
    {
        return;
    }
    
    auto findIter = std::find_if(mSceneObjects.begin(), mSceneObjects.end(), [&](const std::shared_ptr<SceneObject>& sceneObject)
    {
        return sceneObject->mName == sceneObjectName;
    });
    if (findIter != mSceneObjects.end())
    {
        mSceneObjects.erase(findIter);
    }
}

///------------------------------------------------------------------------------------------------

void Scene::RemoveAllSceneObjectsWithName(const strutils::StringId& sceneObjectName)
{
    while (true)
    {
        const auto sizeBefore = GetSceneObjectCount();
        RemoveSceneObject(sceneObjectName);
        if (GetSceneObjectCount() == sizeBefore)
        {
            break;
        }
    }
}

///------------------------------------------------------------------------------------------------

void Scene::RemoveAllSceneObjectsButTheOnesNamed(const std::unordered_set<strutils::StringId, strutils::StringIdHasher>& sceneObjectNames)
{
    for (auto iter = mSceneObjects.begin(); iter != mSceneObjects.end();)
    {
        if (sceneObjectNames.count((*iter)->mName) == 0)
        {
            iter = mSceneObjects.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

///------------------------------------------------------------------------------------------------

void Scene::RemoveAllParticleEffects()
{
    for (auto iter = mSceneObjects.begin(); iter != mSceneObjects.end();)
    {
        if (std::holds_alternative<scene::ParticleEmitterObjectData>((*iter)->mSceneObjectTypeData))
        {
            iter = mSceneObjects.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

///------------------------------------------------------------------------------------------------

std::size_t Scene::GetSceneObjectCount() const { return mSceneObjects.size(); }

///------------------------------------------------------------------------------------------------

const std::vector<std::shared_ptr<SceneObject>>& Scene::GetSceneObjects() const { return mSceneObjects; }

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<SceneObject>>& Scene::GetSceneObjects() { return mSceneObjects;}

///------------------------------------------------------------------------------------------------

rendering::Camera& Scene::GetCamera() { return mCamera; }

///------------------------------------------------------------------------------------------------

const rendering::Camera& Scene::GetCamera() const { return mCamera; }

///------------------------------------------------------------------------------------------------

const strutils::StringId& Scene::GetName() const { return mSceneName; }

///------------------------------------------------------------------------------------------------

float Scene::GetUpdateTimeSpeedFactor() const { return mUpdateTimeSpeedFactor; }

///------------------------------------------------------------------------------------------------

float& Scene::GetUpdateTimeSpeedFactor() { return mUpdateTimeSpeedFactor; }

///------------------------------------------------------------------------------------------------

bool Scene::IsLoaded() const { return mLoaded; }

///------------------------------------------------------------------------------------------------

bool Scene::HasLoadedPredefinedObjects() const { return mHasLoadedPredefinedObjects; }

///------------------------------------------------------------------------------------------------

void Scene::SetLoaded(const bool loaded) { mLoaded = loaded;  if (mLoaded) SDL_RaiseWindow(&CoreSystemsEngine::GetInstance().GetContextWindow()); }

///------------------------------------------------------------------------------------------------

void Scene::SetHasLoadedPredefinedObjects(const bool hasLoadedPredefinedObjects) { mHasLoadedPredefinedObjects = hasLoadedPredefinedObjects; }

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
