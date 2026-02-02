///------------------------------------------------------------------------------------------------
///  Game.cpp                                                                                        
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#include <bitset>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Camera.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/rendering/Fonts.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Date.h>
#include <engine/utils/Logging.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/OSMessageBox.h>
#include <engine/utils/PlatformMacros.h>
#include <enet/enet.h>
#include <fstream>
#include <game/AnimatedButton.h>
#include <game/Game.h>
#include <game/events/EventSystem.h>
#include <game/LocalPlayerInputController.h>
#include <game/ObjectAnimationController.h>
#include <imgui/imgui.h>
#include <net_common/NetworkMessages.h>
#include <map/GlobalMapDataRepository.h>
#include <map/MapConstants.h>
#include <map/MapResourceController.h>
#include <mutex>
#include <SDL.h>

//#define ALLOW_OFFLINE_PLAY
#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId NAVMAP_DEBUG_SCENE_OBJECT_NAME = strutils::StringId("debug_navmap");

///------------------------------------------------------------------------------------------------

Game::Game(const int argc, char** argv)
{
    if (argc > 0)
    {
        logging::Log(logging::LogType::INFO, "Initializing from CWD : %s", argv[0]);
    }
    
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::SetAssetFolder();
#endif
    
    CoreSystemsEngine::GetInstance().Start([&](){ Init(); }, [&](const float dtMillis){ Update(dtMillis); }, [&](){ ApplicationMovedToBackground(); }, [&](){ WindowResize(); }, [&](){ CreateDebugWidgets(); }, [&](){ OnOneSecondElapsed(); });
}

///------------------------------------------------------------------------------------------------

Game::~Game(){}

///------------------------------------------------------------------------------------------------
static ENetHost* sClient;
static ENetPeer* sServer;
static enet_uint32 sRTTAccum = 0;
static enet_uint32 sRTTSampleCount = 0;
static enet_uint32 sCurrentRTT = 0;
static bool sShowColliders = false;

void Game::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    GlobalMapDataRepository::GetInstance().LoadMapDefinitions();
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(game_constants::WORLD_SCENE_NAME);
    scene->GetCamera().SetZoomFactor(50.0f);
    scene->SetLoaded(true);
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mMapChangeEventListener = eventSystem.RegisterForEvent<events::MapChangeEvent>([this](const events::MapChangeEvent& event)
    {
        const auto& mapResources = mMapResourceController->GetMapResources(event.mNewMapName);
        mCurrentNavmap = mapResources.mNavmap;
    });
  
    mMapSupersessionEventListener = eventSystem.RegisterForEvent<events::MapSupersessionEvent>([=](const events::MapSupersessionEvent& event)
    {
        scene->RemoveSceneObject(strutils::StringId(event.mSupersededMapName.GetString() + "_top"));
        scene->RemoveSceneObject(strutils::StringId(event.mSupersededMapName.GetString() + "_bottom"));
    });
  
    mMapResourcesReadyEventListener = eventSystem.RegisterForEvent<events::MapResourcesReadyEvent>([this](const events::MapResourcesReadyEvent& event)
    {
        CreateMapSceneObjects(event.mMapName);
    });
    
    scene = systemsEngine.GetSceneManager().CreateScene(game_constants::GUI_SCENE_NAME);
    scene->GetCamera().SetZoomFactor(50.0f);
    scene->SetLoaded(true);
    
    scene::TextSceneObjectData textData;
    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textData.mText = "Health Points: 100";
    auto guiSceneObject = scene->CreateSceneObject(strutils::StringId("gui"));
    guiSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
    guiSceneObject->mSceneObjectTypeData = std::move(textData);
    guiSceneObject->mPosition = glm::vec3(0.0f, -0.155f, 1.0f);
    guiSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    guiSceneObject->mScale = glm::vec3(0.0004f);
    
    mTestButton = std::make_unique<AnimatedButton>(glm::vec3(-0.3f, 0.0f, 1.0f), glm::vec3(0.0001f), game_constants::DEFAULT_FONT_NAME, "Test my limits, left and right :)", strutils::StringId("test_button"), [](){}, *scene);
    
    mObjectAnimationController = std::make_unique<ObjectAnimationController>();
    mLocalPlayerId = 0;

    enet_initialize();
    atexit(enet_deinitialize);
    
    sClient = enet_host_create(nullptr, 1, 2, 0, 0);

    ENetAddress address{};
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    sServer = enet_host_connect(sClient, &address, 2, 0);
    if (!sServer)
    {
        logging::Log(logging::LogType::ERROR, "Failed to connect");
        return;
    }

    ENetEvent event;
    if (enet_host_service(sClient, &event, 5000) <= 0 ||
        event.type != ENET_EVENT_TYPE_CONNECT)
    {
        logging::Log(logging::LogType::ERROR, "Connection failed");
    }
    else
    {
        logging::Log(logging::LogType::INFO, "Connected to server");
    }
}

///------------------------------------------------------------------------------------------------

inline network::FacingDirection VecToDirection(const glm::vec3& vec)
{
    // make sure dir is not zero-length
    if (glm::length(vec) < 1e-6f) {
        // default or handle error
        return network::FacingDirection::SOUTH;
    }

    // angle in radians: atan2 returns angle from -pi to pi
    float angle = std::atan2(vec.y, vec.x);

    // convert to degrees (optional, but easier to reason about)
    float degrees = glm::degrees(angle);

    // normalize to [0, 360)
    if (degrees < 0.0f)
        degrees += 360.0f;

    // angular sectors: 360/8 = 45 degrees each
    if      (degrees >= 337.5f || degrees < 22.5f) return network::FacingDirection::EAST;
    else if (degrees < 67.5f)                      return network::FacingDirection::NORTH_EAST;
    else if (degrees < 112.5f)                     return network::FacingDirection::NORTH;
    else if (degrees < 157.5f)                     return network::FacingDirection::NORTH_WEST;
    else if (degrees < 202.5f)                     return network::FacingDirection::WEST;
    else if (degrees < 247.5f)                     return network::FacingDirection::SOUTH_WEST;
    else if (degrees < 292.5f)                     return network::FacingDirection::SOUTH;
    else                                           return network::FacingDirection::SOUTH_EAST;
}

float sDebugPlayerVelocityMultiplier = 1.0f;


void Game::Update(const float dtMillis)
{
    ENetEvent event;
    while (enet_host_service(sClient, &event, 0) > 0)
    {
        sRTTAccum += sServer->roundTripTime;
        sRTTSampleCount++;

        if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            auto messageType = static_cast<network::MessageType>(event.packet->data[0]);
            switch (messageType)
            {
                case network::MessageType::ObjectStateUpdateMessage:
                {
                    auto* message = reinterpret_cast<network::ObjectStateUpdateMessage*>(event.packet->data);
                    
                    // Pre-existing object
                    if (!mLocalObjectWrappers.contains(message->objectData.objectId))
                    {
                        CreateObject(message->objectData);
                        CreateObjectCollider(message->objectData);
                    }
                    
                    // Update everything but local player's data (for now)
                    if (message->objectData.objectId != mLocalPlayerId)
                    {
                        mLocalObjectWrappers[message->objectData.objectId].mObjectData = message->objectData;
                    }
                } break;
                
                case network::MessageType::PlayerConnectedMessage:
                {
                    auto* message = reinterpret_cast<network::PlayerConnectedMessage*>(event.packet->data);
                    mLocalPlayerId = message->objectId;
                    logging::Log(logging::LogType::INFO, "Received player ID %d", mLocalPlayerId);
                } break;
                    
                case network::MessageType::PlayerDisconnectedMessage:
                {
                    auto* message = reinterpret_cast<network::PlayerDisconnectedMessage*>(event.packet->data);
                    events::EventSystem::GetInstance().DispatchEvent<events::ObjectDestroyedEvent>(strutils::StringId("object-" + std::to_string(message->objectId)));

                    DestroyObject(message->objectId);
                } break;
                    
                case network::MessageType::ObjectCreatedMessage:
                {
                    auto* message = reinterpret_cast<network::ObjectCreatedMessage*>(event.packet->data);
                    CreateObject(message->objectData);
                    CreateObjectCollider(message->objectData);
                } break;
                
                case network::MessageType::ObjectDestroyedMessage:
                {
                    auto* message = reinterpret_cast<network::ObjectDestroyedMessage*>(event.packet->data);
                    DestroyObject(message->objectId);
                } break;
                    
                case network::MessageType::AttackMessage:
                case network::MessageType::UNUSED:
                    break;
            }
            
            enet_packet_destroy(event.packet);
        }
    }
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
    
    for (auto& [objectId, objectWrapperData]: mLocalObjectWrappers)
    {
        auto rootSceneObject = objectWrapperData.mSceneObjects.front();
        
        assert(rootSceneObject);

        if (objectId == mLocalPlayerId)
        {
            // Attacking overrides movement direction
            bool hasAttacked = false;
            if (CoreSystemsEngine::GetInstance().GetInputStateManager().VButtonTapped(input::Button::MAIN_BUTTON))
            {
                // Cooldown checks etc..
//                hasAttacked = true;
//                const auto& cam = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->GetCamera();
//                const auto& pointingPos = CoreSystemsEngine::GetInstance().GetInputStateManager().VGetPointingPosInWorldSpace(cam.GetViewMatrix(), cam.GetProjMatrix());
//                const auto& playerToPointingPos = glm::normalize(glm::vec3(pointingPos.x, pointingPos.y, objectWrapperData.mObjectData.position.z) - objectWrapperData.mObjectData.position);
//                const auto facingDirection = VecToDirection(playerToPointingPos);
//                
//                mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, glm::vec3(0.0f), dtMillis, facingDirection);
//                objectWrapperData.mObjectData.facingDirection = facingDirection;
//                
//                network::ObjectStateUpdateMessage stateUpdateMessage = {};
//                stateUpdateMessage.objectData = objectWrapperData.mObjectData;
//                
//                SendMessage(sServer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::RELIABLE);
//                
//                network::AttackMessage attackMessage = {};
//                attackMessage.attackerId = mLocalPlayerId;
//                attackMessage.attackType = network::AttackType::PROJECTILE;
//                attackMessage.projectileType = network::ProjectileType::FIREBALL;
//
//                SendMessage(sServer, &attackMessage, sizeof(attackMessage), network::channels::RELIABLE);
            }
            
            if (!hasAttacked)
            {
                const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
                const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMap);
                
                auto inputDirection = LocalPlayerInputController::GetMovementDirection();
                auto velocity = glm::vec3(inputDirection.x, inputDirection.y, 0.0f) * objectWrapperData.mObjectData.speed * sDebugPlayerVelocityMultiplier * dtMillis;
                
                const auto& animationInfoResult = mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, velocity, dtMillis, std::nullopt);
                
                // Movement integration first horizontally
                rootSceneObject->mPosition.x += velocity.x;
                
                auto speculativeNavmapCoord = mCurrentNavmap->GetNavmapCoord(rootSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
                if (mCurrentNavmap->GetNavmapTileAt(speculativeNavmapCoord) == network::NavmapTileType::SOLID)
                {
                    rootSceneObject->mPosition.x -= velocity.x;
                }
                
                // ... then vertically
                rootSceneObject->mPosition.y += velocity.y;
                speculativeNavmapCoord = mCurrentNavmap->GetNavmapCoord(rootSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
                if (mCurrentNavmap->GetNavmapTileAt(speculativeNavmapCoord) == network::NavmapTileType::SOLID)
                {
                    rootSceneObject->mPosition.y -= velocity.y;
                }
                
                // Determine map change direction
                static const float MAP_TRANSITION_THRESHOLD = 0.00f;
                strutils::StringId nextMapName = map_constants::NO_MAP_CONNECTION_NAME;
                if (rootSceneObject->mPosition.x > currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE + (currentMapDefinition.mMapDimensions.x * game_constants::MAP_RENDERED_SCALE)/2.0f - MAP_TRANSITION_THRESHOLD)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::EAST);
                }
                else if (rootSceneObject->mPosition.x < currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE - (currentMapDefinition.mMapDimensions.x * game_constants::MAP_RENDERED_SCALE)/2.0f + MAP_TRANSITION_THRESHOLD)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::WEST);
                }
                else if (rootSceneObject->mPosition.y > currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE + (currentMapDefinition.mMapDimensions.y * game_constants::MAP_RENDERED_SCALE)/2.0f - MAP_TRANSITION_THRESHOLD)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::NORTH);
                }
                else if (rootSceneObject->mPosition.y < currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE - (currentMapDefinition.mMapDimensions.y * game_constants::MAP_RENDERED_SCALE)/2.0f + MAP_TRANSITION_THRESHOLD)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::SOUTH);
                }
                
                if (nextMapName != map_constants::NO_MAP_CONNECTION_NAME)
                {
                    mCurrentMap = nextMapName;
                       
                    events::EventSystem::GetInstance().DispatchEvent<events::MapChangeEvent>(mCurrentMap);
                    
                    if (scene->FindSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME))
                    {
                        HideDebugNavmap();
                        ShowDebugNavmap();
                    }
                    
                    // Rubberband out of any new solid tiles we land in after map change
                    speculativeNavmapCoord = mCurrentNavmap->GetNavmapCoord(rootSceneObject->mPosition, currentMapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
                    if (mCurrentNavmap->GetNavmapTileAt(speculativeNavmapCoord) == network::NavmapTileType::SOLID)
                    {
                        rootSceneObject->mPosition -= velocity;
                    }
                }
                
                objectWrapperData.mObjectData.position = rootSceneObject->mPosition;
                objectWrapperData.mObjectData.velocity = velocity;
                objectWrapperData.mObjectData.currentAnimation = network::AnimationType::RUNNING;
                objectWrapperData.mObjectData.facingDirection = animationInfoResult.mFacingDirection;
                network::SetCurrentMap(objectWrapperData.mObjectData, mCurrentMap.GetString());
                
                network::ObjectStateUpdateMessage stateUpdateMessage = {};
                stateUpdateMessage.objectData = mLocalObjectWrappers[mLocalPlayerId].mObjectData;
                
                SendMessage(sServer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::UNRELIABLE);
            }
        }
        else
        {
            auto vecToPosition = objectWrapperData.mObjectData.position - rootSceneObject->mPosition;
            if (glm::length(vecToPosition) > 0.002f)
            {
                auto direction = glm::normalize(vecToPosition);
                auto velocity = glm::vec3(direction.x, direction.y, 0.0f) * objectWrapperData.mObjectData.speed * dtMillis;
                rootSceneObject->mPosition += velocity;
            }
            
            if (objectWrapperData.mObjectData.objectType != network::ObjectType::ATTACK || objectWrapperData.mObjectData.attackType != network::AttackType::PROJECTILE)
            {
                mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, objectWrapperData.mObjectData.velocity, dtMillis, objectWrapperData.mObjectData.facingDirection);
            }
        }
        
        for (auto otherSceneObject: objectWrapperData.mSceneObjects)
        {
            otherSceneObject->mPosition = glm::vec3(rootSceneObject->mPosition.x, rootSceneObject->mPosition.y, otherSceneObject->mPosition.z);
        }
    }

    enet_host_flush(sClient);
    
    // Camera updates
    auto sceneObject = scene->FindSceneObject(strutils::StringId("object-" + std::to_string(mLocalPlayerId)));
    if (sceneObject)
    {
        scene->GetCamera().SetPosition(glm::vec3(sceneObject->mPosition.x, sceneObject->mPosition.y, scene->GetCamera().GetPosition().z));
    }
    
    if (mMapResourceController)
    {
        mMapResourceController->Update(mCurrentMap);
    }
    
    mTestButton->Update(dtMillis);
}

///------------------------------------------------------------------------------------------------

void Game::ApplicationMovedToBackground()
{
}

///------------------------------------------------------------------------------------------------

void Game::OnOneSecondElapsed()
{
    sCurrentRTT = sRTTAccum/(math::Max(1U, sRTTSampleCount));
    sRTTAccum = 0;
    sRTTSampleCount = 0;
}

///------------------------------------------------------------------------------------------------

void Game::WindowResize()
{
}

///------------------------------------------------------------------------------------------------

void Game::CreateObject(const network::ObjectData& objectData)
{
    if (objectData.objectId == mLocalPlayerId)
    {
        assert(!mMapResourceController);
        mCurrentMap = strutils::StringId(network::GetCurrentMapString(objectData));
        mMapResourceController = std::make_unique<MapResourceController>(mCurrentMap);
        mCurrentNavmap = mMapResourceController->GetMapResources(mCurrentMap).mNavmap;
        auto loadedMapResources = mMapResourceController->GetAllLoadedMapResources();
        for (const auto& mapResources: loadedMapResources)
        {
            CreateMapSceneObjects(mapResources.first);
        }
    }
    
    mLocalObjectWrappers[objectData.objectId].mObjectData = objectData;

    auto sceneObjectName = strutils::StringId("object-" + std::to_string(objectData.objectId));

    auto sceneObject = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->FindSceneObject(sceneObjectName);
    
    if (sceneObject)
    {
        logging::Log(logging::LogType::WARNING, "Attempted to re-create pre-existing object %s", sceneObjectName.GetString().c_str());
    }
    else
    {
        sceneObject = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->CreateSceneObject(sceneObjectName);
        mLocalObjectWrappers[objectData.objectId].mSceneObjects.push_back(sceneObject);
        switch (objectData.objectType)
        {
            case network::ObjectType::PLAYER:
            {
                sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/char.png");
                sceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "player.vs");
                sceneObject->mShaderBoolUniformValues[IS_TEXTURE_SHEET_UNIFORM_NAME] = true;
                sceneObject->mShaderBoolUniformValues[strutils::StringId("is_local")] = objectData.objectId == mLocalPlayerId;
                sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                sceneObject->mScale = glm::vec3(objectData.objectScale);
            } break;
            
            case network::ObjectType::ATTACK:
            {
                if (objectData.attackType == network::AttackType::PROJECTILE && objectData.projectileType == network::ProjectileType::FIREBALL)
                {
                    sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/projectile.png");
                    sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                    sceneObject->mScale = glm::vec3(objectData.objectScale);
                }
                else
                {
                    assert(false);
                }
            } break;

            case network::ObjectType::NPC:
            case network::ObjectType::STATIC:
            {
                assert(false);
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::CreateObjectCollider(const network::ObjectData& objectData)
{
    mLocalObjectWrappers[objectData.objectId].mColliderData = objectData.colliderData;

    // IF DEBUG
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto sceneObjectName = strutils::StringId("object-" + std::to_string(objectData.objectId) + "-collider");
    auto sceneObject = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->CreateSceneObject(sceneObjectName);
    
    switch (objectData.colliderData.colliderType)
    {
        case network::ColliderType::CIRCLE:
        {
            sceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "debug/debug_circle.png");
        } break;
        
        case network::ColliderType::RECTANGLE:
        {
            
        } break;
    }

    sceneObject->mScale = glm::vec3(objectData.colliderData.colliderRelativeDimentions.x, objectData.colliderData.colliderRelativeDimentions.y, 1.0f);
    sceneObject->mScale *= mLocalObjectWrappers[objectData.objectId].mSceneObjects.front()->mScale;
    sceneObject->mPosition = mLocalObjectWrappers[objectData.objectId].mObjectData.position;
    sceneObject->mPosition.z = map_constants::TILE_NAVMAP_LAYER_Z;
    sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.5f;
    sceneObject->mInvisible = !sShowColliders;
    mLocalObjectWrappers[objectData.objectId].mSceneObjects.push_back(sceneObject);
}

///------------------------------------------------------------------------------------------------

void Game::DestroyObject(const network::objectId_t objectId)
{
    for (auto sceneObject: mLocalObjectWrappers.at(objectId).mSceneObjects)
    {
        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->RemoveSceneObject(sceneObject->mName);
    }
    mLocalObjectWrappers.erase(objectId);
}

///------------------------------------------------------------------------------------------------

void Game::CreateMapSceneObjects(const strutils::StringId& mapName)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
    
    const auto& mapDefinition = GlobalMapDataRepository::GetInstance().GetMapDefinition(mapName);
    const auto& mapResources = mMapResourceController->GetMapResources(mapName);
    
    assert(mapResources.mMapResourcesState == MapResourcesState::LOADED);
    
    auto mapBottomLayer = scene->CreateSceneObject(strutils::StringId(mapDefinition.mMapName.GetString()  + "_bottom"));
    mapBottomLayer->mPosition.x = mapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE;
    mapBottomLayer->mPosition.y = mapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE;
    mapBottomLayer->mPosition.z = map_constants::TILE_BOTTOM_LAYER_Z;
    mapBottomLayer->mScale *= game_constants::MAP_RENDERED_SCALE;
    mapBottomLayer->mTextureResourceId = mapResources.mBottomLayerTextureResourceId;
    mapBottomLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
    mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapDefinition.mMapDimensions.x + map_constants::MAP_RENDERING_SEAMS_BIAS;
    mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapDefinition.mMapDimensions.y + map_constants::MAP_RENDERING_SEAMS_BIAS;
    
    auto mapTopLayer = scene->CreateSceneObject(strutils::StringId(mapDefinition.mMapName.GetString()  + "_top"));
    mapTopLayer->mPosition.x = mapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE;
    mapTopLayer->mPosition.y = mapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE;
    mapTopLayer->mPosition.z = map_constants::TILE_TOP_LAYER_Z;
    mapTopLayer->mScale *= game_constants::MAP_RENDERED_SCALE;
    mapTopLayer->mTextureResourceId = mapResources.mTopLayerTextureResourceId;
    mapTopLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
    mapTopLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapDefinition.mMapDimensions.x + map_constants::MAP_RENDERING_SEAMS_BIAS;
    mapTopLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapDefinition.mMapDimensions.y + map_constants::MAP_RENDERING_SEAMS_BIAS;
}

///------------------------------------------------------------------------------------------------

void Game::ShowDebugNavmap()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);

    const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMap);
    
    auto navmapSceneObject = scene->CreateSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    navmapSceneObject->mPosition.x = currentMapDefinition.mMapPosition.x * game_constants::MAP_RENDERED_SCALE;
    navmapSceneObject->mPosition.y = currentMapDefinition.mMapPosition.y * game_constants::MAP_RENDERED_SCALE;
    navmapSceneObject->mPosition.z = map_constants::TILE_NAVMAP_LAYER_Z;
    navmapSceneObject->mScale *= game_constants::MAP_RENDERED_SCALE;
    
    auto navmapSurface = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::ImageSurfaceResource>(mMapResourceController->GetMapResources(mCurrentMap).mNavmapImageResourceId).GetSurface();
    
    GLuint glTextureId; int mode;
    rendering::CreateGLTextureFromSurface(navmapSurface, glTextureId, mode, true);
    
    navmapSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().AddDynamicallyCreatedTextureResourceId(NAVMAP_DEBUG_SCENE_OBJECT_NAME.GetString(), glTextureId, map_constants::CLIENT_NAVMAP_IMAGE_SIZE, map_constants::CLIENT_NAVMAP_IMAGE_SIZE);
    navmapSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.6f;
}

///------------------------------------------------------------------------------------------------

void Game::HideDebugNavmap()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);

    auto navmapSceneObject = scene->FindSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    systemsEngine.GetResourceLoadingService().UnloadResource(navmapSceneObject->mTextureResourceId);
    scene->RemoveSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
}

///------------------------------------------------------------------------------------------------

#if defined(USE_IMGUI)
void Game::CreateDebugWidgets()
{
    ImGui::Begin("Game Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Ping (millis): %d", sCurrentRTT);
    ImGui::Text("Local Player Id: %llu", mLocalPlayerId);
    ImGui::SliderFloat("PVM", &sDebugPlayerVelocityMultiplier, 0.01f, 10.0f);
    ImGui::Text("Show Colliders: ");
    ImGui::SameLine();
    if (ImGui::Checkbox("##", &sShowColliders))
    {
        for (const auto& [objectId, objectWrapperData]: mLocalObjectWrappers)
        {
            for (auto sceneObject: objectWrapperData.mSceneObjects)
            {
                if (strutils::StringEndsWith(sceneObject->mName.GetString(), "collider"))
                {
                    sceneObject->mInvisible = !sShowColliders;
                }
            }
        }
        // Toggle SO invisibility of colliders in object wrappers
    }
    
    ImGui::SeparatorText("Network Object Data");
    for (const auto& [objectId, objectWrapperData]: mLocalObjectWrappers)
    {
        auto name = objectId == mLocalPlayerId ? std::string("localPlayer") : ("object-" + std::to_string(objectId));
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_None))
        {
            ImGui::PushID(name.c_str());
            ImGui::Text("Object Type: %d", static_cast<int>(objectWrapperData.mObjectData.objectType));
            ImGui::Text("Current Map: %s", network::GetCurrentMapString(objectWrapperData.mObjectData).c_str());
            ImGui::Text("Facing Direction: %d", static_cast<int>(objectWrapperData.mObjectData.facingDirection));
            
            const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
            const auto& mapName = strutils::StringId(network::GetCurrentMapString(objectWrapperData.mObjectData));
            if (mMapResourceController && mMapResourceController->GetAllLoadedMapResources().contains(mapName) && mMapResourceController->GetAllLoadedMapResources().at(mapName).mMapResourcesState == MapResourcesState::LOADED)
            {
                const auto& mapDefinition = globalMapDataRepo.GetMapDefinition(mapName);
                auto navmap = mMapResourceController->GetAllLoadedMapResources().at(mapName).mNavmap;
                auto currentNavmapCoords = navmap->GetNavmapCoord(objectWrapperData.mSceneObjects.front()->mPosition, mapDefinition.mMapPosition, game_constants::MAP_RENDERED_SCALE);
                auto currentNavmapTileType = navmap->GetNavmapTileAt(currentNavmapCoords);
                
                ImGui::Text("Navmap Tile: x:%d, y:%d", currentNavmapCoords.y, currentNavmapCoords.x);
                ImGui::Text("Navmap Type: %s", network::GetNavmapTileTypeName(currentNavmapTileType));
            }
            
            ImGui::PopID();
        }
    }
    ImGui::End();
    
    static bool sShowNavmap = false;
    ImGui::Begin("Map", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Current Map: %s", mCurrentMap.GetString().c_str());
    ImGui::Text("Show Navmap: ");
    ImGui::SameLine();
    if (ImGui::Checkbox("##", &sShowNavmap))
    {
        if (mMapResourceController)
        {
            if (sShowNavmap)
            {
                ShowDebugNavmap();
            }
            else
            {
                HideDebugNavmap();
            }
        }
    }
    
    if (mMapResourceController)
    {
        ImGui::SeparatorText("LoadedMaps");
        mMapResourceController->CreateDebugWidgets();
    }
    ImGui::End();
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------
