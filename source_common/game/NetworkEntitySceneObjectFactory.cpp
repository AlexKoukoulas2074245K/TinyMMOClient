///------------------------------------------------------------------------------------------------
///  NetworkEntitySceneObjectFactory.cpp
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 05/02/2026
///------------------------------------------------------------------------------------------------

#include <game/NetworkEntitySceneObjectFactory.h>
#include <game/GameCommon.h>
#include <game/GameConstants.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/Scene.h>
#include <engine/utils/Logging.h>
#include <map/MapConstants.h>
#include <net_common/NetworkCommon.h>

///------------------------------------------------------------------------------------------------

void NetworkEntitySceneObjectFactory::CreateSceneObjects(const network::ObjectData& objectData, const bool collidersVisible, std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects)
{
    auto& resService = CoreSystemsEngine::GetInstance().GetResourceLoadingService();
    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
    auto sceneObjectName = GetSceneObjectNameId(objectData.objectId);

    auto sceneObject = scene->FindSceneObject(sceneObjectName);
    if (sceneObject)
    {
        logging::Log(logging::LogType::WARNING, "Attempted to re-create pre-existing object %s", sceneObjectName.GetString().c_str());
    }
    else
    {
        sceneObject = scene->CreateSceneObject(sceneObjectName);
        sceneObjects.push_back(sceneObject);

        switch (objectData.objectType)
        {
            case network::ObjectType::PLAYER:
            {
                sceneObject->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/anims/player_running/core.png");
                sceneObject->mShaderResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "player.vs");
                sceneObject->mShaderBoolUniformValues[IS_TEXTURE_SHEET_UNIFORM_NAME] = true;
                sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                sceneObject->mScale = glm::vec3(objectData.objectScale);
            } break;
            
            case network::ObjectType::ATTACK:
            {
                if (objectData.attackType == network::AttackType::PROJECTILE && objectData.projectileType == network::ProjectileType::FIREBALL)
                {
                    sceneObject->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/fireball_fx.png");
                    sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                    sceneObject->mScale = glm::vec3(objectData.objectScale);
                }
                else if (objectData.attackType == network::AttackType::MELEE)
                {
                    sceneObject->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/anims/melee_slash_001/core.png");
                    sceneObject->mShaderBoolUniformValues[IS_TEXTURE_SHEET_UNIFORM_NAME] = true;
                    sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                    sceneObject->mScale = glm::vec3(objectData.objectScale);
                }
            } break;

            case network::ObjectType::NPC:
            case network::ObjectType::STATIC:
            {
                assert(false);
            }
        }
        
        // IF DEBUG
        auto colliderSceneObjectName = strutils::StringId(GetSceneObjectName(objectData.objectId) + "-collider");
        auto colliderSceneObject = scene->CreateSceneObject(colliderSceneObjectName);
        
        switch (objectData.colliderData.colliderType)
        {
            case network::ColliderType::CIRCLE:
            {
                colliderSceneObject->mTextureResourceId = resService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "debug/debug_circle.png");
            } break;
            
            case network::ColliderType::RECTANGLE:
            {
                
            } break;
        }

        colliderSceneObject->mScale = glm::vec3(objectData.colliderData.colliderRelativeDimentions.x, objectData.colliderData.colliderRelativeDimentions.y, 1.0f);
        colliderSceneObject->mScale *= sceneObjects.front()->mScale;
        colliderSceneObject->mPosition = objectData.position;
        colliderSceneObject->mPosition.z = map_constants::TILE_NAVMAP_LAYER_Z;
        colliderSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.5f;
        colliderSceneObject->mInvisible = !collidersVisible;
        sceneObjects.push_back(colliderSceneObject);
    }
}

///------------------------------------------------------------------------------------------------
