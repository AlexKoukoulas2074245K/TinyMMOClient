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
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneManager.h>
#include <engine/utils/MathUtils.h>
#include <engine/CoreSystemsEngine.h>
#include <map/GlobalMapDataRepository.h>
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>
#include <SDL_surface.h>
#include <imgui/imgui.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId NAVMAP_DEBUG_SCENE_OBJECT_NAME = strutils::StringId("navmap_debug");
static const float MAP_TRANSITION_THRESHOLD = 0.03f;

///------------------------------------------------------------------------------------------------

glm::vec4 GetRGBAAt(SDL_Surface* surface, const int x, const int y)
{
    Uint8 r,g,b,a;
    auto pixel = *(Uint32*)((Uint8*)surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel);
    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
    return glm::vec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

///------------------------------------------------------------------------------------------------

PlayerController::PlayerController(const strutils::StringId& mapName)
    : mCurrentMapName(mapName)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    mNavmapResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/maps/" + mapName.GetString() + "/" + mapName.GetString() + "_navmap.png");
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
    
    auto navmapSurface = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::ImageSurfaceResource>(mNavmapResourceId).GetSurface();
    
    objectData.objectVelocity = {};
    auto length = glm::length(impulseVector);
    if (length > 0.0f)
    {
        objectData.objectVelocity = glm::normalize(impulseVector) * game_constants::PLAYER_SPEED * dtMillis;
        
        const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
        const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMapName);
        
        auto navmapCoords = glm::ivec2(static_cast<int>(((objectData.objectPosition.x - (currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE))/game_constants::MAP_RENDERED_SCALE + 0.5f) * navmapSurface->w), static_cast<int>((1.0f - ((objectData.objectPosition.y - (currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE))/game_constants::MAP_RENDERED_SCALE + 0.5f)) * navmapSurface->h));
        auto navmapColor = GetRGBAAt(navmapSurface, navmapCoords.x, navmapCoords.y);
        
        if (navmapColor.a > 0.0f)
        {
            objectData.objectVelocity *= navmapColor.r * navmapColor.r;
        }
        
        objectData.objectPosition += objectData.objectVelocity;
        
        playerSceneObject->mPosition += objectData.objectVelocity;
        playerNameSceneObject->mPosition += objectData.objectVelocity;
        
        auto nextNavmapCoords = glm::ivec2(static_cast<int>(((playerSceneObject->mPosition.x - (currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE))/game_constants::MAP_RENDERED_SCALE + 0.5f) * navmapSurface->w), static_cast<int>((1.0f - ((playerSceneObject->mPosition.y - (currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE))/game_constants::MAP_RENDERED_SCALE + 0.5f)) * navmapSurface->h));
        auto nextNavmapColor = GetRGBAAt(navmapSurface, nextNavmapCoords.x, nextNavmapCoords.y);
        
        if (nextNavmapColor.a <= 0.0f)
        {
            strutils::StringId nextMapName;
            
            // Determine movement direction
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
            
            // Give a further push to the player to bring them concretely
            // into the next navmap
            playerSceneObject->mPosition += objectData.objectVelocity;
            playerNameSceneObject->mPosition += objectData.objectVelocity;
            
            auto& systemsEngine = CoreSystemsEngine::GetInstance();
            systemsEngine.GetResourceLoadingService().UnloadResource(mNavmapResourceId);
            mNavmapResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/maps/" + nextMapName.GetString() + "/" + nextMapName.GetString() + "_navmap.png");
            mCurrentMapName = nextMapName;
            
            if (scene.FindSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME))
            {
                HideNavmapDebugView();
                ShowNavmapDebugView();
            }
        }
        
        mPreviousNavmapCoords = navmapCoords;
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
        auto scene = systemsEngine.GetSceneManager().FindScene(strutils::StringId("world"));
        
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
    auto scene = systemsEngine.GetSceneManager().FindScene(strutils::StringId("world"));
    
    const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMapName);
    
    auto navmapSceneObject = scene->CreateSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    navmapSceneObject->mPosition.x = currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE;
    navmapSceneObject->mPosition.y = currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE;
    navmapSceneObject->mPosition.z = 15.0f;
    navmapSceneObject->mScale *= game_constants::MAP_RENDERED_SCALE;
    
    auto navmapSurface = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::ImageSurfaceResource>(mNavmapResourceId).GetSurface();
    
    GLuint glTextureId; int mode;
    rendering::CreateGLTextureFromSurface(navmapSurface, glTextureId, mode);
    
    navmapSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().AddDynamicallyCreatedTextureResourceId("debug_navmap", glTextureId, 4096, 4096);
    navmapSceneObject->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.5f;
}

///------------------------------------------------------------------------------------------------

void PlayerController::HideNavmapDebugView()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(strutils::StringId("world"));
    
    auto navmapSceneObject = scene->FindSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    systemsEngine.GetResourceLoadingService().UnloadResource(navmapSceneObject->mTextureResourceId);
    scene->RemoveSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
}

///------------------------------------------------------------------------------------------------
