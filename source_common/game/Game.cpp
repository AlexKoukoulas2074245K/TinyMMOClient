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
#include <net_common/BestHandFinder.h>

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
static const strutils::StringId PLAYER_HOLE_CARD_0_NAME = strutils::StringId("player_hole_card_0");
static const strutils::StringId PLAYER_HOLE_CARD_1_NAME = strutils::StringId("player_hole_card_1");
static const strutils::StringId OPPONENT_HOLE_CARD_0_NAME = strutils::StringId("opponent_hole_card_0");
static const strutils::StringId OPPONENT_HOLE_CARD_1_NAME = strutils::StringId("opponent_hole_card_1");
static const strutils::StringId BEST_HAND_TEXT_NAME = strutils::StringId("best_hand_text");

static const std::string COMMUNITY_CARD_NAME_PREFIX = "community_card_";
static const std::string CARD_BACK_TEXTURE_FILE_PATH = "game/cards/back_0.png";

static const glm::vec3 BEST_HAND_TEXT_SCALE = glm::vec3(0.00016f);
static const glm::vec3 ACTION_TEXT_SCALE = glm::vec3(0.00056f);
static const glm::vec3 TABLE_SCALE = glm::vec3(1.5f * 1.0f, 1.0f, 1.0f);

static const float TABLE_REQUEST_DELAY_SECS = 50.0f;

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
    table->mPosition.z = -0.2f;
    table->mScale = TABLE_SCALE;
    
    mPlayButton = std::make_unique<AnimatedButton>(glm::vec3(-0.075f, 0.134f, 1.0f), ACTION_TEXT_SCALE, game_constants::DEFAULT_FONT_NAME, "Play!", PLAY_BUTTON_NAME, [&](){ OnPlayButtonPressed(); }, *scene);
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mSendNetworkMessageEventListener = eventSystem.RegisterForEvent<events::SendNetworkMessageEvent>([this](const events::SendNetworkMessageEvent& event)
    {
        SendNetworkMessage(event.mMessageJson, event.mMessageType, event.mMessagePriority);
    });
    
    mTableId = 0;
    mPlayerId = 0;
    mTableStateRequestTimer = TABLE_REQUEST_DELAY_SECS;
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
    
    ImGui::Begin("Debug Data", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("Player ID: %lld", mPlayerId);
    ImGui::Text("Table ID: %lld", mTableId);
    ImGui::Text("Round State Name: %s", mRoundStateName.c_str());
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
    
    if (mPlayerId != 0 && mTableId != 0)
    {
        mTableStateRequestTimer -= dtMillis;
        if (mTableStateRequestTimer <= 0.0f)
        {
            mTableStateRequestTimer += TABLE_REQUEST_DELAY_SECS;
            
            networking::TableStateRequest tableStateRequest = {};
            tableStateRequest.playerId = mPlayerId;
            tableStateRequest.tableId = mTableId;
            
            SendNetworkMessage(tableStateRequest.SerializeToJson(), networking::MessageType::CS_TABLE_STATE_REQUEST, networking::MessagePriority::HIGH);
        }
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
        
        if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_PLAY_RESPONSE))
        {
            OnServerPlayResponse(responseJson);
        }
        else if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_TABLE_STATE_RESPONSE))
        {
            OnServerTableStateResponse(responseJson);
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

void Game::OnServerPlayResponse(const nlohmann::json& responseJson)
{
    networking::PlayResponse playResponse;
    playResponse.DeserializeFromJson(responseJson);
    
    if (playResponse.allowed)
    {
        mPlayerId = playResponse.playerId;
        mTableId = playResponse.tableId;

        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto& sceneManager = systemsEngine.GetSceneManager();
        auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
        
        // Fade button out
        for (auto& sceneObject: mPlayButton->GetSceneObjects())
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(sceneObject->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 0.0f, 0.2f), [this, scene, sceneObject]()
            {
                scene->RemoveSceneObject(sceneObject->mName);
                mPlayButton = nullptr;
            });
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnServerTableStateResponse(const nlohmann::json& responseJson)
{
    networking::TableStateResponse tableStateResponse;
    tableStateResponse.DeserializeFromJson(responseJson);
    
    mRoundStateName = tableStateResponse.roundStateName;
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
    
    if (!scene->FindSceneObject(PLAYER_HOLE_CARD_0_NAME) && !scene->FindSceneObject(PLAYER_HOLE_CARD_1_NAME))
    {
        auto playerHoleCardsSplitByComma = strutils::StringSplit(tableStateResponse.holeCards[0], ',');
        mHoleCards.emplace_back(poker::Card(playerHoleCardsSplitByComma[0]));
        mHoleCards.emplace_back(poker::Card(playerHoleCardsSplitByComma[1]));
        
        auto holeCard0 = scene->CreateSceneObject(PLAYER_HOLE_CARD_0_NAME);
        holeCard0->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.0f;
        holeCard0->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/cards/" + playerHoleCardsSplitByComma[0] + ".png");
        holeCard0->mPosition = glm::vec3(-0.05f, -0.2f, 1.5f);
        holeCard0->mScale = glm::vec3(0.13f * 0.7f, 0.13f, 1.0f);
        
        systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(holeCard0->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 1.0f, 0.2f), [](){});
        
        auto holeCard1 = scene->CreateSceneObject(PLAYER_HOLE_CARD_1_NAME);
        holeCard1->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.0f;
        holeCard1->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/cards/" + playerHoleCardsSplitByComma[1] + ".png");
        holeCard1->mPosition = glm::vec3(0.05f, -0.2f, 1.5f);
        holeCard1->mScale = glm::vec3(0.13f * 0.7f, 0.13f, 1.0f);
        
        systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(holeCard1->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 1.0f, 0.2f), [](){});
    }
    
    if (!scene->FindSceneObject(OPPONENT_HOLE_CARD_0_NAME) && !scene->FindSceneObject(OPPONENT_HOLE_CARD_1_NAME))
    {
        auto playerHoleCardsSplitByComma = strutils::StringSplit(tableStateResponse.holeCards[1], ',');
        
        auto holeCard0 = scene->CreateSceneObject(OPPONENT_HOLE_CARD_0_NAME);
        holeCard0->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.0f;
        holeCard0->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_BACK_TEXTURE_FILE_PATH);
        holeCard0->mPosition = glm::vec3(-0.05f, 0.2f, 1.5f);
        holeCard0->mScale = glm::vec3(0.13f * 0.7f, 0.13f, 1.0f);
        
        systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(holeCard0->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 1.0f, 0.2f), [](){});
        
        auto holeCard1 = scene->CreateSceneObject(OPPONENT_HOLE_CARD_1_NAME);
        holeCard1->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.0f;
        holeCard1->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_BACK_TEXTURE_FILE_PATH);
        holeCard1->mPosition = glm::vec3(0.05f, 0.2f, 1.5f);
        holeCard1->mScale = glm::vec3(0.13f * 0.7f, 0.13f, 1.0f);
        
        systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(holeCard1->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 1.0f, 0.2f), [](){});
    }
    
    const auto& communityCardsSplitByComma = strutils::StringSplit(tableStateResponse.communityCards, ',');
    for (int i = 0; i < communityCardsSplitByComma.size(); ++i)
    {
        auto communityCardName = strutils::StringId(COMMUNITY_CARD_NAME_PREFIX + std::to_string(i));
        if (!scene->FindSceneObject(communityCardName))
        {
            mCommunityCards.emplace_back(poker::Card(communityCardsSplitByComma[i]));
            
            auto communityCard = scene->CreateSceneObject(communityCardName);
            communityCard->mShaderFloatUniformValues[strutils::StringId("custom_alpha")] = 0.0f;
            communityCard->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/cards/" + communityCardsSplitByComma[i] + ".png");
            communityCard->mPosition = glm::vec3(-0.2f + 0.1f * i, 0.0f, 1.5f);
            communityCard->mScale = glm::vec3(0.13f * 0.7f, 0.13f, 1.0f);
            
            systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(communityCard->mShaderFloatUniformValues[strutils::StringId("custom_alpha")], 1.0f, 0.2f), [](){});
        }
    }
    
    if (tableStateResponse.roundStateName == "WAITING_FOR_ACTIONS_POSTRIVER")
    {
        if (!scene->FindSceneObject(BEST_HAND_TEXT_NAME))
        {
            scene::TextSceneObjectData textData;
            textData.mFontName = game_constants::DEFAULT_FONT_NAME;
            textData.mText = "Best Hand: " + poker::GetHandKindString(poker::BestHandFinder::FindBestHand({mHoleCards[0], mHoleCards[1], mCommunityCards[0], mCommunityCards[1], mCommunityCards[2], mCommunityCards[3], mCommunityCards[4]}).GetHandKind());
            
            auto bestHandText = scene->CreateSceneObject(BEST_HAND_TEXT_NAME);
            bestHandText->mSceneObjectTypeData = std::move(textData);
            bestHandText->mPosition = glm::vec3(0.2f, -0.2f, 1.5f);
            bestHandText->mScale = BEST_HAND_TEXT_SCALE;
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnPlayButtonPressed()
{
    // Request quick play
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_PLAY_REQUEST, networking::MessagePriority::HIGH);
}

///------------------------------------------------------------------------------------------------
