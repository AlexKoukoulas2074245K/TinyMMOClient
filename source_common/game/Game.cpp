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

static const strutils::StringId LOGIN_BUTTON_NAME = strutils::StringId("login_button");
static const strutils::StringId SPIN_BUTTON_NAME = strutils::StringId("spin_button");
static const strutils::StringId SPIN_BUTTON_EFFECT_NAME = strutils::StringId("spin_button_effect");
static const strutils::StringId BACKGROUND_NAME = strutils::StringId("background");
static const strutils::StringId BACKGROUND_MASK_NAME = strutils::StringId("background_mask");
static const strutils::StringId CREDITS_NAME = strutils::StringId("credits");
static const strutils::StringId CREDITS_WAGER_NAME = strutils::StringId("credits_wager");
static const strutils::StringId CREDIT_UPDATE_ANIMATION_NAME = strutils::StringId("credit_update_animation");
static const strutils::StringId CREDITS_WAGER_PLUS_BUTTON_NAME = strutils::StringId("credit_wager_plus");
static const strutils::StringId CREDITS_WAGER_MINUS_BUTTON_NAME = strutils::StringId("credit_wager_minus");

static const glm::vec3 BACKGROUND_SCALE = glm::vec3(1.370f, 1.04f, 1.0f);

static const glm::vec3 ACTION_TEXT_SCALE = glm::vec3(0.00056f);
static const glm::vec3 CREDITS_TEXT_SCALE = glm::vec3(0.00032f);
static const glm::vec3 SPIN_BUTTON_SCALE = glm::vec3(0.156f);
static const glm::vec3 SPIN_BUTTON_EFFECT_SCALE = glm::vec3(0.224f);
static const glm::vec3 SPIN_BUTTON_EFFECT_POSITION = glm::vec3(0.403f, -0.05f, 1.9f);
static const glm::vec3 SPIN_BUTTON_POSITION = glm::vec3(0.403f, -0.05f, 2.0f);
static const glm::vec3 BACKGROUND_POSITION = glm::vec3(0.0f, 0.0f, 1.0f);
static const glm::vec3 LOGIN_BUTTON_POSITION = glm::vec3(-0.075f, 0.134f, 2.0f);
static const glm::vec3 CREDITS_TEXT_POSITION = glm::vec3(-0.3f, 0.292f, 2.0f);
static const glm::vec3 CREDITS_WAGER_TEXT_POSITION = glm::vec3(0.05f, 0.292f, 2.0f);
static const glm::vec3 CREDITS_WAGER_PLUS_BUTTON_POSITION = glm::vec3(0.35f, 0.15f, 2.0f);
static const glm::vec3 CREDITS_WAGER_MINUS_BUTTON_POSITION = glm::vec3(0.45f, 0.15f, 2.0f);

static const float GAME_ELEMENTS_PRESENTATION_TIME = 1.0f;
static const float PAYLINE_REVEAL_ANIMATION_DURATION = 1.0f;
static const float PAYLINE_HIDE_ANIMATION_DURATION = 0.5f;
static const float PAYLINE_ANIMATION_DURATION = PAYLINE_REVEAL_ANIMATION_DURATION + PAYLINE_HIDE_ANIMATION_DURATION;
static const float SPIN_BUTTON_DEPRESSED_SCALE_FACTOR = 0.85f;
static const float SPIN_BUTTON_ANIMATION_DURATION = 0.15f;
static const float SPIN_BUTTON_EFFECT_ANIMATION_DURATION = 0.5f;
static const float CREDIT_UPDATE_ANIMATION_DURATION = 1.0f;


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

    background->mPosition = BACKGROUND_POSITION;
    background->mScale = BACKGROUND_SCALE;
    background->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    background->mShaderFloatUniformValues[strutils::StringId("mask_alpha_comp")] = 1.0f;
    background->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/background_mask.png");
    background->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "background.vs");
    
    scene::TextSceneObjectData textData;
    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textData.mText = "Credits: " + std::to_string(mCredits);
    
    auto creditsSceneObject = scene->CreateSceneObject(CREDITS_NAME);
    creditsSceneObject->mSceneObjectTypeData = std::move(textData);
    creditsSceneObject->mPosition = CREDITS_TEXT_POSITION;
    creditsSceneObject->mScale = CREDITS_TEXT_SCALE;
    creditsSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    scene::TextSceneObjectData creditsWagerTextData;
    creditsWagerTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
    creditsWagerTextData.mText = "Wager Per Spin: " + std::to_string(mCreditsWagerPerSpin);
    
    auto creditsWagerSceneObject = scene->CreateSceneObject(CREDITS_WAGER_NAME);
    creditsWagerSceneObject->mSceneObjectTypeData = std::move(creditsWagerTextData);
    creditsWagerSceneObject->mPosition = CREDITS_WAGER_TEXT_POSITION;
    creditsWagerSceneObject->mScale = CREDITS_TEXT_SCALE;
    creditsWagerSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    mLoginButton = std::make_unique<AnimatedButton>(LOGIN_BUTTON_POSITION, ACTION_TEXT_SCALE, game_constants::DEFAULT_FONT_NAME, "Log in", LOGIN_BUTTON_NAME, [&](){ OnLoginButtonPressed(); }, *scene);
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
    mCredits = 100;
    mCreditsWagerPerSpin = 1;
    mDisplayedCredits = 100.0f;
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
    ImGui::SameLine();
    if (ImGui::Button("Copy to Clipboard"))
    {
        SDL_SetClipboardText(std::to_string(mSpinId).c_str());
    }
    ImGui::Text("Spin Animation State: %s", mBoardView ? mBoardView->GetSpinAnimationStateName().c_str() : "IDLE");
    
    if (ImGui::Button("Refill Board"))
    {
        if (mBoardView)
        {
            mSpinId = math::RandomInt();
            mBoardModel.PopulateBoardForSpin(mSpinId);
            mBoardView->ResetBoardSymbols();
        }
    }
    
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
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", slots::Board::GetSymbolDebugName(mBoardModel.GetBoardSymbol(row, column)).c_str());
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", slots::Board::GetSymbolDebugName(mBoardModel.GetBoardSymbol(row, column)).c_str());
                }
            }
        }
        ImGui::EndTable();
    }
    
    ImGui::End();
    
    ImGui::Begin("Paylines", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    static int sPaylineIndex = 0;
    static float sRevealDurationSecs = 1.0f;
    static float sHiddingDurationSecs = 0.5f;
    static std::vector<std::string> sPaylines;

    if (sPaylines.empty())
    {
        for (int i = 0; i < static_cast<int>(slots::PaylineType::PAYLINE_COUNT); ++i)
        {
            sPaylines.emplace_back(PaylineView::GetPaylineName(static_cast<slots::PaylineType>(i)));
        }
    }
    
    if (ImGui::BeginCombo(" ", sPaylines.at(sPaylineIndex).c_str()))
    {
        for (auto n = 0U; n < sPaylines.size(); n++)
        {
            const bool isSelected = (sPaylineIndex == n);
            if (ImGui::Selectable(sPaylines.at(n).c_str(), isSelected))
            {
                sPaylineIndex = n;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SliderFloat("Reveal Duration(s)", &sRevealDurationSecs, 0.01f, 5.0f);
    ImGui::SliderFloat("Hiding Duration(s)", &sHiddingDurationSecs, 0.01f, 5.0f);
    if (ImGui::Button("Animate Payline"))
    {
        if (mBoardView)
        {
            slots::PaylineResolutionData paylineResolutionData;
            paylineResolutionData.mPayline = static_cast<slots::PaylineType>(sPaylineIndex);
            mBoardView->AnimatePaylineReveal(paylineResolutionData, sRevealDurationSecs, sHiddingDurationSecs);
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
    
    if (mCreditsWagerPlusButton && mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
    {
        mCreditsWagerPlusButton->Update(dtMillis);
    }
    
    if (mCreditsWagerMinusButton && mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
    {
        mCreditsWagerMinusButton->Update(dtMillis);
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
            
            if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) && mCredits >= mCreditsWagerPerSpin)
            {
                auto initScale = spinButton->mScale;
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(spinButton, spinButton->mPosition, initScale * SPIN_BUTTON_DEPRESSED_SCALE_FACTOR, SPIN_BUTTON_ANIMATION_DURATION), [this, initScale, spinButton]()
                {
                    OnSpinButtonPressed();
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(spinButton, spinButton->mPosition, initScale, SPIN_BUTTON_ANIMATION_DURATION, animation_flags::NONE), [](){});
                });
                
                auto currentRotation = spinButton->mRotation;
                animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(spinButton, glm::vec3(currentRotation.x, currentRotation.y, currentRotation.z - 2.0f * math::PI), 1.0f), [](){});
                
                animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(spinButtonEffect, 0.0f, SPIN_BUTTON_EFFECT_ANIMATION_DURATION), [](){});
            }
        }
    }
    
    if (spinButtonEffect)
    {
        spinButtonEffect->mShaderFloatUniformValues[TIME_UNIFORM_NAME] = time;
    }
    
    if (mBoardView)
    {
        mBoardView->Update(dtMillis);
        
        if (mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::POST_SPINNING)
        {
            const auto& boardStateResolutionData = mBoardModel.ResolveBoardState();
            if (boardStateResolutionData.mTotalWinMultiplier <= 0)
            {
                mBoardView->CompleteSpin();
            }
            else
            {
                mBoardView->WaitForPaylines(boardStateResolutionData);
                for (int i = 0; i < boardStateResolutionData.mWinningPaylines.size(); ++i)
                {
                    mBoardView->AnimatePaylineReveal(boardStateResolutionData.mWinningPaylines[i], PAYLINE_REVEAL_ANIMATION_DURATION, PAYLINE_HIDE_ANIMATION_DURATION, i * PAYLINE_ANIMATION_DURATION);
                    
                    const auto& wonCreditMultiplier = boardStateResolutionData.mWinningPaylines[i].mWinMultiplier;
                    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(PAYLINE_ANIMATION_DURATION * (i + 1)), [wonCreditMultiplier, this]()
                    {
                        UpdateCredits(wonCreditMultiplier * static_cast<int>(mCreditsWagerPerSpin));
                    });
                }
                
                if (boardStateResolutionData.mShouldTumble)
                {
                    auto tumbleResolutionData = mBoardModel.ResolveBoardTumble();
                    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(boardStateResolutionData.mWinningPaylines.size() * PAYLINE_ANIMATION_DURATION), [this, tumbleResolutionData]()
                    {
                        mBoardView->BeginTumble(tumbleResolutionData);
                    });
                }
                else
                {
                    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(boardStateResolutionData.mWinningPaylines.size() * PAYLINE_ANIMATION_DURATION), [this]()
                    {
                        mBoardView->CompleteSpin();
                    });
                }
            }
        }
    }
    
    auto creditsSceneObject = scene->FindSceneObject(CREDITS_NAME);
    if (creditsSceneObject)
    {
        std::get<scene::TextSceneObjectData>(creditsSceneObject->mSceneObjectTypeData).mText = "Credits: " + std::to_string(static_cast<int>(mDisplayedCredits));
    }
    
    auto creditsWagerSceneObject = scene->FindSceneObject(CREDITS_WAGER_NAME);
    if (creditsWagerSceneObject)
    {
        std::get<scene::TextSceneObjectData>(creditsWagerSceneObject->mSceneObjectTypeData).mText = "Wager Per Spin: " + std::to_string(static_cast<int>(mCreditsWagerPerSpin));
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
#ifndef ALLOW_OFFLINE_PLAY
    networking::LoginResponse loginResponse;
    loginResponse.DeserializeFromJson(responseJson);
    
    if (loginResponse.allowed)
    {
        mPlayerId = loginResponse.playerId;
#endif
        mPlayerId = 1;
        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto& sceneManager = systemsEngine.GetSceneManager();
        auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
        
        // Fade button out
#ifdef ALLOW_OFFLINE_PLAY
        scene::TextSceneObjectData offlinePlayTexData;
        offlinePlayTexData.mFontName = game_constants::DEFAULT_FONT_NAME;
        offlinePlayTexData.mText = "(OFFLINE PLAY)";
        
        auto offlinePlaySceneObject = scene->CreateSceneObject(strutils::StringId("offline_play"));
        offlinePlaySceneObject->mSceneObjectTypeData = std::move(offlinePlayTexData);
        offlinePlaySceneObject->mPosition = CREDITS_TEXT_POSITION;
        offlinePlaySceneObject->mPosition.x = -0.075f;
        offlinePlaySceneObject->mPosition.y = -0.226f;
        offlinePlaySceneObject->mScale = CREDITS_TEXT_SCALE/2.0f;
        offlinePlaySceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(0.5f), [=, this]()
        {
#endif
            for (auto& sceneObject: mLoginButton->GetSceneObjects())
            {
                scene->RemoveSceneObject(sceneObject->mName);
            }
            mLoginButton = nullptr;
#ifdef ALLOW_OFFLINE_PLAY
        });
#endif
        
        auto background = scene->FindSceneObject(BACKGROUND_NAME);
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(background->mShaderFloatUniformValues[strutils::StringId("mask_alpha_comp")], 0.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});
        
        auto spinButton = scene->CreateSceneObject(SPIN_BUTTON_NAME);
        spinButton->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/wheel.png");
        spinButton->mPosition = SPIN_BUTTON_POSITION;
        spinButton->mScale = SPIN_BUTTON_SCALE;
        spinButton->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(spinButton, 1.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});
        
        auto spinButtonEffect = scene->CreateSceneObject(SPIN_BUTTON_EFFECT_NAME);
        spinButtonEffect->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "spin_button_effect.vs");
        spinButtonEffect->mPosition = SPIN_BUTTON_EFFECT_POSITION;
        spinButtonEffect->mScale = SPIN_BUTTON_EFFECT_SCALE;
        spinButtonEffect->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("time_speed")] = 5.162f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_resolution")] = 312.0f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_clarity")] = 5.23f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_r_multipier")] = 0.0f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_g_multipier")] = 0.0f;
        spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_b_multipier")] = 0.0f;
        UpdateSpinButtonEffectAura();

        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(spinButtonEffect, 1.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});
        
        mCreditsWagerPlusButton = std::make_unique<AnimatedButton>(CREDITS_WAGER_PLUS_BUTTON_POSITION, ACTION_TEXT_SCALE, game_constants::DEFAULT_FONT_NAME, "+", CREDITS_WAGER_PLUS_BUTTON_NAME, [&](){ mCreditsWagerPerSpin *= 2; mCreditsWagerPerSpin = math::Min(128LL, mCreditsWagerPerSpin); UpdateSpinButtonEffectAura(); }, *scene);
        mCreditsWagerMinusButton = std::make_unique<AnimatedButton>(CREDITS_WAGER_MINUS_BUTTON_POSITION, ACTION_TEXT_SCALE, game_constants::DEFAULT_FONT_NAME, "-", CREDITS_WAGER_MINUS_BUTTON_NAME, [&](){ mCreditsWagerPerSpin /= 2; mCreditsWagerPerSpin = math::Max(1LL, mCreditsWagerPerSpin); UpdateSpinButtonEffectAura(); }, *scene);
        
        for (auto& sceneObject: mCreditsWagerPlusButton->GetSceneObjects())
        {
            sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});
        }
        for (auto& sceneObject: mCreditsWagerMinusButton->GetSceneObjects())
        {
            sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});
        }

        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(CREDITS_NAME), 1.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(CREDITS_WAGER_NAME), 1.0f, GAME_ELEMENTS_PRESENTATION_TIME), [](){});

        mBoardView = std::make_unique<BoardView>(*scene, mBoardModel);
        
        for (auto i = 0; i < static_cast<int>(slots::SymbolType::COUNT); ++i)
        {
            CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + BoardView::GetSymbolTexturePath(static_cast<slots::SymbolType>(i)));
        }
        
#ifndef ALLOW_OFFLINE_PLAY
    }
#endif
}

///------------------------------------------------------------------------------------------------

void Game::OnServerSpinResponse(const nlohmann::json& responseJson)
{
#ifndef ALLOW_OFFLINE_PLAY
    networking::SpinResponse spinResponse;
    spinResponse.DeserializeFromJson(responseJson);
    
    mSpinId = spinResponse.spinId;
    //mSpinId = 828030532; roast_chicken and chicken_soup
    mSpinId = 1539116524;
#else
    mSpinId = math::RandomInt();
#endif
    mCredits -= mCreditsWagerPerSpin;
    mDisplayedCredits = static_cast<int>(mCredits);
    UpdateSpinButtonEffectAura();

    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(CREDIT_UPDATE_ANIMATION_NAME);

    mBoardView->ResetBoardSymbols();
    mBoardModel.PopulateBoardForSpin(mSpinId);
    mBoardView->BeginSpin();
    
    logging::Log(logging::LogType::INFO, "Spin %d!", mSpinId);
}

///------------------------------------------------------------------------------------------------

void Game::OnLoginButtonPressed()
{
#ifdef ALLOW_OFFLINE_PLAY
    OnServerLoginResponse(nlohmann::json());
#else
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_LOGIN_REQUEST, networking::MessagePriority::HIGH);
#endif
}

///------------------------------------------------------------------------------------------------

void Game::OnSpinButtonPressed()
{
#ifdef ALLOW_OFFLINE_PLAY
    OnServerSpinResponse(nlohmann::json());
#else
    SendNetworkMessage(nlohmann::json(), networking::MessageType::CS_SPIN_REQUEST, networking::MessagePriority::HIGH);
#endif
}

///------------------------------------------------------------------------------------------------

void Game::UpdateCredits(const int wonCreditMultiplier)
{
    mCredits += wonCreditMultiplier;

    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
    
    scene::TextSceneObjectData textData;
    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textData.mText = "+" + std::to_string(wonCreditMultiplier);
    
    auto soName = strutils::StringId("WonCreditMultiplier " + std::to_string(SDL_GetTicks()));
    auto newWonCreditMultiplierSceneObject = scene->CreateSceneObject(soName);
    newWonCreditMultiplierSceneObject->mSceneObjectTypeData = std::move(textData);
    newWonCreditMultiplierSceneObject->mPosition = glm::vec3(-0.427f, 0.0f, CREDITS_TEXT_POSITION.z);
    newWonCreditMultiplierSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    newWonCreditMultiplierSceneObject->mScale = CREDITS_TEXT_SCALE;
    
    auto targetPosition = newWonCreditMultiplierSceneObject->mPosition;
    targetPosition.y += 0.1f;

    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(newWonCreditMultiplierSceneObject, 0.0f, CREDIT_UPDATE_ANIMATION_DURATION), [](){});
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mDisplayedCredits, static_cast<float>(mCredits), CREDIT_UPDATE_ANIMATION_DURATION), [](){}, CREDIT_UPDATE_ANIMATION_NAME);
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(newWonCreditMultiplierSceneObject, targetPosition, newWonCreditMultiplierSceneObject->mScale, CREDIT_UPDATE_ANIMATION_DURATION), [=](){
        scene->RemoveSceneObject(soName);
    });
}

///------------------------------------------------------------------------------------------------

void Game::UpdateSpinButtonEffectAura()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);

    auto spinButtonEffect = scene->FindSceneObject(SPIN_BUTTON_EFFECT_NAME);
    if (spinButtonEffect)
    {
        if (mCredits < mCreditsWagerPerSpin)
        {
            spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_r_multipier")] = 1.0f;
            spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_g_multipier")] = 0.0f;
        }
        else
        {
            spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_r_multipier")] = 0.0f;
            spinButtonEffect->mShaderFloatUniformValues[strutils::StringId("perlin_color_g_multipier")] = 1.0f;
        }
    }
}

///------------------------------------------------------------------------------------------------
