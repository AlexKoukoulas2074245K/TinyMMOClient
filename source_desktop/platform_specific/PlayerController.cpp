///------------------------------------------------------------------------------------------------
///  PlayerController.cpp
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 09/05/2024
///------------------------------------------------------------------------------------------------

#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/PlayerController.h>
#include <engine/input/IInputStateManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/utils/MathUtils.h>
#include <engine/CoreSystemsEngine.h>
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>
#include <SDL_surface.h>

///------------------------------------------------------------------------------------------------

extern SDL_Surface* sNavmapSurface;
extern glm::vec3 GetRGBAt(SDL_Surface* surface, const int x, const int y);

///------------------------------------------------------------------------------------------------

void PlayerController::Update(const float dtMillis, const strutils::StringId& playerName, networking::WorldObjectData& objectData, scene::Scene& scene)
{
    auto playerSceneObject = scene.FindSceneObject(playerName);
    auto playerNameSceneObject = scene.FindSceneObject(strutils::StringId(playerName.GetString() + "_name"));
    
    assert(playerSceneObject);
    assert(playerNameSceneObject);
    
    auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    glm::vec3 impulseVector = {};
    if (inputStateManager.VKeyPressed(input::Key::W))
    {
        impulseVector.y = 1.0f;
    }
    else if (inputStateManager.VKeyPressed(input::Key::S))
    {
        impulseVector.y = -1.0f;
    }
    else
    {
        impulseVector.y = 0.0f;
    }
    
    if (inputStateManager.VKeyPressed(input::Key::A))
    {
        impulseVector.x = -1.0f;
    }
    else if (inputStateManager.VKeyPressed(input::Key::D))
    {
        impulseVector.x = 1.0f;
    }
    else
    {
        impulseVector.x = 0.0f;
    }
    
    if (inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
    {
        auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene.GetCamera().GetViewMatrix(), scene.GetCamera().GetProjMatrix());
        
        networking::ThrowRangedWeaponRequest throwRangedWeaponRequest;
        throwRangedWeaponRequest.playerId = objectData.objectId;
        throwRangedWeaponRequest.targetPosition = glm::vec3(worldTouchPos.x, worldTouchPos.y, objectData.objectPosition.z);
        
        events::EventSystem::GetInstance().DispatchEvent<events::SendNetworkMessageEvent>(throwRangedWeaponRequest.SerializeToJson(), networking::MessageType::CS_THROW_RANGED_WEAPON, true);
    }
    
    objectData.objectVelocity = {};
    auto length = glm::length(impulseVector);
    if (length > 0.0f)
    {
        objectData.objectVelocity = glm::normalize(impulseVector) * game_constants::PLAYER_SPEED * dtMillis;
        
        auto navmapCoords = glm::ivec2(static_cast<int>((objectData.objectPosition.x/game_constants::MAP_SCALE + 0.5f) * sNavmapSurface->w), static_cast<int>((1.0f - (objectData.objectPosition.y/game_constants::MAP_SCALE + 0.5f)) * sNavmapSurface->h));
        auto navmapColor = GetRGBAt(sNavmapSurface, navmapCoords.x, navmapCoords.y);
        
        objectData.objectVelocity *= navmapColor.r * navmapColor.r;
        objectData.objectPosition += objectData.objectVelocity;
        
        playerSceneObject->mPosition += objectData.objectVelocity;
        playerNameSceneObject->mPosition += objectData.objectVelocity;
    }
}

///------------------------------------------------------------------------------------------------
