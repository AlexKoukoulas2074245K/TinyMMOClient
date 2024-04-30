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
#include <game/Game.h>
#include <game/events/EventSystem.h>
#include <game/utils/NameGenerator.h>
#include <imgui/imgui.h>
#include <mutex>
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>

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
static float PLAYER_SPEED = 0.0002f;
static std::mutex sWorldMutex;

///------------------------------------------------------------------------------------------------

Game::Game(const int argc, char** argv)
    : mCanSendNetworkMessage(true)
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

void Game::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(strutils::StringId("world"));
    scene->SetLoaded(true);
    
    auto background = scene->CreateSceneObject(strutils::StringId("forest"));
    background->mPosition.z = 0.0f;
    background->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/japanese_forest.png");
    
    auto pos = glm::vec3(math::RandomFloat(-0.3f, 0.3f), math::RandomFloat(-0.15f, 0.15f), 0.1f);
    auto color = math::RandomFloat(0.0f, 1.0f);
    auto name = GenerateName();
    
    networking::PlayerData playerData = { strutils::StringId(name), pos, {}, color, true };
    CreatePlayerWorldObject(playerData);
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    InterpolateLocalWorld(dtMillis);
    CheckForStateSending(dtMillis);
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
    static float sPlayerSpeedMultiplier = 1.0f;
    
    ImGui::Begin("Net Stats", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Ping %d millis", mLastPingMillis.load());
    ImGui::End();
    
    ImGui::Begin("Game Hacks", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    if (ImGui::SliderFloat("Player Speed Multiplier", &sPlayerSpeedMultiplier, 0.1f, 3.0f))
    {
        PLAYER_SPEED = 0.0002f * sPlayerSpeedMultiplier;
    }
    ImGui::End();
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------

void Game::CreatePlayerWorldObject(const networking::PlayerData& playerData)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(strutils::StringId("world"));
    auto ninja = scene->CreateSceneObject(playerData.playerName);
    ninja->mPosition = playerData.playerPosition;
    ninja->mScale /= 10.0f;
    ninja->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "portrait.vs");
    ninja->mTextureResourceId =  systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/portrait.png");
    ninja->mShaderFloatUniformValues[strutils::StringId("portrait_value")] = playerData.color;
    
    auto ninjaName = scene->CreateSceneObject(strutils::StringId(playerData.playerName.GetString() + "_name"));
    
    scene::TextSceneObjectData textSceneObjectData;
    textSceneObjectData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textSceneObjectData.mText = playerData.playerName.GetString();
    
    ninjaName->mScale /= 3000.0f;
    ninjaName->mPosition = ninja->mPosition;
    ninjaName->mSceneObjectTypeData = std::move(textSceneObjectData);
    ninjaName->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "portrait.vs");
    ninjaName->mShaderFloatUniformValues[strutils::StringId("portrait_value")] = playerData.color;
    
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*ninjaName);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    ninjaName->mPosition += game_constants::PLAYER_NAMEPLATE_OFFSET;
    ninjaName->mPosition.x -= textLength/2.0f;
    
    mPlayerData.emplace_back(playerData);
}

///------------------------------------------------------------------------------------------------

void Game::InterpolateLocalWorld(const float dtMillis)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& inputStateManager = systemsEngine.GetInputStateManager();
    auto& sceneManager = systemsEngine.GetSceneManager();
    
    std::lock_guard<std::mutex> worldLockGuard(sWorldMutex);

    auto scene = sceneManager.FindScene(strutils::StringId("world"));
    
    for (auto& playerNameToCleanup: playerNamesToCleanup)
    {
        scene->RemoveSceneObject(playerNameToCleanup);
        scene->RemoveSceneObject(strutils::StringId(playerNameToCleanup.GetString() + "_name"));
    }
    playerNamesToCleanup.clear();
    
    for (auto& playerData: mPlayerData)
    {
        auto playerSceneObject = scene->FindSceneObject(playerData.playerName);
        auto playerNameSceneObject = scene->FindSceneObject(strutils::StringId(playerData.playerName.GetString() + "_name"));
        
        if (playerData.isLocal)
        {
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
            
            playerData.playerVelocity = {};
            auto length = glm::length(impulseVector);
            if (length > 0.0f)
            {
                playerData.playerVelocity = glm::normalize(impulseVector) * PLAYER_SPEED * dtMillis;
                playerData.playerPosition += playerData.playerVelocity;
                
                playerSceneObject->mPosition += playerData.playerVelocity;
                playerNameSceneObject->mPosition += playerData.playerVelocity;
            }
        }
        else
        {
            auto directionToTarget = playerData.playerPosition - playerSceneObject->mPosition;
            auto distanceToTarget = glm::length(directionToTarget);
            
            // If (unlikely) we the player is exactly on target, or if
            // the player's movement delta vector length exceeds the distance to target
            // we teleport to target
            if (distanceToTarget <= 0.0f || distanceToTarget < glm::length(glm::normalize(directionToTarget) * PLAYER_SPEED * dtMillis))
            {
                playerSceneObject->mPosition = playerData.playerPosition;
                playerNameSceneObject->mPosition = playerSceneObject->mPosition + game_constants::PLAYER_NAMEPLATE_OFFSET;
                
                auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*playerNameSceneObject);
                auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
                playerNameSceneObject->mPosition.x -= textLength/2.0f;
            }
            else
            {
                playerSceneObject->mPosition += glm::normalize(directionToTarget) * PLAYER_SPEED * dtMillis;
                playerNameSceneObject->mPosition += glm::normalize(directionToTarget) * PLAYER_SPEED * dtMillis;
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::CheckForStateSending(const float dtMillis)
{
    static float stateSendingTimer = 0.0f;
    stateSendingTimer += dtMillis;
    if (stateSendingTimer > game_constants::STATE_SEND_DELAY_MILLIS)
    {
        stateSendingTimer -= game_constants::STATE_SEND_DELAY_MILLIS;
        
        if (!mCanSendNetworkMessage)
        {
            return;
        }
        
        std::lock_guard<std::mutex> worldLockGuard(sWorldMutex);
        auto playerIter = std::find_if(mPlayerData.begin(), mPlayerData.end(), [&](networking::PlayerData& playerData){ return playerData.isLocal; });
        
        assert(playerIter != mPlayerData.end());
        auto& playerData = *playerIter;
        
#if defined(MACOS) || defined(MOBILE_FLOW)        
        apple_utils::SendNetworkMessage(playerData.SerializeToJson(), networking::MessageType::CS_PLAYER_STATE, [&](const apple_utils::ServerResponseData& responseData)
        {
            mCanSendNetworkMessage = true;
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
        mCanSendNetworkMessage = false;
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnServerResponse(const std::string& response)
{
    if (nlohmann::json::accept(response))
    {
        auto responseJson = nlohmann::json::parse(response);
        //logging::Log(logging::LogType::INFO, worldStateJson.dump(4).c_str());
        
        if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_PLAYER_STATE_RESPONSE))
        {
            OnServerPlayerStateResponse(responseJson);
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
    
    // Invalidate all local player data
    for (auto& playerData: mPlayerData)
    {
        playerData.invalidated = true;
    }
    
    // Parse and update local player data validating them
    for (const auto& playerJson: responseJson[networking::PlayerData::ObjectCollectionHeader()])
    {
        networking::PlayerData remotePlayerData;
        remotePlayerData.DeserializeFromJson(playerJson);
        
        auto playerIter = std::find_if(mPlayerData.begin(), mPlayerData.end(), [&](networking::PlayerData& playerData){ return playerData.playerName == remotePlayerData.playerName; });
        if (playerIter == mPlayerData.end())
        {
            CreatePlayerWorldObject(remotePlayerData);
        }
        else
        {
            auto& playerData = *playerIter;
            
            // Local position is not updated (for now)
            if (!remotePlayerData.isLocal)
            {
                playerData.playerPosition = remotePlayerData.playerPosition;
                playerData.playerVelocity = remotePlayerData.playerVelocity;
            }
            
            playerData.invalidated = false;
        }
    }
    
    // Clean up all player data entries that the server does not know of (anymore)
    for (auto iter = mPlayerData.begin(); iter != mPlayerData.end();)
    {
        if (iter->invalidated)
        {
            playerNamesToCleanup.push_back(iter->playerName);
            iter = mPlayerData.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

///------------------------------------------------------------------------------------------------
