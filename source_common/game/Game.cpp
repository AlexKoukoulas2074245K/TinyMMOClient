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
#include <fstream>
#include <game/AnimatedButton.h>
#include <game/Game.h>
#include <game/events/EventSystem.h>
#include <imgui/imgui.h>
#include <mutex>

#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId PLAY_BUTTON_NAME = strutils::StringId("play_button");

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

void Game::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(game_constants::WORLD_SCENE_NAME);
    scene->SetLoaded(true);
    
    mPlayButton = std::make_unique<AnimatedButton>(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0002f, 0.0002f, 0.0002f), game_constants::DEFAULT_FONT_NAME, "Play", PLAY_BUTTON_NAME, [&](){ OnPlayButtonPressed(); }, *scene);
    mPlayButton->GetSceneObject()->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 1.0f;
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mSendNetworkMessageEventListener = eventSystem.RegisterForEvent<events::SendNetworkMessageEvent>([=](const events::SendNetworkMessageEvent& event)
    {
        SendNetworkMessage(event.mMessageJson, event.mMessageType, event.mMessagePriority);
    });
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    UpdateGUI(dtMillis);
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
    ImGui::Begin("Net Stats", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Ping %d millis", mLastPingMillis.load());
    ImGui::End();
//    
//    ImGui::Begin("Game Hacks", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
//    if (ImGui::SliderFloat("Player Speed Multiplier", &sPlayerSpeedMultiplier, 0.1f, 3.0f))
//    {
//        PLAYER_SPEED = 0.0002f * sPlayerSpeedMultiplier;
//    }
//    ImGui::End();
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

void Game::SendNetworkMessage(const nlohmann::json& message, const networking::MessageType messageType, const networking::MessagePriority messagePriority)
{
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::SendNetworkMessage(message, messageType, messagePriority, [&](const networking::ServerResponseData& responseData)
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
    windows_utils::SendNetworkMessage(message, messageType, messagePriority, [&](const networking::ServerResponseData& responseData)
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

void Game::OnServerResponse(const std::string& response)
{
    if (nlohmann::json::accept(response))
    {
        auto responseJson = nlohmann::json::parse(response);
        //logging::Log(logging::LogType::INFO, responseJson.dump(4).c_str());
        
        if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_LOGIN_RESPONSE))
        {
            OnServerLoginResponse(responseJson);
        }
        else if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_WORD_RESPONSE))
        {
            OnServerWordResponse(responseJson);
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

///------------------------------------------------------------------------------------------------

void Game::OnServerLoginResponse(const nlohmann::json& responseJson)
{
    networking::LoginResponse loginResponse;
    loginResponse.DeserializeFromJson(responseJson);
    
    if (loginResponse.allowed)
    {
        networking::WordRequest wordRequest = {};
        wordRequest.sourceLanguge = "Greek";
        wordRequest.targetLanguage = "Chinese";
        SendNetworkMessage(wordRequest.SerializeToJson(), networking::MessageType::CS_WORD_REQUEST, networking::MessagePriority::HIGH);
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnServerWordResponse(const nlohmann::json& responseJson)
{
    networking::WordResponse wordResponse = {};
    wordResponse.DeserializeFromJson(responseJson);
    
    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::WORLD_SCENE_NAME);
    
    std::make_unique<AnimatedButton>(glm::vec3(0.0f, 0.1f, 1.0f), glm::vec3(0.0002f, 0.0002f, 0.0002f), game_constants::DEFAULT_FONT_NAME, wordResponse.sourceWord, strutils::StringId(wordResponse.sourceWord), [&](){  }, *scene);
    std::make_unique<AnimatedButton>(glm::vec3(-0.2f, 0.0f, 1.0f), glm::vec3(0.0002f, 0.0002f, 0.0002f), game_constants::DEFAULT_FONT_NAME, wordResponse.choices[0], strutils::StringId(wordResponse.choices[0]), [&](){  }, *scene);
    std::make_unique<AnimatedButton>(glm::vec3(0.2f, 0.0f, 1.0f), glm::vec3(0.0002f, 0.0002f, 0.0002f), game_constants::DEFAULT_FONT_NAME, wordResponse.choices[1], strutils::StringId(wordResponse.choices[1]), [&](){ }, *scene);
    std::make_unique<AnimatedButton>(glm::vec3(-0.2f, -0.2f, 1.0f), glm::vec3(0.0002f, 0.0002f, 0.0002f), game_constants::DEFAULT_FONT_NAME, wordResponse.choices[2], strutils::StringId(wordResponse.choices[2]), [&](){ }, *scene);
    std::make_unique<AnimatedButton>(glm::vec3(0.2f, -0.2f, 1.0f), glm::vec3(0.0002f, 0.0002f, 0.0002f), game_constants::DEFAULT_FONT_NAME, wordResponse.choices[3], strutils::StringId(wordResponse.choices[3]), [&](){}, *scene);
}

///------------------------------------------------------------------------------------------------

void Game::OnPlayButtonPressed()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
    
    // Fade button out
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mPlayButton->GetSceneObject()->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 0.0f, 0.2f), [=]()
    {
        scene->RemoveSceneObject(PLAY_BUTTON_NAME);
        mPlayButton = nullptr;
    });
    
    // Request login details
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_LOGIN_REQUEST, networking::MessagePriority::HIGH);
}

///------------------------------------------------------------------------------------------------
