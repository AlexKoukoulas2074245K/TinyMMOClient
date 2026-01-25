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
static ENetPeer* sServer;

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
    
    
    scene = systemsEngine.GetSceneManager().CreateScene(game_constants::GUI_SCENE_NAME);
    scene->GetCamera().SetZoomFactor(50.0f);
    scene->SetLoaded(true);
    
    scene::TextSceneObjectData textData;
    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textData.mText = "Health Points: 100";
    auto guiSceneObject = scene->CreateSceneObject(strutils::StringId("gui"));
    guiSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
    guiSceneObject->mSceneObjectTypeData = std::move(textData);
    guiSceneObject->mPosition = glm::vec3(-0.192f, -0.235f, 1.0f);
    guiSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    guiSceneObject->mScale = glm::vec3(0.0004f);
    
    mTestButton = std::make_unique<AnimatedButton>(glm::vec3(-0.2f, 0.0f, 1.0f), glm::vec3(0.0001f), game_constants::DEFAULT_FONT_NAME, "Test my limits, left and right :)", strutils::StringId("test_button"), [](){}, *scene);
    
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
                    events::EventSystem::GetInstance().DispatchEvent<events::ObjectDestroyedEvent>(strutils::StringId("object-" + std::to_string(message->objectId)));

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
            // Attacking overrides movement direction
            bool hasAttacked = false;
            if (CoreSystemsEngine::GetInstance().GetInputStateManager().VButtonTapped(input::Button::MAIN_BUTTON))
            {
                // Cooldown checks etc..
                hasAttacked = true;
                const auto& cam = systemsEngine.GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME)->GetCamera();
                const auto& pointingPos = CoreSystemsEngine::GetInstance().GetInputStateManager().VGetPointingPosInWorldSpace(cam.GetViewMatrix(), cam.GetProjMatrix());
                const auto& playerToPointingPos = glm::normalize(glm::vec3(pointingPos.x, pointingPos.y, objectData.position.z) - objectData.position);
                const auto facingDirection = VecToDirection(playerToPointingPos);
                
                mObjectAnimationController->UpdateObjectAnimation(sceneObject, glm::vec3(0.0f), dtMillis, facingDirection);
                mLocalObjectDataMap[mLocalPlayerId].facingDirection = facingDirection;
                
                network::ObjectStateUpdateMessage stateUpdateMessage = {};
                stateUpdateMessage.objectData = mLocalObjectDataMap[mLocalPlayerId];
                
                SendMessage(sServer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::RELIABLE);
                
                network::AttackMessage attackMessage = {};
                attackMessage.attackerId = mLocalPlayerId;
                attackMessage.attackType = network::AttackType::PROJECTILE;
                attackMessage.projectileType = network::ProjectileType::FIREBALL;

                SendMessage(sServer, &attackMessage, sizeof(attackMessage), network::channels::RELIABLE);
            }
            
            if (!hasAttacked)
            {
                auto inputDirection = LocalPlayerInputController::GetMovementDirection();
                auto velocity = glm::vec3(inputDirection.x, inputDirection.y, 0.0f) * objectData.speed * sDebugPlayerVelocityMultiplier * dtMillis;
                
                const auto& animationInfoResult = mObjectAnimationController->UpdateObjectAnimation(sceneObject, velocity, dtMillis, std::nullopt);
                sceneObject->mPosition += velocity;
                
                mLocalObjectDataMap[mLocalPlayerId].position = sceneObject->mPosition;
                mLocalObjectDataMap[mLocalPlayerId].velocity = velocity;
                mLocalObjectDataMap[mLocalPlayerId].currentAnimation = network::AnimationType::RUNNING;
                mLocalObjectDataMap[mLocalPlayerId].facingDirection = animationInfoResult.mFacingDirection;
                
                network::ObjectStateUpdateMessage stateUpdateMessage = {};
                stateUpdateMessage.objectData = mLocalObjectDataMap[mLocalPlayerId];
                
                SendMessage(sServer, &stateUpdateMessage, sizeof(stateUpdateMessage), network::channels::UNRELIABLE);
            }
        }
        else
        {
            auto vecToPosition = mLocalObjectDataMap.at(objectId).position - sceneObject->mPosition;
            if (glm::length(vecToPosition) > 0.002f)
            {
                auto direction = glm::normalize(vecToPosition);
                auto velocity = glm::vec3(direction.x, direction.y, 0.0f) * objectData.speed * dtMillis;
                sceneObject->mPosition += velocity;
            }
            
            if (objectData.objectType != network::ObjectType::ATTACK || objectData.attackType != network::AttackType::PROJECTILE)
            {
                mObjectAnimationController->UpdateObjectAnimation(sceneObject, mLocalObjectDataMap.at(objectId).velocity, dtMillis, mLocalObjectDataMap.at(objectId).facingDirection);
            }
        }
    }

    enet_host_flush(sClient);
    
    mTestButton->Update(dtMillis);
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
            
            case network::ObjectType::ATTACK:
            {
                if (objectData.attackType == network::AttackType::PROJECTILE && objectData.projectileType == network::ProjectileType::FIREBALL)
                {
                    sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/projectile.png");
                    sceneObject->mPosition = glm::vec3(objectData.position.x, objectData.position.y, objectData.position.z);
                    sceneObject->mScale = glm::vec3(0.03f, 0.03f, 0.03f);
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
    ImGui::SeparatorText("Network Object Data");
    for (const auto& [objectId, objectData]: mLocalObjectDataMap)
    {
        auto name = ("object-" + std::to_string(objectId));
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_None))
        {
            ImGui::PushID(name.c_str());
            ImGui::Text("Object Type: %d", static_cast<int>(objectData.objectType));
            ImGui::Text("Facing Direction: %d", static_cast<int>(objectData.facingDirection));
            ImGui::PopID();
        }
    }
    ImGui::End();
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------
