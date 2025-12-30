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
#include <game/PlayerAnimationController.h>
#include <imgui/imgui.h>
#include <net_common/NetworkMessages.h>
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
static ENetPeer* sPeer;

void Game::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(game_constants::WORLD_SCENE_NAME);
    scene->GetCamera().SetZoomFactor(50.0f);
    scene->SetLoaded(true);
    
    auto bg = scene->CreateSceneObject(strutils::StringId("background"));
    bg->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/entry_map_bottom_layer.png");
    bg->mPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    bg->mScale = glm::vec3(5.0f, 5.0f, 0.5f);
    
    mPlayerAnimationController = std::make_unique<PlayerAnimationController>();
    mLocalPlayerId = 0;

    enet_initialize();
    atexit(enet_deinitialize);
    
    sClient = enet_host_create(nullptr, 1, 2, 0, 0);

    ENetAddress address{};
    enet_address_set_host(&address, "127.0.0.1");
    address.port = 7777;

    sPeer = enet_host_connect(sClient, &address, 2, 0);
    if (!sPeer)
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
static const float sPlayerSpeed = 0.0003f;

void Game::Update(const float dtMillis)
{
    ENetEvent event;
    while (enet_host_service(sClient, &event, 0) > 0)
    {
        if (event.type == ENET_EVENT_TYPE_RECEIVE)
        {
            auto messageType = static_cast<network::MessageType>(event.packet->data[0]);
            switch (messageType)
            {
                case network::MessageType::ObjectStateUpdateMessage:
                {
                    auto* message = reinterpret_cast<network::ObjectStateUpdateMessage*>(event.packet->data);
                    
                    // Pre-existing object
                    if (!mLocalObjectDataMap.contains(message->objectData.objectId))
                    {
                        CreateObject(message->objectData);
                    }
                    
                    // Update everything but local player's data (for now)
                    if (message->objectData.objectId != mLocalPlayerId)
                    {
                        mLocalObjectDataMap[message->objectData.objectId] = message->objectData;
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
                    mPlayerAnimationController->OnPlayerDisconnected(strutils::StringId("object-" + std::to_string(message->objectId)));
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
                    
                case network::MessageType::AttackMessage:
                case network::MessageType::UNUSED:
                    break;
            }
            
            enet_packet_destroy(event.packet);
        }
    }
    
    for (const auto& [objectId, objectData]: mLocalObjectDataMap)
    {
        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        
        auto sceneObject = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->FindSceneObject(strutils::StringId("object-" + std::to_string(objectId)));
        
        assert(sceneObject);

        if (objectId == mLocalPlayerId)
        {
            auto inputDirection = LocalPlayerInputController::GetMovementDirection();
            auto velocity = glm::vec3(inputDirection.x, inputDirection.y, 0.0f) * sPlayerSpeed * sDebugPlayerVelocityMultiplier * dtMillis;
            
            const auto& animationInfoResult = mPlayerAnimationController->UpdatePlayerAnimation(sceneObject, sPlayerSpeed * sDebugPlayerVelocityMultiplier, velocity, dtMillis);
            sceneObject->mPosition += velocity;
            
            mLocalObjectDataMap[mLocalPlayerId].position = sceneObject->mPosition;
            mLocalObjectDataMap[mLocalPlayerId].velocity = velocity;
            mLocalObjectDataMap[mLocalPlayerId].animationIndex = animationInfoResult.mAnimationIndex;
            
            network::ObjectStateUpdateMessage stateUpdateMessage = {};
            stateUpdateMessage.objectData = mLocalObjectDataMap[mLocalPlayerId];
            
            SendMessage(sPeer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::UNRELIABLE);
        }
        else
        {
            auto vecToPosition = mLocalObjectDataMap.at(objectId).position - sceneObject->mPosition;
            if (glm::length(vecToPosition) > 0.002f)
            {
                auto direction = glm::normalize(vecToPosition);
                auto velocity = glm::vec3(direction.x, direction.y, 0.0f) * sPlayerSpeed * sDebugPlayerVelocityMultiplier * dtMillis;
                sceneObject->mPosition += velocity;
            }
            
            mPlayerAnimationController->UpdatePlayerAnimation(sceneObject, sPlayerSpeed * sDebugPlayerVelocityMultiplier, mLocalObjectDataMap.at(objectId).velocity, dtMillis, mLocalObjectDataMap.at(objectId).animationIndex);
        }
    }

    enet_host_flush(sClient);
}

///------------------------------------------------------------------------------------------------

void Game::ApplicationMovedToBackground()
{
}

///------------------------------------------------------------------------------------------------

void Game::OnOneSecondElapsed()
{
}

///------------------------------------------------------------------------------------------------

void Game::WindowResize()
{
}

///------------------------------------------------------------------------------------------------

void Game::CreateObject(const network::ObjectData& objectData)
{
    mLocalObjectDataMap[objectData.objectId] = objectData;
    auto sceneObjectName = strutils::StringId("object-" + std::to_string(objectData.objectId));

    auto sceneObject = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->FindSceneObject(sceneObjectName);
    
    if (sceneObject)
    {
        logging::Log(logging::LogType::WARNING, "Attempted to re-create pre-existing object %s", sceneObjectName.GetString().c_str());
    }
    else
    {
        sceneObject = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->CreateSceneObject(sceneObjectName);
        switch (objectData.objectType)
        {
            case network::ObjectType::PLAYER:
            {
                sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/char.png");
                sceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "player.vs");
                sceneObject->mShaderBoolUniformValues[IS_TEXTURE_SHEET_UNIFORM_NAME] = true;
                sceneObject->mShaderBoolUniformValues[strutils::StringId("is_local")] = objectData.objectId == mLocalPlayerId;
                sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                sceneObject->mScale = glm::vec3(0.1f, 0.1f, 0.1f);
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

void Game::DestroyObject(const network::objectId_t objectId)
{
    mLocalObjectDataMap.erase(objectId);
    CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->RemoveSceneObject(strutils::StringId("object-" + std::to_string(objectId)));
}

///------------------------------------------------------------------------------------------------

#if defined(USE_IMGUI)
void Game::CreateDebugWidgets()
{
    ImGui::Begin("Game Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Local Player Id: %llu", mLocalPlayerId);
    ImGui::SliderFloat("Player velocity Multiplier", &sDebugPlayerVelocityMultiplier, 0.01f, 10.0f);
    ImGui::End();
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------
