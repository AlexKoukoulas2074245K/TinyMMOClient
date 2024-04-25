///------------------------------------------------------------------------------------------------
///  SceneManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneManager.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <engine/utils/PlatformMacros.h>
#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

///------------------------------------------------------------------------------------------------

namespace scene
{

///------------------------------------------------------------------------------------------------

static const std::string SCENE_DESCRIPTORS_PATH = "scene_descriptors/";
static const std::unordered_map<std::string, scene::SnapToEdgeBehavior> STRING_TO_SNAP_TO_EDGE_BEHAVIOR_MAP =
{
    { "none", scene::SnapToEdgeBehavior::NONE },
    { "snap_to_left_edge", scene::SnapToEdgeBehavior::SNAP_TO_LEFT_EDGE },
    { "snap_to_right_edge", scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE },
    { "snap_to_top_edge", scene::SnapToEdgeBehavior::SNAP_TO_TOP_EDGE },
    { "snap_to_bot_edge", scene::SnapToEdgeBehavior::SNAP_TO_BOT_EDGE }
};

///------------------------------------------------------------------------------------------------

std::shared_ptr<Scene> SceneManager::CreateScene(const strutils::StringId sceneName /* = strutils::StringId() */)
{
    mScenes.emplace_back(std::make_shared<Scene>(sceneName));
    return mScenes.back();
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<Scene> SceneManager::FindScene(const strutils::StringId& sceneName) const
{
    auto findIter = std::find_if(mScenes.begin(), mScenes.end(), [&](const std::shared_ptr<Scene>& scene)
    {
        return scene->GetName() == sceneName;
    });
    
    return findIter != mScenes.end() ? *findIter : nullptr;
}

///------------------------------------------------------------------------------------------------

void SceneManager::LoadPredefinedObjectsFromDescriptorForScene(std::shared_ptr<Scene> scene)
{
    if (scene->HasLoadedPredefinedObjects())
    {
        return;
    }
    
    scene->SetHasLoadedPredefinedObjects(true);
    
    auto sceneDescriptorPath = resources::ResourceLoadingService::RES_DATA_ROOT + SCENE_DESCRIPTORS_PATH + scene->GetName().GetString() + ".json";
    std::ifstream testFile(sceneDescriptorPath);
    if (!testFile.is_open())
    {
        return;
    }
    
    auto& resourceService = CoreSystemsEngine::GetInstance().GetResourceLoadingService();
    
    auto sceneDescriptorJsonResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(sceneDescriptorPath);
    const auto sceneDescriptorJson =  nlohmann::json::parse(resourceService.GetResource<resources::DataFileResource>(sceneDescriptorJsonResourceId).GetContents());
    
    for (const auto& childSceneJson: sceneDescriptorJson["children_scenes"])
    {
        const auto& childSceneName = strutils::StringId(childSceneJson.get<std::string>());
        auto childScene = FindScene(childSceneName);
        if (!childScene)
        {
            childScene = CoreSystemsEngine::GetInstance().GetSceneManager().CreateScene(childSceneName);
        }
        
        LoadPredefinedObjectsFromDescriptorForScene(childScene);
    }
    
    for (const auto& sceneObjectJson: sceneDescriptorJson["scene_objects"])
    {
        if (sceneObjectJson.count("tablet_only"))
        {
            if (sceneObjectJson["tablet_only"].get<bool>())
            {
#if defined(MOBILE_FLOW)
                if (!ios_utils::IsIPad())
                {
                    continue;
                }
#else
                continue;
#endif
            }
            else
            {
#if defined(MOBILE_FLOW)
                if (ios_utils::IsIPad())
                {
                    continue;
                }
#endif
            }
        }

        auto sceneObjectName = strutils::StringId(sceneObjectJson["name"].get<std::string>());
        assert (!scene->FindSceneObject(sceneObjectName));
        auto sceneObject = scene->CreateSceneObject(strutils::StringId(sceneObjectName));
        
        if (sceneObjectJson.count("texture"))
        {
            sceneObject->mTextureResourceId = resourceService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + sceneObjectJson["texture"].get<std::string>());
        }
        
        if (sceneObjectJson.count("effect_textures"))
        {
            int i = 0;
            for (const auto& effectTextureJson: sceneObjectJson["effect_textures"])
            {
                sceneObject->mEffectTextureResourceIds[i++] = resourceService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + effectTextureJson.get<std::string>());
            }
        }
        
        if (sceneObjectJson.count("shader"))
        {
            sceneObject->mShaderResourceId = resourceService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + sceneObjectJson["shader"].get<std::string>());
        }
        
        if (sceneObjectJson.count("position"))
        {
            sceneObject->mPosition = glm::vec3
            (
             sceneObjectJson["position"]["x"].get<float>(),
             sceneObjectJson["position"]["y"].get<float>(),
             sceneObjectJson["position"]["z"].get<float>()
             );
        }
        
        if (sceneObjectJson.count("scale"))
        {
            sceneObject->mScale = glm::vec3
            (
             sceneObjectJson["scale"]["x"].get<float>(),
             sceneObjectJson["scale"]["y"].get<float>(),
             sceneObjectJson["scale"]["z"].get<float>()
             );
        }
        
        if (sceneObjectJson.count("rotation"))
        {
            sceneObject->mRotation = glm::vec3
            (
             sceneObjectJson["rotation"]["x"].get<float>(),
             sceneObjectJson["rotation"]["y"].get<float>(),
             sceneObjectJson["rotation"]["z"].get<float>()
             );
        }
        
        if (sceneObjectJson.count("alpha"))
        {
            sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = sceneObjectJson["alpha"].get<float>();
        }
        
        if (sceneObjectJson.count("invisible"))
        {
            sceneObject->mInvisible = sceneObjectJson["invisible"].get<bool>();
        }
        
        if (sceneObjectJson.count("snap_to_edge"))
        {
            sceneObject->mSnapToEdgeBehavior = STRING_TO_SNAP_TO_EDGE_BEHAVIOR_MAP.at(sceneObjectJson["snap_to_edge"].get<std::string>());
        }
        
        if (sceneObjectJson.count("snap_to_edge_factor"))
        {
            sceneObject->mSnapToEdgeScaleOffsetFactor = sceneObjectJson["snap_to_edge_factor"].get<float>();
        }
        
        if (sceneObjectJson.count("uniform_floats"))
        {
            for (const auto& uniformFloatJson: sceneObjectJson["uniform_floats"])
            {
                sceneObject->mShaderFloatUniformValues[strutils::StringId(uniformFloatJson["name"].get<std::string>())] = uniformFloatJson["value"].get<float>();
            }
        }
        
        scene::TextSceneObjectData textData;
        if (sceneObjectJson.count("font"))
        {
            textData.mFontName = strutils::StringId(sceneObjectJson["font"].get<std::string>());
            
            if (sceneObjectJson.count("color"))
            {
                sceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME].r = sceneObjectJson["color"]["r"].get<float>();
                sceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME].g = sceneObjectJson["color"]["g"].get<float>();
                sceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME].b = sceneObjectJson["color"]["b"].get<float>();
            }
        }
        if (sceneObjectJson.count("text"))
        {
            textData.mText = sceneObjectJson["text"].get<std::string>();
        }
        
        if (!textData.mText.empty() || !textData.mFontName.isEmpty())
        {
            sceneObject->mSceneObjectTypeData = std::move(textData);
        }
    }
}

///------------------------------------------------------------------------------------------------

void SceneManager::SortSceneObjects(std::shared_ptr<Scene> scene)
{
    auto& sceneObjects = scene->GetSceneObjects();
    std::sort(sceneObjects.begin(), sceneObjects.end(), [&](const std::shared_ptr<scene::SceneObject>& lhs, const std::shared_ptr<scene::SceneObject>& rhs)
    {
        return lhs->mPosition.z < rhs->mPosition.z;
    });
}

///------------------------------------------------------------------------------------------------

void SceneManager::RemoveScene(const strutils::StringId& sceneName)
{
    auto findIter = std::find_if(mScenes.begin(), mScenes.end(), [&](const std::shared_ptr<Scene>& scene)
    {
        return scene->GetName() == sceneName;
    });
    if (findIter != mScenes.end())
    {
        for (auto sceneObject: (*findIter)->GetSceneObjects())
        {
            sceneObject->mScene = nullptr;
        }
        CollectTextureResourceIdCandidates(*findIter);
        mScenes.erase(findIter);
        UnloadUnusedTextures();
    }
}

///------------------------------------------------------------------------------------------------

void SceneManager::RepositionSceneToTheEnd(std::shared_ptr<Scene> sceneToReposition)
{
    auto findIter = std::find_if(mScenes.begin(), mScenes.end(), [&](const std::shared_ptr<Scene>& scene)
    {
        return scene->GetName() == sceneToReposition->GetName();
    });
    
    assert(findIter != mScenes.end());
    
    mScenes.erase(findIter);
    mScenes.push_back(sceneToReposition);
}

///------------------------------------------------------------------------------------------------

[[nodiscard]] std::size_t SceneManager::GetSceneCount() const { return mScenes.size(); }

///------------------------------------------------------------------------------------------------

const std::vector<std::shared_ptr<Scene>>& SceneManager::GetScenes() const { return mScenes; }

///------------------------------------------------------------------------------------------------

void SceneManager::CollectTextureResourceIdCandidates(std::shared_ptr<Scene> sceneToRemove)
{
    mTextureResourceCandidatesToRemove.clear();
    for (auto& sceneObject: sceneToRemove->GetSceneObjects())
    {
        // Particle textures should not be unloaded
        if (std::holds_alternative<scene::ParticleEmitterObjectData>(sceneObject->mSceneObjectTypeData))
        {
            continue;
        }
        
        mTextureResourceCandidatesToRemove.insert(sceneObject->mTextureResourceId);
        for (auto i = 0U; i < EFFECT_TEXTURES_COUNT; ++i)
        {
            mTextureResourceCandidatesToRemove.insert(sceneObject->mEffectTextureResourceIds[i]);
        }
        sceneObject->mScene = nullptr;
    }
}

///------------------------------------------------------------------------------------------------

void SceneManager::UnloadUnusedTextures()
{
    auto& resourceService = CoreSystemsEngine::GetInstance().GetResourceLoadingService();
    for (auto resourceId: mTextureResourceCandidatesToRemove)
    {
        bool found = false;
        for (auto& scene: mScenes)
        {
            for (auto& sceneObject: scene->GetSceneObjects())
            {
                if (sceneObject->mTextureResourceId == resourceId)
                {
                    found = true;
                    break;
                }
                
                for (auto i = 0U; i < EFFECT_TEXTURES_COUNT; ++i)
                {
                    if (sceneObject->mEffectTextureResourceIds[i] == resourceId)
                    {
                        found = true;
                        break;
                    }
                }
            }
            
            if (found)
            {
                break;
            }
        }
        
        if (!found)
        {
            resourceService.UnloadResource(resourceId);
        }
    }
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
