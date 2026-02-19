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
#include <game/ui/AnimatedButton.h>
#include <game/CastBarController.h>
#include <game/Game.h>
#include <game/GameCommon.h>
#include <game/NetworkEntitySceneObjectFactory.h>
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
#include <iostream>

///------------------------------------------------------------------------------------------------

static const strutils::StringId NAVMAP_DEBUG_SCENE_OBJECT_NAME = strutils::StringId("debug_navmap");
static const strutils::StringId MAP_DEBUG_GRID_UNIFORM_NAME = strutils::StringId("debug_grid");
static const std::string QUADTREE_DEBUG_SCENE_OBJECT_NAME_PREFIX = "debug_quadtree_";
static const std::string PATH_DEBUG_SCENE_OBJECT_NAME_PREFIX = "debug_path_";
static const float DESTROYED_OBJECT_FADE_OUT_TIME_SECS = 0.1f;

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
static bool sShowQuadtree = false;
static bool sShowDebugGrid = false;
static bool sShowObjectPaths = false;
static float sRequestQuadtreeTimer = 1.0f;
static float sRequestObjectPathTimer = 0.1f;

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
    
//    scene::TextSceneObjectData textData;
//    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
//    textData.mText = "Health Points: 100";
//    auto guiSceneObject = scene->CreateSceneObject(strutils::StringId("gui"));
//    guiSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
//    guiSceneObject->mSceneObjectTypeData = std::move(textData);
//    guiSceneObject->mPosition = glm::vec3(0.0f, -0.155f, 1.0f);
//    guiSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
//    guiSceneObject->mScale = glm::vec3(0.0004f);
    
//    mTestButton = std::make_unique<AnimatedButton>(glm::vec3(-0.3f, 0.0f, 1.0f), glm::vec3(0.0001f), game_constants::DEFAULT_FONT_NAME, "Test my limits, left and right :)", strutils::StringId("test_button"), [](){}, *scene);
    
    mCastBarController = std::make_unique<CastBarController>(scene);
    //mCastBarController->ShowCastBar(1.0f);

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
                    }
                    
                    // Update everything but local player's data (for now)
                    if (message->objectData.objectId != mLocalPlayerId)
                    {
                        mLocalObjectWrappers[message->objectData.objectId].mObjectData = message->objectData;
                    }
                    
                    assert(math::Abs(mLocalObjectWrappers[message->objectData.objectId].mSceneObjects.front()->mScale.x - message->objectData.objectScale) < 0.0001f);
                } break;
                
                case network::MessageType::DebugGetQuadtreeResponseMessage:
                {
                    auto* message = reinterpret_cast<network::DebugGetQuadtreeResponseMessage*>(event.packet->data);
                    
                    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
                    
                    scene->RemoveAllSceneObjectsWithNameStartingWith(QUADTREE_DEBUG_SCENE_OBJECT_NAME_PREFIX);
                    for (int i = 0; i < message->quadtreeData.debugRectCount; ++i)
                    {
                        auto quadtreeSceneObject = scene->CreateSceneObject(strutils::StringId(QUADTREE_DEBUG_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
                        quadtreeSceneObject->mPosition = message->quadtreeData.debugRectPositions[i];
                        quadtreeSceneObject->mScale = message->quadtreeData.debugRectDimensions[i];
                        quadtreeSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "debug/debug_quadtree.png");
                        quadtreeSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                        quadtreeSceneObject->mInvisible = !sShowQuadtree;
                    }
                } break;
                    
                case network::MessageType::DebugGetObjectPathResponseMessage:
                {
                    auto* message = reinterpret_cast<network::DebugGetObjectPathResponseMessage*>(event.packet->data);
                    
                    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
                    
                    scene->RemoveAllSceneObjectsWithNameStartingWith(PATH_DEBUG_SCENE_OBJECT_NAME_PREFIX);
                    for (int i = 0; i < message->pathData.debugPathPositionsCount; ++i)
                    {
                        auto pathSceneObject = scene->CreateSceneObject(strutils::StringId(PATH_DEBUG_SCENE_OBJECT_NAME_PREFIX + std::to_string(message->objectId) + "_" + std::to_string(i)));
                        pathSceneObject->mPosition = message->pathData.debugPathPositions[i];
                        pathSceneObject->mScale = glm::vec3(network::MAP_TILE_SIZE/10.0f) * glm::vec3(i + 1);
                        pathSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "debug/debug_circle.png");
                        pathSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                        pathSceneObject->mInvisible = !sShowObjectPaths;
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
                    DestroyObject(message->objectId);
                } break;
                    
                case network::MessageType::ObjectCreatedMessage:
                {
                    auto* message = reinterpret_cast<network::ObjectCreatedMessage*>(event.packet->data);
                    CreateObject(message->objectData);
                } break;
                
                case network::MessageType::ObjectDestroyedMessage:
                {
                    auto* message = reinterpret_cast<network::ObjectDestroyedMessage*>(event.packet->data);
                    DestroyObject(message->objectId);
                } break;
                
                case network::MessageType::BeginAttackResponseMessage:
                {
                    auto* message = reinterpret_cast<network::BeginAttackResponseMessage*>(event.packet->data);
                    if (message->allowed)
                    {
                        mCastBarController->BeginCast(message->chargeDurationSecs, [this]()
                        {
                            mLocalObjectWrappers[mLocalPlayerId].mObjectData.objectState = network::ObjectState::MELEE_ATTACK;
                        });
                    }
                    else
                    {
                        mLocalObjectWrappers[mLocalPlayerId].mObjectData.objectState = network::ObjectState::IDLE;
                    }
                } break;
                    
                case network::MessageType::BeginAttackRequestMessage:
                case network::MessageType::CancelAttackMessage:
                case network::MessageType::DebugGetQuadtreeRequestMessage:
                case network::MessageType::DebugGetObjectPathRequestMessage:
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
            if (CoreSystemsEngine::GetInstance().GetInputStateManager().VButtonTapped(input::Button::SECONDARY_BUTTON) && objectWrapperData.mObjectData.objectState != network::ObjectState::BEGIN_MELEE &&
                objectWrapperData.mObjectData.objectState != network::ObjectState::MELEE_ATTACK)
            {
                // Cooldown checks etc..
                const auto& cam = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->GetCamera();
                const auto& pointingPos = CoreSystemsEngine::GetInstance().GetInputStateManager().VGetPointingPosInWorldSpace(cam.GetViewMatrix(), cam.GetProjMatrix());
                const auto& playerToPointingPos = glm::normalize(glm::vec3(pointingPos.x, pointingPos.y, objectWrapperData.mObjectData.position.z) - objectWrapperData.mObjectData.position);
                const auto facingDirection = network::VecToFacingDirection(playerToPointingPos);
                
                objectWrapperData.mObjectData.objectState = network::ObjectState::BEGIN_MELEE;
                objectWrapperData.mObjectData.facingDirection = facingDirection;
                mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, objectWrapperData.mObjectData.objectType, objectWrapperData.mObjectData.objectState, facingDirection, glm::vec3(0.0f), dtMillis);
                
                network::ObjectStateUpdateMessage stateUpdateMessage = {};
                stateUpdateMessage.objectData = objectWrapperData.mObjectData;
                
                network::SendMessage(sServer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::RELIABLE);
                
                network::BeginAttackRequestMessage attackRequestMessage = {};
                attackRequestMessage.attackerId = mLocalPlayerId;
                attackRequestMessage.attackType = network::AttackType::MELEE;

                network::SendMessage(sServer, &attackRequestMessage, sizeof(attackRequestMessage), network::channels::RELIABLE);
            }
            else if (objectWrapperData.mObjectData.objectState == network::ObjectState::BEGIN_MELEE)
            {
                mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, objectWrapperData.mObjectData.objectType, objectWrapperData.mObjectData.objectState, objectWrapperData.mObjectData.facingDirection, glm::vec3(0.0f), dtMillis);
            }
            else if (objectWrapperData.mObjectData.objectState == network::ObjectState::MELEE_ATTACK)
            {
                const auto& animationInfoResult = mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, objectWrapperData.mObjectData.objectType, objectWrapperData.mObjectData.objectState, objectWrapperData.mObjectData.facingDirection, glm::vec3(0.0f), dtMillis);
                if (animationInfoResult.mAnimationFinished)
                {
                    objectWrapperData.mObjectData.objectState = network::ObjectState::IDLE;
                }
            }
            else if (objectWrapperData.mObjectData.objectState == network::ObjectState::IDLE || objectWrapperData.mObjectData.objectState == network::ObjectState::RUNNING)
            {
                const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
                const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMap);
                
                auto inputDirection = LocalPlayerInputController::GetMovementDirection();
                auto velocity = glm::vec3(inputDirection.x, inputDirection.y, 0.0f) * objectWrapperData.mObjectData.speed * sDebugPlayerVelocityMultiplier * dtMillis;
                
                const auto& animationInfoResult = mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, objectWrapperData.mObjectData.objectType, objectWrapperData.mObjectData.objectState, network::VecToFacingDirection(velocity), velocity, dtMillis);
                
                // Movement integration first horizontally
                rootSceneObject->mPosition.x += velocity.x;
                
                auto speculativeNavmapCoord = mCurrentNavmap->GetNavmapCoord(rootSceneObject->mPosition, currentMapDefinition.mMapPosition, network::MAP_GAME_SCALE);
                if (mCurrentNavmap->GetNavmapTileAt(speculativeNavmapCoord) == network::NavmapTileType::SOLID)
                {
                    rootSceneObject->mPosition.x -= velocity.x;
                }
                
                // ... then vertically
                rootSceneObject->mPosition.y += velocity.y;
                speculativeNavmapCoord = mCurrentNavmap->GetNavmapCoord(rootSceneObject->mPosition, currentMapDefinition.mMapPosition, network::MAP_GAME_SCALE);
                if (mCurrentNavmap->GetNavmapTileAt(speculativeNavmapCoord) == network::NavmapTileType::SOLID)
                {
                    rootSceneObject->mPosition.y -= velocity.y;
                }
                
                // Determine map change direction
                strutils::StringId nextMapName = map_constants::NO_MAP_CONNECTION_NAME;
                if (rootSceneObject->mPosition.x > currentMapDefinition.mMapPosition.x * network::MAP_GAME_SCALE + (currentMapDefinition.mMapDimensions.x * network::MAP_GAME_SCALE)/2.0f)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::EAST);
                }
                else if (rootSceneObject->mPosition.x < currentMapDefinition.mMapPosition.x * network::MAP_GAME_SCALE - (currentMapDefinition.mMapDimensions.x * network::MAP_GAME_SCALE)/2.0f)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::WEST);
                }
                else if (rootSceneObject->mPosition.y > currentMapDefinition.mMapPosition.y * network::MAP_GAME_SCALE + (currentMapDefinition.mMapDimensions.y * network::MAP_GAME_SCALE)/2.0f)
                {
                    nextMapName = globalMapDataRepo.GetConnectedMapName(mCurrentMap, MapConnectionDirection::NORTH);
                }
                else if (rootSceneObject->mPosition.y < currentMapDefinition.mMapPosition.y * network::MAP_GAME_SCALE - (currentMapDefinition.mMapDimensions.y * network::MAP_GAME_SCALE)/2.0f)
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
                    speculativeNavmapCoord = mCurrentNavmap->GetNavmapCoord(rootSceneObject->mPosition, globalMapDataRepo.GetMapDefinition(mCurrentMap).mMapPosition, network::MAP_GAME_SCALE);
                    if (mCurrentNavmap->GetNavmapTileAt(speculativeNavmapCoord) == network::NavmapTileType::SOLID)
                    {
                        rootSceneObject->mPosition -= velocity;
                    }
                }
                
                objectWrapperData.mObjectData.position = rootSceneObject->mPosition;
                objectWrapperData.mObjectData.velocity = velocity;
                objectWrapperData.mObjectData.objectState = network::ObjectState::RUNNING;
                objectWrapperData.mObjectData.facingDirection = animationInfoResult.mFacingDirection;
                network::SetCurrentMap(objectWrapperData.mObjectData, mCurrentMap.GetString());
                
                network::ObjectStateUpdateMessage stateUpdateMessage = {};
                stateUpdateMessage.objectData = mLocalObjectWrappers[mLocalPlayerId].mObjectData;
                
                network::SendMessage(sServer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::UNRELIABLE);
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
            
            mObjectAnimationController->UpdateObjectAnimation(rootSceneObject, objectWrapperData.mObjectData.objectType, objectWrapperData.mObjectData.objectState, objectWrapperData.mObjectData.facingDirection, objectWrapperData.mObjectData.velocity, dtMillis);
        }
        
        for (auto otherSceneObject: objectWrapperData.mSceneObjects)
        {
            otherSceneObject->mPosition = glm::vec3(rootSceneObject->mPosition.x, rootSceneObject->mPosition.y, otherSceneObject->mPosition.z);
        }
    }
    
    if (sShowQuadtree)
    {
        sRequestQuadtreeTimer -= dtMillis/1000.0f;
        if (sRequestQuadtreeTimer < 0)
        {
            sRequestQuadtreeTimer = 1.0f;
            network::DebugGetQuadtreeRequestMessage requestQuadtreeDataMessage = {};
            network::SendMessage(sServer, &requestQuadtreeDataMessage, sizeof(requestQuadtreeDataMessage), network::channels::RELIABLE);
        }
    }
    else
    {
        scene->RemoveAllSceneObjectsWithNameStartingWith(QUADTREE_DEBUG_SCENE_OBJECT_NAME_PREFIX);
    }
    
    if (sShowObjectPaths)
    {
        sRequestObjectPathTimer -= dtMillis/1000.0f;
        if (sRequestObjectPathTimer < 0)
        {
            sRequestObjectPathTimer = 0.1f;
            network::DebugGetObjectPathRequestMessage requestPathDataMessage = {};
            
            for (auto& [objectId, objectWrapperData]: mLocalObjectWrappers)
            {
                if (objectWrapperData.mObjectData.objectType == network::ObjectType::NPC)
                {
                    requestPathDataMessage.objectId = objectId;
                    network::SendMessage(sServer, &requestPathDataMessage, sizeof(requestPathDataMessage), network::channels::UNRELIABLE);
                }
            }
        }
    }
    else
    {
        scene->RemoveAllSceneObjectsWithNameStartingWith(PATH_DEBUG_SCENE_OBJECT_NAME_PREFIX);
    }
    
    enet_host_flush(sClient);
    
    // Camera updates
    auto sceneObject = scene->FindSceneObject(GetSceneObjectNameId(mLocalPlayerId));
    if (sceneObject)
    {
        scene->GetCamera().SetPosition(glm::vec3(sceneObject->mPosition.x, sceneObject->mPosition.y, scene->GetCamera().GetPosition().z));
    }
    
    if (mMapResourceController)
    {
        mMapResourceController->Update(mCurrentMap);
    }
    
    if (mTestButton)
    {
        mTestButton->Update(dtMillis);
    }
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
    mLocalObjectWrappers[objectData.objectId].mColliderData = objectData.colliderData;
    
    NetworkEntitySceneObjectFactory::CreateSceneObjects(objectData, sShowColliders, mLocalObjectWrappers[objectData.objectId].mSceneObjects);
}

///------------------------------------------------------------------------------------------------

void Game::DestroyObject(const network::objectId_t objectId)
{
    events::EventSystem::GetInstance().DispatchEvent<events::ObjectDestroyedEvent>(GetSceneObjectNameId(objectId));
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    for (auto sceneObject: mLocalObjectWrappers.at(objectId).mSceneObjects)
    {
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, DESTROYED_OBJECT_FADE_OUT_TIME_SECS), [sceneObject]()
        {
            CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->RemoveSceneObject(sceneObject->mName);
        });
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
    mapBottomLayer->mPosition.x = mapDefinition.mMapPosition.x * network::MAP_GAME_SCALE;
    mapBottomLayer->mPosition.y = mapDefinition.mMapPosition.y * network::MAP_GAME_SCALE;
    mapBottomLayer->mPosition.z = map_constants::TILE_BOTTOM_LAYER_Z;
    mapBottomLayer->mScale *= network::MAP_GAME_SCALE;
    mapBottomLayer->mTextureResourceId = mapResources.mBottomLayerTextureResourceId;
    mapBottomLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
    mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapDefinition.mMapDimensions.x + map_constants::MAP_RENDERING_SEAMS_BIAS;
    mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapDefinition.mMapDimensions.y + map_constants::MAP_RENDERING_SEAMS_BIAS;
    mapBottomLayer->mShaderBoolUniformValues[MAP_DEBUG_GRID_UNIFORM_NAME] = sShowDebugGrid;
    
    auto mapTopLayer = scene->CreateSceneObject(strutils::StringId(mapDefinition.mMapName.GetString()  + "_top"));
    mapTopLayer->mPosition.x = mapDefinition.mMapPosition.x * network::MAP_GAME_SCALE;
    mapTopLayer->mPosition.y = mapDefinition.mMapPosition.y * network::MAP_GAME_SCALE;
    mapTopLayer->mPosition.z = map_constants::TILE_TOP_LAYER_Z;
    mapTopLayer->mScale *= network::MAP_GAME_SCALE;
    mapTopLayer->mTextureResourceId = mapResources.mTopLayerTextureResourceId;
    mapTopLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
    mapTopLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapDefinition.mMapDimensions.x + map_constants::MAP_RENDERING_SEAMS_BIAS;
    mapTopLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapDefinition.mMapDimensions.y + map_constants::MAP_RENDERING_SEAMS_BIAS;
    mapTopLayer->mShaderBoolUniformValues[MAP_DEBUG_GRID_UNIFORM_NAME] = sShowDebugGrid;
}

///------------------------------------------------------------------------------------------------

void Game::ShowDebugNavmap()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);

    const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& currentMapDefinition = globalMapDataRepo.GetMapDefinition(mCurrentMap);
    
    auto navmapSceneObject = scene->CreateSceneObject(NAVMAP_DEBUG_SCENE_OBJECT_NAME);
    navmapSceneObject->mPosition.x = currentMapDefinition.mMapPosition.x * network::MAP_GAME_SCALE;
    navmapSceneObject->mPosition.y = currentMapDefinition.mMapPosition.y * network::MAP_GAME_SCALE;
    navmapSceneObject->mPosition.z = map_constants::TILE_NAVMAP_LAYER_Z;
    navmapSceneObject->mScale *= network::MAP_GAME_SCALE;
    
    auto navmapSurface = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::ImageSurfaceResource>(mMapResourceController->GetMapResources(mCurrentMap).mNavmapImageResourceId).GetSurface();
    
    GLuint glTextureId; int mode;
    rendering::CreateGLTextureFromSurface(navmapSurface, glTextureId, mode, true);
    
    navmapSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().AddDynamicallyCreatedTextureResourceId(NAVMAP_DEBUG_SCENE_OBJECT_NAME.GetString(), glTextureId, network::NAVMAP_SIZE, network::NAVMAP_SIZE);
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
        auto name = objectId == mLocalPlayerId ? std::string("localPlayer") : GetSceneObjectName(objectId);
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_None))
        {
            ImGui::PushID(name.c_str());
            ImGui::Text("Object Type: %s", network::GetObjectTypeString(objectWrapperData.mObjectData.objectType));
            ImGui::Text("Object State: %s", network::GetObjectStateString(objectWrapperData.mObjectData.objectState));
            ImGui::Text("Facing Direction: %s", network::GetFacingDirectionString(objectWrapperData.mObjectData.facingDirection));
            ImGui::Text("Object Faction: %s", network::GetObjectFactionString(objectWrapperData.mObjectData.objectFaction));
            ImGui::Text("Attack Type: %s", network::GetAttackTypeString(objectWrapperData.mObjectData.attackType));
            ImGui::Text("Projectile Type: %s", network::GetProjectileTypeString(objectWrapperData.mObjectData.projectileType));
            ImGui::Text("Current Map: %s", network::GetCurrentMapString(objectWrapperData.mObjectData).c_str());
            ImGui::Text("Object Speed: %.4f", objectWrapperData.mObjectData.speed);
            ImGui::Text("Object Scale: %.4f", objectWrapperData.mObjectData.objectScale);
            ImGui::Text("Action Timer: %.4f", objectWrapperData.mObjectData.actionTimer);
            ImGui::Text("Object position: %.4f, %.4f, %.4f", objectWrapperData.mObjectData.position.x, objectWrapperData.mObjectData.position.y, objectWrapperData.mObjectData.position.z);
            ImGui::Text("Object velocity: %.4f, %.4f, %.4f", objectWrapperData.mObjectData.velocity.x, objectWrapperData.mObjectData.velocity.y, objectWrapperData.mObjectData.velocity.z);
            ImGui::Text("Object Speed: %.4f", objectWrapperData.mObjectData.speed);
            const auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
            const auto& mapName = strutils::StringId(network::GetCurrentMapString(objectWrapperData.mObjectData));
            if (mMapResourceController && mMapResourceController->GetAllLoadedMapResources().contains(mapName) && mMapResourceController->GetAllLoadedMapResources().at(mapName).mMapResourcesState == MapResourcesState::LOADED)
            {
                const auto& mapDefinition = globalMapDataRepo.GetMapDefinition(mapName);
                auto navmap = mMapResourceController->GetAllLoadedMapResources().at(mapName).mNavmap;
                auto currentNavmapCoords = navmap->GetNavmapCoord(objectWrapperData.mSceneObjects.front()->mPosition, mapDefinition.mMapPosition, network::MAP_GAME_SCALE);
                auto currentNavmapTileType = navmap->GetNavmapTileAt(currentNavmapCoords);
                
                ImGui::Text("Navmap Tile: x:%d, y:%d", currentNavmapCoords.x, currentNavmapCoords.y);
                ImGui::Text("Navmap Type: %s", network::GetNavmapTileTypeName(currentNavmapTileType));
            }
            
            ImGui::PopID();
        }
    }
    ImGui::End();
    
    static bool sShowNavmap = false;

    ImGui::Begin("Map", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Current Map: %s", mCurrentMap.GetString().c_str());
    if (ImGui::Checkbox("Show Navmap", &sShowNavmap))
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

    ImGui::Checkbox("Show Quadtree", &sShowQuadtree);
    ImGui::Checkbox("Show Object Paths", &sShowObjectPaths);
    
    if (ImGui::Checkbox("Show Grid", &sShowDebugGrid))
    {
        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
        
        auto bottomLayerSceneObjects = scene->FindSceneObjectsWhoseNameEndsWith("_bottom");
        auto topLayerSceneObjects = scene->FindSceneObjectsWhoseNameEndsWith("_top");
        
        for (auto so: bottomLayerSceneObjects) so->mShaderBoolUniformValues[MAP_DEBUG_GRID_UNIFORM_NAME] = sShowDebugGrid;
        for (auto so: topLayerSceneObjects) so->mShaderBoolUniformValues[MAP_DEBUG_GRID_UNIFORM_NAME] = sShowDebugGrid;
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
