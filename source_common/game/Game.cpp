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
#include <engine/rendering/Fonts.h>
#include <engine/rendering/ParticleManager.h>
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
#include <fstream>
#include <game/AnimatedButton.h>
#include <game/Game.h>
#include <game/events/EventSystem.h>
#include <game/PlayerController.h>
#include <game/utils/NameGenerator.h>
#include <imgui/imgui.h>
#include <mutex>
#include <net_common/WorldObjectTypes.h>
#include <net_common/WorldObjectStates.h>

#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId MAIN_MENU_SCENE = strutils::StringId("main_menu_scene");
static const strutils::StringId PLAY_BUTTON_NAME = strutils::StringId("play_button");
static const float ENEMY_SPEED = 0.0002f;
static std::mutex sWorldMutex;

///------------------------------------------------------------------------------------------------

glm::vec3 GetRGBAt(SDL_Surface* surface, const int x, const int y)
{
    Uint8 r,g,b;
    auto pixel = *(Uint32*)((Uint8*)surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel);
    SDL_GetRGB(pixel, surface->format, &r, &g, &b);
    return glm::vec3(r/255.0f, g/255.0f, b/255.0f);
}

///------------------------------------------------------------------------------------------------

Game::Game(const int argc, char** argv)
    : mStateSendingDelayMillis(game_constants::STATE_SEND_MAX_DELAY_MILLIS)
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
SDL_Surface* sNavmapSurface = nullptr;

void Game::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(strutils::StringId("world"));
    scene->SetLoaded(true);
    
    auto background = scene->CreateSceneObject(strutils::StringId("forest"));
    background->mPosition.z = 0.0f;
    background->mScale *= game_constants::MAP_SCALE;
    background->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/maps/map_tower.png");
    
    auto navmapResourceID = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/maps/map_tower_navmap.png");
    sNavmapSurface = systemsEngine.GetResourceLoadingService().GetResource<resources::ImageSurfaceResource>(navmapResourceID).GetSurface();
    
    mLocalPlayerSceneObject = nullptr;
    mPlayButton = std::make_unique<AnimatedButton>(glm::vec3(-0.057f, 0.038f, 1.0f), glm::vec3(0.001f, 0.001f, 0.001f), game_constants::DEFAULT_FONT_NAME, "Play", PLAY_BUTTON_NAME, [&](){ OnPlayButtonPressed(); }, *scene);
    mPlayButton->GetSceneObject()->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 1.0f;
    
    mPlayerController = std::make_unique<PlayerController>();
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mSendNetworkMessageEventListener = eventSystem.RegisterForEvent<events::SendNetworkMessageEvent>([=](const events::SendNetworkMessageEvent& event)
    {
        SendNetworkMessage(event.mMessageJson, event.mMessageType, event.mIsHighPriority);
    });
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    UpdateGUI(dtMillis);
    CheckForPendingWorldObjectsToBeCreated();
    InterpolateLocalWorld(dtMillis);
    CheckForStateSending(dtMillis);
    UpdateCamera(dtMillis);
}

///------------------------------------------------------------------------------------------------

void Game::UpdateCamera(const float)
{
    if (mLocalPlayerSceneObject)
    {
        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto scene = systemsEngine.GetSceneManager().FindScene(strutils::StringId("world"));
        scene->GetCamera().SetPosition(glm::vec3(mLocalPlayerSceneObject->mPosition.x, mLocalPlayerSceneObject->mPosition.y, scene->GetCamera().GetPosition().z));
    }
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

#if defined(USE_IMGUI)
void Game::CreateDebugWidgets()
{
    //static float sPlayerSpeedMultiplier = 1.0f;
    
    ImGui::Begin("Net Stats", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Ping %d millis", mLastPingMillis.load());
    ImGui::Text("State sending %d millis", static_cast<int>(mStateSendingDelayMillis));
    ImGui::End();
//    
//    ImGui::Begin("Game Hacks", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
//    if (ImGui::SliderFloat("Player Speed Multiplier", &sPlayerSpeedMultiplier, 0.1f, 3.0f))
//    {
//        PLAYER_SPEED = 0.0002f * sPlayerSpeedMultiplier;
//    }
//    ImGui::End();
    
    
    ImGui::Begin("Enemy NPC Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    std::lock_guard<std::mutex> worldGuard(sWorldMutex);
    
    for (auto& objectData: mWorldObjectData)
    {
        if (objectData.objectType == networking::OBJ_TYPE_NPC_ENEMY)
        {
            if (ImGui::CollapsingHeader(std::to_string(objectData.objectId).c_str(), ImGuiTreeNodeFlags_None))
            {
                ImGui::PushID(std::to_string(objectData.objectId).c_str());
                
                switch (objectData.objectState)
                {
                    case networking::OBJ_STATE_ALIVE:
                    {
                        ImGui::Text("State: Alive");
                        ImGui::Text("Speed: %.4f, %.4f", objectData.objectVelocity.x, objectData.objectVelocity.y);
                    } break;
                        
                    case networking::OBJ_STATE_CHASING:
                    {
                        ImGui::Text("State: Chasing %llu", objectData.parentObjectId);
                        ImGui::Text("Speed: %.4f, %.4f", objectData.objectVelocity.x, objectData.objectVelocity.y);
                    } break;
                        
                    case networking::OBJ_STATE_DEAD:
                    {
                        ImGui::Text("State: Dead");
                    } break;
                }
                ImGui::PopID();
            }
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

void Game::UpdateGUI(const float dtMillis)
{
    if (mPlayButton)
    {
        mPlayButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void Game::CheckForPendingWorldObjectsToBeCreated()
{
    // Pending world object creation
    std::lock_guard<std::mutex> worldGuard(sWorldMutex);

    for (auto worldObjectData: mPendingWorldObjectDataToCreate)
    {
        worldObjectData.invalidated = false;
        CreateWorldObject(worldObjectData);
    }
    mPendingWorldObjectDataToCreate.clear();
}

///------------------------------------------------------------------------------------------------

void Game::InterpolateLocalWorld(const float dtMillis)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    
    std::lock_guard<std::mutex> worldLockGuard(sWorldMutex);

    auto scene = sceneManager.FindScene(strutils::StringId("world"));
    
    for (int worldObjectIDToCleanup: mWorldObjectIDsToCleanup)
    {
        scene->RemoveSceneObject(strutils::StringId(worldObjectIDToCleanup));
        scene->RemoveSceneObject(strutils::StringId(std::to_string(worldObjectIDToCleanup) + "_name"));
    }
    mWorldObjectIDsToCleanup.clear();
    
    for (auto& objectData: mWorldObjectData)
    {
        switch (objectData.objectType)
        {
            case networking::OBJ_TYPE_PLAYER:
            {
                auto playerSceneObject = scene->FindSceneObject(strutils::StringId(objectData.objectId));
                auto playerNameSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(objectData.objectId) + "_name"));
                
                if (objectData.isLocal)
                {
                    mPlayerController->Update(dtMillis, strutils::StringId(objectData.objectId), objectData, *scene);
                }
                else
                {
                    auto directionToTarget = objectData.objectPosition - playerSceneObject->mPosition;
                    auto distanceToTarget = glm::length(directionToTarget);
                    
                    // If (unlikely) we the player is exactly on target, or if
                    // the player's movement delta vector length exceeds the distance to target
                    // we teleport to target
                    if (distanceToTarget <= 0.0f || distanceToTarget < glm::length(glm::normalize(directionToTarget) * game_constants::PLAYER_SPEED * dtMillis))
                    {
                        playerSceneObject->mPosition = objectData.objectPosition;
                        playerNameSceneObject->mPosition = playerSceneObject->mPosition + game_constants::PLAYER_NAMEPLATE_OFFSET;
                        
                        auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*playerNameSceneObject);
                        auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
                        playerNameSceneObject->mPosition.x -= textLength/2.0f;
                    }
                    else
                    {
                        playerSceneObject->mPosition += glm::normalize(directionToTarget) * game_constants::PLAYER_SPEED * dtMillis;
                        playerNameSceneObject->mPosition += glm::normalize(directionToTarget) * game_constants::PLAYER_SPEED * dtMillis;
                    }
                }
            } break;
                 
            case networking::OBJ_TYPE_NPC_ENEMY:
            {
                auto npcSceneObject = scene->FindSceneObject(strutils::StringId(objectData.objectId));
                if (objectData.objectState == networking::OBJ_STATE_ALIVE)
                {
                    npcSceneObject->mPosition = objectData.objectPosition;
                }
                else if (objectData.objectState == networking::OBJ_STATE_CHASING)
                {
                    auto playerObjectDataIter = std::find_if(mWorldObjectData.begin(), mWorldObjectData.end(), [&](const networking::WorldObjectData& otherObjectData){ return otherObjectData.objectId == objectData.parentObjectId; });
                    if (playerObjectDataIter != mWorldObjectData.end() && playerObjectDataIter->objectState == networking::OBJ_STATE_ALIVE)
                    {
                        auto& playerObjectData = *playerObjectDataIter;
                        objectData.objectVelocity = glm::normalize(playerObjectData.objectPosition - npcSceneObject->mPosition) * ENEMY_SPEED;
                        objectData.objectVelocity.z = 0.0f;
                        npcSceneObject->mPosition += objectData.objectVelocity * dtMillis;
                    }
                    else
                    {
                        npcSceneObject->mPosition = objectData.objectPosition;
                    }
                }
            } break;
                
                
            case networking::OBJ_TYPE_NPC_SHURIKEN:
            {
                if (objectData.objectState != networking::OBJ_STATE_DEAD)
                {
                    auto npcSceneObject = scene->FindSceneObject(strutils::StringId(objectData.objectId));
                    
                    objectData.objectPosition += objectData.objectVelocity * dtMillis;
                    npcSceneObject->mPosition = objectData.objectPosition;
                }
            } break;
                
            default:
            {
                assert(false);
            } break;
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::CheckForStateSending(const float dtMillis)
{
    static float stateSendingTimer = 0.0f;
    mStateSendingDelayMillis = mLastPingMillis.load() < game_constants::STATE_SEND_MIN_DELAY_MILLIS ? game_constants::STATE_SEND_MIN_DELAY_MILLIS : game_constants::STATE_SEND_MAX_DELAY_MILLIS;

    stateSendingTimer += dtMillis;
    if (stateSendingTimer > mStateSendingDelayMillis)
    {
        stateSendingTimer -= mStateSendingDelayMillis;
        
        std::lock_guard<std::mutex> worldLockGuard(sWorldMutex);
        auto localPlayerFoundIter = std::find_if(mWorldObjectData.begin(), mWorldObjectData.end(), [&](networking::WorldObjectData& worldObjectEntry){ return worldObjectEntry.objectType == networking::OBJ_TYPE_PLAYER && worldObjectEntry.isLocal; });
        
        // Local player is not found in local world data.
        // Send an empty state just to request other world entities
        if (localPlayerFoundIter == mWorldObjectData.end())
        {
            SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_PLAYER_STATE, false);
        }
        // Local player found in world data. Send its updated state
        else
        {
            SendNetworkMessage(localPlayerFoundIter->SerializeToJson(), networking::MessageType::CS_PLAYER_STATE, false);
        }
        
    }
}

///------------------------------------------------------------------------------------------------

void Game::SendNetworkMessage(const nlohmann::json& message, const networking::MessageType messageType, const bool highPriority)
{
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::SendNetworkMessage(message, messageType, highPriority, [&](const networking::ServerResponseData& responseData)
    {
        if (!responseData.mError.empty())
        {
            logging::Log(logging::LogType::ERROR, responseData.mError.c_str());
        }
        else
        {
            mLastPingMillis = static_cast<int>(responseData.mPingMillis);
            OnServerResponse(responseData.mResponse);
        }
    });
#elif defined(WINDOWS)
    windows_utils::SendNetworkMessage(message, messageType, highPriority, [&](const networking::ServerResponseData& responseData)
    {
        if (!responseData.mError.empty())
        {
            logging::Log(logging::LogType::ERROR, responseData.mError.c_str());
        }
        else
        {
            mLastPingMillis = static_cast<int>(responseData.mPingMillis);
            OnServerResponse(responseData.mResponse);
        }
    });
#endif
}

///------------------------------------------------------------------------------------------------

void Game::CreateWorldObject(const networking::WorldObjectData& worldObjectData)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(strutils::StringId("world"));
    
    switch (worldObjectData.objectType)
    {
        case networking::OBJ_TYPE_PLAYER:
        {
            auto ninja = scene->CreateSceneObject(strutils::StringId(worldObjectData.objectId));
            ninja->mPosition = worldObjectData.objectPosition;
            ninja->mScale /= 10.0f;
            ninja->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "portrait.vs");
            ninja->mTextureResourceId =  systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/portrait.png");
            ninja->mShaderFloatUniformValues[strutils::StringId("portrait_value")] = worldObjectData.color;
            
            auto ninjaName = scene->CreateSceneObject(strutils::StringId(std::to_string(worldObjectData.objectId) + "_name"));
            scene::TextSceneObjectData textSceneObjectData;
            textSceneObjectData.mFontName = game_constants::DEFAULT_FONT_NAME;
            textSceneObjectData.mText = worldObjectData.objectName.GetString();
            
            ninjaName->mScale /= 3000.0f;
            ninjaName->mPosition = ninja->mPosition;
            ninjaName->mSceneObjectTypeData = std::move(textSceneObjectData);
            ninjaName->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "portrait.vs");
            ninjaName->mShaderFloatUniformValues[strutils::StringId("portrait_value")] = worldObjectData.color;
            
            auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*ninjaName);
            auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
            ninjaName->mPosition += game_constants::PLAYER_NAMEPLATE_OFFSET;
            ninjaName->mPosition.x -= textLength/2.0f;
            
            if (worldObjectData.isLocal)
            {
                mLocalPlayerSceneObject = ninja;
            }
        } break;
          
        case networking::OBJ_TYPE_NPC_ENEMY:
        {
            auto worldObject = scene->CreateSceneObject(strutils::StringId(worldObjectData.objectId));
            worldObject->mPosition = worldObjectData.objectPosition;
            worldObject->mScale /= 13.0f;
            worldObject->mTextureResourceId =  systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/portrait_enemy.png");
        } break;
            
        case networking::OBJ_TYPE_NPC_SHURIKEN:
        {
            auto worldObject = scene->CreateSceneObject(strutils::StringId(worldObjectData.objectId));
            worldObject->mPosition = worldObjectData.objectPosition;
            worldObject->mScale /= 30.0f;
            worldObject->mTextureResourceId =  systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/shuriken.png");
        } break;
            
        default:
        {
            assert(false);
        } break;
    }
    
    mWorldObjectData.emplace_back(worldObjectData);
}

///------------------------------------------------------------------------------------------------

void Game::OnServerResponse(const std::string& response)
{
    if (nlohmann::json::accept(response))
    {
        auto responseJson = nlohmann::json::parse(response);
        //logging::Log(logging::LogType::INFO, responseJson.dump(4).c_str());
        
        if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_PLAYER_STATE_RESPONSE))
        {
            OnServerPlayerStateResponse(responseJson);
        }
        else if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_REQUEST_LOGIN_RESPONSE))
        {
            OnServerLoginResponse(responseJson);
        }
        else if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_THROW_RANGED_WEAPON_RESPONSE))
        {
        }
        else
        {
            logging::Log(logging::LogType::ERROR, "Unrecognised message type %d", static_cast<int>(networking::GetMessageType(responseJson)));
        }
    }
    else
    {
        logging::Log(logging::LogType::ERROR, "Error parsing world state");
    }
    
}

void Game::OnServerPlayerStateResponse(const nlohmann::json& responseJson)
{
    std::lock_guard<std::mutex> worldLockGuard(sWorldMutex);
    
    // Invalidate all local world object data
    for (auto& worldObjectData: mWorldObjectData)
    {
        worldObjectData.invalidated = true;
    }
    
    // Parse and update local world object data validating them
    for (const auto& worldObjectJson: responseJson[networking::WorldObjectData::ObjectCollectionHeader()])
    {
        networking::WorldObjectData remoteWorldObjectData;
        remoteWorldObjectData.DeserializeFromJson(worldObjectJson);
        
        auto worldObjectIter = std::find_if(mWorldObjectData.begin(), mWorldObjectData.end(), [&](networking::WorldObjectData& worldObjectData)
        {
            return worldObjectData.objectId == remoteWorldObjectData.objectId;
        });
        
        if (worldObjectIter == mWorldObjectData.end())
        {
            mPendingWorldObjectDataToCreate.push_back(remoteWorldObjectData);
        }
        else
        {
            auto& worldObjectData = *worldObjectIter;
            
            // Local position is not updated (for now)
            if (worldObjectData.objectType != networking::OBJ_TYPE_PLAYER || !worldObjectData.isLocal)
            {
                worldObjectData.objectPosition = remoteWorldObjectData.objectPosition;
                worldObjectData.objectVelocity = remoteWorldObjectData.objectVelocity;
            }
            
            worldObjectData.parentObjectId = remoteWorldObjectData.parentObjectId;
            worldObjectData.objectState = remoteWorldObjectData.objectState;
            worldObjectData.invalidated = false;
        }
    }
    
    // Clean up all player data entries that the server does not know of (anymore)
    for (auto worldObjectIter = mWorldObjectData.begin(); worldObjectIter != mWorldObjectData.end();)
    {
        auto& worldObjectData = *worldObjectIter;
        if (worldObjectData.invalidated)
        {
            mWorldObjectIDsToCleanup.push_back(strutils::StringId(worldObjectData.objectId));
            worldObjectIter = mWorldObjectData.erase(worldObjectIter);
        }
        else
        {
            worldObjectIter++;
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnServerLoginResponse(const nlohmann::json& responseJson)
{
    networking::LoginResponse loginResponse;
    loginResponse.DeserializeFromJson(responseJson);
    
    if (loginResponse.allowed)
    {
        std::lock_guard<std::mutex> worldLockGuard(sWorldMutex);
        
        networking::WorldObjectData worldObjectData = {};
        worldObjectData.objectId = loginResponse.playerId;
        worldObjectData.objectName = loginResponse.playerName;
        worldObjectData.objectPosition = loginResponse.playerPosition;
        worldObjectData.color = loginResponse.color;
        worldObjectData.objectType = networking::OBJ_TYPE_PLAYER;
        worldObjectData.objectState = networking::OBJ_STATE_ALIVE;
        worldObjectData.isLocal = true;
        worldObjectData.invalidated = false;
        mPendingWorldObjectDataToCreate.push_back(worldObjectData);
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnPlayButtonPressed()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(strutils::StringId("world"));
    
    // Fade button out
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mPlayButton->GetSceneObject()->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 0.0f, 0.2f), [=]()
    {
        scene->RemoveSceneObject(PLAY_BUTTON_NAME);
        mPlayButton = nullptr;
    });
    
    // Request login details
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_REQUEST_LOGIN, true);
}

///------------------------------------------------------------------------------------------------
