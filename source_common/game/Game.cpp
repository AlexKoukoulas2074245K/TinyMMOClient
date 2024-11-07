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
static const strutils::StringId POKER_TABLE_NAME = strutils::StringId("poker_table");

static const glm::vec3 TABLE_SCALE = glm::vec3(1.5f * 1.0f, 1.0f, 1.0f);

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
    scene->GetCamera().SetZoomFactor(50.0f);
    scene->SetLoaded(true);
    
    auto table = scene->CreateSceneObject(POKER_TABLE_NAME);
    table->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/poker_table.png");
    table->mScale = TABLE_SCALE;
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mSendNetworkMessageEventListener = eventSystem.RegisterForEvent<events::SendNetworkMessageEvent>([this](const events::SendNetworkMessageEvent& event)
    {
        SendNetworkMessage(event.mMessageJson, event.mMessageType, event.mMessagePriority);
    });
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    // Dequeue and process any pending server messages
    while (mQueuedServerResponses.size() > 0)
    {
        OnServerResponse(std::move(mQueuedServerResponses.dequeue()));
    }
    
    // Update rest of game logic
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
}
#else
void Game::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------

void Game::UpdateGUI(const float dtMillis)
{
    
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
            mQueuedServerResponses.enqueue(std::move(responseData.mResponse));
        }
    });
#elif defined(WINDOWS)
    windows_utils::SendNetworkMessage(message, messageType, messagePriority == networking::MessagePriority::HIGH, [&](const networking::ServerResponseData& responseData)
    {
        if (!responseData.mError.empty())
        {
            logging::Log(logging::LogType::ERROR, responseData.mError.c_str());
        }
        else
        {
            mLastPingMillis = static_cast<int>(responseData.mPingMillis);
            mQueuedServerResponses.enqueue(std::move(responseData.mResponse));
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
//        auto& systemsEngine = CoreSystemsEngine::GetInstance();
//        auto& sceneManager = systemsEngine.GetSceneManager();
//        auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
//        
//        // Fade button out
//        for (auto& sceneObject: mPlayButton->GetSceneObjects())
//        {
//            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(sceneObject->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 0.0f, 0.2f), [this, scene, sceneObject]()
//            {
//                scene->RemoveSceneObject(sceneObject->mName);
//                mPlayButton = nullptr;
//            });
//        }
//        
//        networking::WordRequest wordRequest = {};
//        wordRequest.sourceLanguge = sSourceLanguage;
//        wordRequest.targetLanguage = sTargetLanguage;
//        SendNetworkMessage(wordRequest.SerializeToJson(), networking::MessageType::CS_WORD_REQUEST, networking::MessagePriority::HIGH);
    }
}

///------------------------------------------------------------------------------------------------
