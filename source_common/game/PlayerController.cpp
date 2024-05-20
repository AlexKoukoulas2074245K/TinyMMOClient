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
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneManager.h>
#include <engine/utils/MathUtils.h>
#include <engine/CoreSystemsEngine.h>
#include <map/MapConstants.h>
#include <map/GlobalMapDataRepository.h>
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>
#include <SDL_surface.h>
#include <imgui/imgui.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId NAVMAP_DEBUG_SCENE_OBJECT_NAME = strutils::StringId("navmap_debug");
static const float MAP_TRANSITION_THRESHOLD = 0.03f;

///------------------------------------------------------------------------------------------------

PlayerController::PlayerController(const strutils::StringId& mapName)
    : mCurrentMapName(mapName)
{
}

///------------------------------------------------------------------------------------------------

const strutils::StringId& PlayerController::GetCurrentMapName() const
{
    return mCurrentMapName;
}

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
    
    if (inputStateManager.VKeyPressed(input::Key::A))
    {
        impulseVector.x = -1.0f;
    }
    else if (inputStateManager.VKeyPressed(input::Key::D))
    {
        impulseVector.x = 1.0f;
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
        
        const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
        const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMapName);
        
        objectData.objectPosition += objectData.objectVelocity;
        playerSceneObject->mPosition += objectData.objectVelocity;
        playerNameSceneObject->mPosition += objectData.objectVelocity;
        
        auto nextNavmapCoords = mCurrentNavmap->GetNavmapCoord(playerSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
        auto nextNavmapTileType = mCurrentNavmap->GetNavmapTileAt(nextNavmapCoords);
        
        // Map Transition
        if (nextNavmapTileType == networking::NavmapTileType::VOID)
        {
            strutils::StringId nextMapName;
            
            // Determine map change direction
            if (playerSceneObject->mPosition.x > currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE + (currentMapDefinition.mMapDimensions.x * game_constants::MAP_RENDERED_SCALE)/2.0f - MAP_TRANSITION_THRESHOLD)
            {
                nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMapName, MapConnectionDirection::EAST);
            }
            else if (playerSceneObject->mPosition.x < currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE - (currentMapDefinition.mMapDimensions.x * game_constants::MAP_RENDERED_SCALE)/2.0f + MAP_TRANSITION_THRESHOLD)
            {
                nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMapName, MapConnectionDirection::WEST);
            }
            else if (playerSceneObject->mPosition.y > currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE + (currentMapDefinition.mMapDimensions.y * game_constants::MAP_RENDERED_SCALE)/2.0f - MAP_TRANSITION_THRESHOLD)
            {
                nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMapName, MapConnectionDirection::NORTH);
            }
            else if (playerSceneObject->mPosition.y < currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE - (currentMapDefinition.mMapDimensions.y * game_constants::MAP_RENDERED_SCALE)/2.0f + MAP_TRANSITION_THRESHOLD)
            {
                nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMapName, MapConnectionDirection::SOUTH);
            }
            
            if (nextMapName != map_constants::NO_CONNECTION_NAME)
            {
                // Give a further push to the player to bring them concretely
                // into the next navmap
                objectData.objectPosition += objectData.objectVelocity;
                playerSceneObject->mPosition += objectData.objectVelocity;
                playerNameSceneObject->mPosition += objectData.objectVelocity;

                mCurrentMapName = nextMapName;
                objectData.objectCurrentMapName = nextMapName;
                
                events::EventSystem::GetInstance().DispatchEvent<events::MapChangeEvent>(mCurrentMapName);
                
                if (scene.FindSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME))
                {
                    HideNavmapDebugView();
                    ShowNavmapDebugView();
                }
                
                TerrainCollisionHandlingPostMapChange(objectData, playerSceneObject, playerNameSceneObject, impulseVector, dtMillis);
            }
        }

        TerrainCollisionHandling(objectData, playerSceneObject, playerNameSceneObject, impulseVector, dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void PlayerController::CreateDebugWidgets()
{
    static bool sNavmapDebugMode = false;
   
    ImGui::Begin("Movement Debug", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    if (ImGui::Checkbox("Navmap Debug Mode", &sNavmapDebugMode))
    {
        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
        
        if (sNavmapDebugMode)
        {
            ShowNavmapDebugView();
        }
        else
        {
            HideNavmapDebugView();
        }
    }
    ImGui::End();
}

///------------------------------------------------------------------------------------------------

void PlayerController::ShowNavmapDebugView()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
    
    const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMapName);
    
    auto navmapSceneObject = scene->CreateSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    navmapSceneObject->mPosition.x = currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE;
    navmapSceneObject->mPosition.y = currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE;
    navmapSceneObject->mPosition.z = 15.0f;
    navmapSceneObject->mScale *= game_constants::MAP_RENDERED_SCALE;
    
    auto navmapSurface = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::ImageSurfaceResource>(mCurrentNavmapResourceId).GetSurface();
    
    GLuint glTextureId; int mode;
    rendering::CreateGLTextureFromSurface(navmapSurface, glTextureId, mode, true);
    
    navmapSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().AddDynamicallyCreatedTextureResourceId("debug_navmap", glTextureId, map_constants::CLIENT_NAVMAP_IMAGE_SIZE, map_constants::CLIENT_NAVMAP_IMAGE_SIZE);
    navmapSceneObject->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.5f;
}

///------------------------------------------------------------------------------------------------

void PlayerController::HideNavmapDebugView()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
    
    auto navmapSceneObject = scene->FindSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    systemsEngine.GetResourceLoadingService().UnloadResource(navmapSceneObject->mTextureResourceId);
    scene->RemoveSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
}

///------------------------------------------------------------------------------------------------

void PlayerController::SetNavmap(const resources::ResourceId navmapImageResourceId, std::shared_ptr<networking::Navmap> navmap)
{
    mCurrentNavmapResourceId = navmapImageResourceId;
    mCurrentNavmap = navmap;
}

///------------------------------------------------------------------------------------------------

void PlayerController::TerrainCollisionHandlingPostMapChange(networking::WorldObjectData& objectData, std::shared_ptr<scene::SceneObject> playerSceneObject, std::shared_ptr<scene::SceneObject> playerNameSceneObject, glm::vec3 impulseVector, const float dtMillis)
{
    const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMapName);
    
    auto nextNavmapCoords = mCurrentNavmap->GetNavmapCoord(playerSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
    auto nextNavmapTileType = mCurrentNavmap->GetNavmapTileAt(nextNavmapCoords);
    
    if (nextNavmapTileType != networking::NavmapTileType::EMPTY)
    {
        objectData.objectPosition.x -= objectData.objectVelocity.x;
        playerSceneObject->mPosition.x -= objectData.objectVelocity.x;
        playerNameSceneObject->mPosition.x -= objectData.objectVelocity.x;
        
        nextNavmapCoords = mCurrentNavmap->GetNavmapCoord(playerSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
        nextNavmapTileType = mCurrentNavmap->GetNavmapTileAt(nextNavmapCoords);
    }
    
    if (nextNavmapTileType != networking::NavmapTileType::EMPTY)
    {
        objectData.objectPosition.y -= objectData.objectVelocity.y;
        playerSceneObject->mPosition.y -= objectData.objectVelocity.y;
        playerNameSceneObject->mPosition.y -= objectData.objectVelocity.y;
    }
}

///------------------------------------------------------------------------------------------------

void PlayerController::TerrainCollisionHandling(networking::WorldObjectData& objectData, std::shared_ptr<scene::SceneObject> playerSceneObject, std::shared_ptr<scene::SceneObject> playerNameSceneObject, glm::vec3 impulseVector, const float dtMillis)
{
    const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMapName);
    
    auto nextNavmapCoords = mCurrentNavmap->GetNavmapCoord(playerSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
    auto nextNavmapTileType = mCurrentNavmap->GetNavmapTileAt(nextNavmapCoords);
    
    // Solid
    if (nextNavmapTileType != networking::NavmapTileType::EMPTY)
    {
        // Backtrack
        objectData.objectPosition -= objectData.objectVelocity;
        playerSceneObject->mPosition -= objectData.objectVelocity;
        playerNameSceneObject->mPosition -= objectData.objectVelocity;
        
        // Try each direction sequentially
        
        // First horizontally
        auto originalImpulseVector = impulseVector;
        
        if (math::Abs(impulseVector.x) > 0.0f)
        {
            impulseVector.y = 0.0f;
            objectData.objectVelocity = glm::normalize(impulseVector) * game_constants::PLAYER_SPEED * dtMillis;
            
            objectData.objectPosition.x += objectData.objectVelocity.x;
            playerSceneObject->mPosition.x += objectData.objectVelocity.x;
            playerNameSceneObject->mPosition.x += objectData.objectVelocity.x;

            auto nextHorizontalNavmapCoords = mCurrentNavmap->GetNavmapCoord(playerSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
            auto nextHorizontalNavmapTileType = mCurrentNavmap->GetNavmapTileAt(nextHorizontalNavmapCoords);
            
            if (nextHorizontalNavmapTileType != networking::NavmapTileType::EMPTY)
            {
                objectData.objectPosition.x -= objectData.objectVelocity.x;
                playerSceneObject->mPosition.x -= objectData.objectVelocity.x;
                playerNameSceneObject->mPosition.x -= objectData.objectVelocity.x;
            }
        }
        
        // Then vertically
        impulseVector = originalImpulseVector;
        
        if (math::Abs(impulseVector.y) > 0.0f)
        {
            impulseVector.x = 0.0f;
            objectData.objectVelocity = glm::normalize(impulseVector) * game_constants::PLAYER_SPEED * dtMillis;
            
            objectData.objectPosition.y += objectData.objectVelocity.y;
            playerSceneObject->mPosition.y += objectData.objectVelocity.y;
            playerNameSceneObject->mPosition.y += objectData.objectVelocity.y;
            
            auto nextVerticalNavmapCoords = mCurrentNavmap->GetNavmapCoord(playerSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
            auto nextVerticalNavmapTileType = mCurrentNavmap->GetNavmapTileAt(nextVerticalNavmapCoords);
                
            if (nextVerticalNavmapTileType != networking::NavmapTileType::EMPTY)
            {
                objectData.objectPosition.y -= objectData.objectVelocity.y;
                playerSceneObject->mPosition.y -= objectData.objectVelocity.y;
                playerNameSceneObject->mPosition.y -= objectData.objectVelocity.y;
            }
        }
    }
}

///------------------------------------------------------------------------------------------------
