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
#include <fstream>
#include <game/AnimatedButton.h>
#include <game/BoardView.h>
#include <game/Game.h>
#include <game/events/EventSystem.h>
#include <imgui/imgui.h>
#include <mutex>
#include <SDL.h>

#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId LOGIN_BUTTON_NAME = strutils::StringId("login_button");
static const strutils::StringId SPIN_BUTTON_NAME = strutils::StringId("spin_button");
static const strutils::StringId SPIN_BUTTON_EFFECT_NAME = strutils::StringId("spin_button_effect");
static const strutils::StringId BACKGROUND_NAME = strutils::StringId("background");
static const strutils::StringId BACKGROUND_MASK_NAME = strutils::StringId("background_mask");

static const glm::vec3 BACKGROUND_SCALE = glm::vec3(1.370f, 1.04f, 1.0f);

static const glm::vec3 ACTION_TEXT_SCALE = glm::vec3(0.00056f);
static const glm::vec3 SPIN_BUTTON_SCALE = glm::vec3(0.156f);
static const glm::vec3 SPIN_BUTTON_EFFECT_SCALE = glm::vec3(0.224f);

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

    auto background = scene->CreateSceneObject(BACKGROUND_NAME);
    background->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/background.png");

    background->mPosition.z = 1.0f;
    background->mScale = BACKGROUND_SCALE;
    background->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    background->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/background_mask.png");

    mLoginButton = std::make_unique<AnimatedButton>(glm::vec3(-0.075f, 0.134f, 2.0f), ACTION_TEXT_SCALE, game_constants::DEFAULT_FONT_NAME, "Log in", LOGIN_BUTTON_NAME, [&](){ OnLoginButtonPressed(); }, *scene);
    for (auto& sceneObject: mLoginButton->GetSceneObjects())
    {
        sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    }
    
    auto& eventSystem = events::EventSystem::GetInstance();
    mSendNetworkMessageEventListener = eventSystem.RegisterForEvent<events::SendNetworkMessageEvent>([this](const events::SendNetworkMessageEvent& event)
    {
        SendNetworkMessage(event.mMessageJson, event.mMessageType, event.mMessagePriority);
    });
    
    mPlayerId = 0;
    mSpinId = 0;
}

///------------------------------------------------------------------------------------------------

void Game::Update(const float dtMillis)
{
    // Dequeue and process any pending server messages
    while (mQueuedServerErrors.size() > 0)
    {
        OnServerError(std::move(mQueuedServerErrors.dequeue()));
    }
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
    ImGui::Text("Current Spin ID: %d", mSpinId);
    ImGui::Text("Spin Animation State: %s", mBoardView ? mBoardView->GetSpinAnimationStateName().c_str() : "IDLE");
    
    if (ImGui::Button("Refill Board"))
    {
        if (mBoardView)
        {
            mSpinId = math::RandomInt();
            mBoardModel.PopulateBoard(mSpinId);
            mBoardView->ResetBoardSymbols();
        }
    }
    
    static const std::unordered_map<slots::SymbolType, std::string> DEBUG_SYMBOL_NAMES =
    {
        { slots::SymbolType::BUTTER, "Butter" },
        { slots::SymbolType::CAMP_FIRE, "CampFire" },
        { slots::SymbolType::CHICKEN, "Chicken" },
        { slots::SymbolType::CHOCOLATE, "Chocolate" },
        { slots::SymbolType::COOKING_OIL, "CookingOil" },
        { slots::SymbolType::EGGS, "Eggs" },
        { slots::SymbolType::FLOUR, "Flour" },
        { slots::SymbolType::GARLICS, "Garlics" },
        { slots::SymbolType::LEMONS, "Lemons" },
        { slots::SymbolType::STRAWBERRIES, "Strawberries" },
        { slots::SymbolType::SUGAR, "Sugar" },
        { slots::SymbolType::CHOCOLATE_CAKE, "ChocolateCake" },
        { slots::SymbolType::STRAWBERRY_CAKE, "StrawberryCake" },
        { slots::SymbolType::ROAST_CHICKEN, "RoastChicken" },
        { slots::SymbolType::WILD, "Grandma" }
    };
    ImGui::Separator();
    if (ImGui::BeginTable("Pending Symbol State", slots::BOARD_COLS))
    {
        ImGui::TableNextRow();
        for (int column = 0; column < slots::BOARD_COLS; column++)
        {
            ImGui::TableSetColumnIndex(column);
            ImGui::Text("%s", mBoardView ? mBoardView->GetPendingSymbolDataStateName(column).c_str() : "LOCKED");
        }
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("Board View", slots::BOARD_COLS))
    {
        for (int row = 0; row < slots::REEL_LENGTH; row++)
        {
            ImGui::TableNextRow();
            for (int column = 0; column < slots::BOARD_COLS; column++)
            {
                ImGui::TableSetColumnIndex(column);
                
                if (row <= 2 || row >= 6)
                {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", DEBUG_SYMBOL_NAMES.at(mBoardModel.GetBoardSymbol(row, column)).c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", DEBUG_SYMBOL_NAMES.at(mBoardModel.GetBoardSymbol(row, column)).c_str());
                }
            }
        }
        ImGui::EndTable();
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
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
    
    if (mLoginButton)
    {
        mLoginButton->Update(dtMillis);
    }
    
    auto spinButton = scene->FindSceneObject(SPIN_BUTTON_NAME);
    auto spinButtonEffect = scene->FindSceneObject(SPIN_BUTTON_EFFECT_NAME);

    if (spinButton)
    {
        while (spinButton->mRotation.z < -2.0f * math::PI)
        {
            spinButton->mRotation.z += 2.0f * math::PI;
        }
        
        if (animationManager.GetAnimationCountPlayingForSceneObject(SPIN_BUTTON_NAME) == 0 && mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
        {
            if (spinButtonEffect->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] < 1.0f)
            {
                spinButtonEffect->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] += dtMillis/1000.0f;
            }
            
            auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
            
            auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*spinButton);
            bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
            
            if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
            {
                auto initScale = spinButton->mScale;
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(spinButton, spinButton->mPosition, initScale * 0.8f, 0.15f), [this, initScale, spinButton]()
                {
                    OnSpinButtonPressed();
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(spinButton, spinButton->mPosition, initScale, 0.15f, animation_flags::NONE), [](){});
                });
                
                auto currentRotation = spinButton->mRotation;
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(spinButton, glm::vec3(currentRotation.x, currentRotation.y, currentRotation.z - 2.0f * math::PI), 1.0f), [](){});
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(spinButtonEffect, 0.0f, 0.5f), [](){});
            }
        }
    }
    
    if (spinButtonEffect)
    {
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("time")] = time;
    }
    
    if (mBoardView)
    {
        mBoardView->Update(dtMillis);
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
            mQueuedServerErrors.enqueue(responseData.mError);
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

void Game::OnServerError(const std::string& error)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
    
    scene::TextSceneObjectData textData;
    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textData.mText = error;
    
    auto soName = strutils::StringId("Error: " + std::to_string(SDL_GetTicks()));
    auto errorTextSceneObject = scene->CreateSceneObject(soName);
    errorTextSceneObject->mSceneObjectTypeData = std::move(textData);
    errorTextSceneObject->mPosition = glm::vec3(0.0f, -0.1f, 5.0f);
    errorTextSceneObject->mScale = glm::vec3(0.00036f);
    
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*errorTextSceneObject);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    errorTextSceneObject->mPosition.x -= textLength/2.0f;
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(errorTextSceneObject, glm::vec3(errorTextSceneObject->mPosition.x, 1.0f, errorTextSceneObject->mPosition.z), errorTextSceneObject->mScale, 1.0f, animation_flags::NONE, 0.2f), [=](){
        scene->RemoveSceneObject(soName);
    });
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
        else if (networking::IsMessageOfType(responseJson, networking::MessageType::SC_SPIN_RESPONSE))
        {
            OnServerSpinResponse(responseJson);
        }
        else
        {
            logging::Log(logging::LogType::ERROR, "Unrecognised message type %d", static_cast<int>(networking::GetMessageType(responseJson)));
        }
    }
    else
    {
        logging::Log(logging::LogType::ERROR, "Error parsing server response");
    }
    
}

///------------------------------------------------------------------------------------------------

void Game::OnServerLoginResponse(const nlohmann::json& responseJson)
{
    networking::LoginResponse loginResponse;
    loginResponse.DeserializeFromJson(responseJson);
    
    if (loginResponse.allowed)
    {
        mPlayerId = loginResponse.playerId;

        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto& sceneManager = systemsEngine.GetSceneManager();
        auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
        
        // Fade button out
        for (auto& sceneObject: mLoginButton->GetSceneObjects())
        {
            scene->RemoveSceneObject(sceneObject->mName);
        }
        mLoginButton = nullptr;
        
        auto background = scene->FindSceneObject(BACKGROUND_NAME);
        background->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "background.vs");
        
        auto spinButton = scene->CreateSceneObject(SPIN_BUTTON_NAME);
        spinButton->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/wheel.png");
        spinButton->mPosition = glm::vec3(0.403f, 0.0f, 2.0f);
        spinButton->mScale = SPIN_BUTTON_SCALE;
        spinButton->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(spinButton, 1.0f, 0.5f), [](){});
        
        auto spinButtonEffect = scene->CreateSceneObject(SPIN_BUTTON_EFFECT_NAME);
        spinButtonEffect->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "spin_button_effect.vs");
        spinButtonEffect->mPosition = glm::vec3(0.403f, 0.0f, 1.95f);
        spinButtonEffect->mScale = SPIN_BUTTON_EFFECT_SCALE;
        spinButtonEffect->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("time_speed")] = 5.162f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_resolution")] = 312.0f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_clarity")] = 5.23f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(spinButtonEffect, 1.0f, 0.5f), [](){});

        mBoardView = std::make_unique<BoardView>(*scene, mBoardModel);
        
        for (auto i = 0; i < static_cast<int>(slots::SymbolType::COUNT); ++i)
        {
            CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + mBoardView->GetSymbolTexturePath(static_cast<slots::SymbolType>(i)));
        }
    }
}

///------------------------------------------------------------------------------------------------

void Game::OnServerSpinResponse(const nlohmann::json& responseJson)
{
    networking::SpinResponse spinResponse;
    spinResponse.DeserializeFromJson(responseJson);
    
    mSpinId = spinResponse.spinId;
    mBoardView->ResetBoardSymbols();
    mBoardModel.PopulateBoard(mSpinId);
    mBoardView->BeginSpin();
}

///------------------------------------------------------------------------------------------------

void Game::OnLoginButtonPressed()
{
    // Request quick play
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_LOGIN_REQUEST, networking::MessagePriority::HIGH);
}

///------------------------------------------------------------------------------------------------

void Game::OnSpinButtonPressed()
{
    logging::Log(logging::LogType::INFO, "Spin!");
    
    // Request quick play
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_SPIN_REQUEST, networking::MessagePriority::HIGH);
}

///------------------------------------------------------------------------------------------------
