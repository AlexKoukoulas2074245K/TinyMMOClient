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
#include <game/DebugGameWidgets.h>
#include <game/events/EventSystem.h>
#include <mutex>
#include <SDL.h>

#define ALLOW_OFFLINE_PLAY
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
static const strutils::StringId SCATTER_REMAINING_SPINS_NAME = strutils::StringId("scatter_remaining_spins");
static const strutils::StringId SCATTER_MULTIPLIER_NAME = strutils::StringId("scatter_multiplier");
static const strutils::StringId CREDIT_UPDATE_ANIMATION_NAME = strutils::StringId("credit_update_animation");
static const strutils::StringId CREDITS_WAGER_PLUS_BUTTON_NAME = strutils::StringId("credit_wager_plus");
static const strutils::StringId CREDITS_WAGER_MINUS_BUTTON_NAME = strutils::StringId("credit_wager_minus");
static const strutils::StringId SCATTER_BACKGROUND_NAME = strutils::StringId("scatter_background");

static const std::string SCATTER_GRANDMA_TEXTURE_PATH = "game/food_slot_images/scatter_grandma.png";
static const std::string SCATTER_MASK_TEXTURE_PATH = "game/food_slot_images/scatter_selected_symbol_mask.png";
static const std::string SCATTER_MASK_SHADER_PATH = "scatter_selected_symbol.vs";

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
static const glm::vec3 CREDITS_WAGER_TEXT_POSITION = glm::vec3(0.333f, 0.197f, 2.0f);
static const glm::vec3 SCATTER_REMAINING_SPINS_POSITION = glm::vec3(-0.157f, -0.204f, 2.0f);
static const glm::vec3 SCATTER_MULTIPLIER_TEXT_POSITION = glm::vec3(0.051f, 0.292f, 2.0f);
static const glm::vec3 CREDITS_WAGER_PLUS_BUTTON_POSITION = glm::vec3(0.35f, 0.15f, 2.0f);
static const glm::vec3 CREDITS_WAGER_MINUS_BUTTON_POSITION = glm::vec3(0.45f, 0.15f, 2.0f);
static const glm::vec3 SCATTER_BACKGROUND_POSITION = glm::vec3(0.0f, 0.0f, -0.1f);
static const glm::vec3 SCATTER_BACKGROUND_SCALE = glm::vec3(0.092f * 5.0f, 0.06624f * 5.0f, 1.0f);

static const float GAME_ELEMENTS_PRESENTATION_TIME = 1.0f;
static const float PAYLINE_REVEAL_ANIMATION_DURATION = 1.0f;
static const float PAYLINE_HIDE_ANIMATION_DURATION = 0.5f;
static const float PAYLINE_ANIMATION_DURATION = PAYLINE_REVEAL_ANIMATION_DURATION + PAYLINE_HIDE_ANIMATION_DURATION;
static const float SPIN_BUTTON_DEPRESSED_SCALE_FACTOR = 0.85f;
static const float SPIN_BUTTON_ANIMATION_DURATION = 0.15f;
static const float SPIN_BUTTON_EFFECT_ANIMATION_DURATION = 0.5f;
static const float CREDIT_UPDATE_ANIMATION_DURATION = 1.0f;
static const float SCATTER_BACKGROUND_MAX_ALPHA = 0.5f;
static const float SCATTER_DELAY_PER_EVENT = 1.0f;

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
    
    scene::TextSceneObjectData scatterRemainingSpinsTextData;
    scatterRemainingSpinsTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
    scatterRemainingSpinsTextData.mText = "Bonus Spins: 3";
    
    auto scatterSpinsSceneObject = scene->CreateSceneObject(SCATTER_REMAINING_SPINS_NAME);
    scatterSpinsSceneObject->mSceneObjectTypeData = std::move(scatterRemainingSpinsTextData);
    scatterSpinsSceneObject->mPosition = SCATTER_REMAINING_SPINS_POSITION;
    scatterSpinsSceneObject->mScale = CREDITS_TEXT_SCALE;
    scatterSpinsSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    scene::TextSceneObjectData scatterMultiplierTextData;
    scatterMultiplierTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
    scatterMultiplierTextData.mText = "5x Bonus Multiplier";
    
    auto scatterMultiplierSceneObject = scene->CreateSceneObject(SCATTER_MULTIPLIER_NAME);
    scatterMultiplierSceneObject->mSceneObjectTypeData = std::move(scatterMultiplierTextData);
    scatterMultiplierSceneObject->mPosition = SCATTER_MULTIPLIER_TEXT_POSITION;
    scatterMultiplierSceneObject->mScale = CREDITS_TEXT_SCALE;
    scatterMultiplierSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;

    scene::TextSceneObjectData creditsWagerTextData;
    creditsWagerTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
    creditsWagerTextData.mText = "Wager: " + std::to_string(mCreditsWagerPerSpin);
    
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
    DebugGameWidgets::CreateDebugWidgets(*this);
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
    
    auto spinButton = scene->FindSceneObject(SPIN_BUTTON_NAME);
    auto spinButtonEffect = scene->FindSceneObject(SPIN_BUTTON_EFFECT_NAME);
    auto scatterBackground = scene->FindSceneObject(SCATTER_BACKGROUND_NAME);
    auto scatterFreeSpins = scene->FindSceneObject(SCATTER_REMAINING_SPINS_NAME);
    auto scatterMultiplier = scene->FindSceneObject(SCATTER_MULTIPLIER_NAME);

    if (mLoginButton)
    {
        mLoginButton->Update(dtMillis);
    }
    
    if (mCreditsWagerPlusButton && !mScatterOngoing && mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
    {
        AnimateSceneObjectVisibility(mCreditsWagerPlusButton->GetSceneObjects().front(), dtMillis, true);
        mCreditsWagerPlusButton->Update(dtMillis);
    }
    else if (mCreditsWagerPlusButton && (mScatterOngoing || mBoardView->GetSpinAnimationState() != BoardView::SpinAnimationState::IDLE))
    {
        AnimateSceneObjectVisibility(mCreditsWagerPlusButton->GetSceneObjects().front(), dtMillis, false);
    }
    
    if (mCreditsWagerMinusButton && !mScatterOngoing && mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
    {
        AnimateSceneObjectVisibility(mCreditsWagerMinusButton->GetSceneObjects().front(), dtMillis, true);
        mCreditsWagerMinusButton->Update(dtMillis);
    }
    else if (mCreditsWagerMinusButton && (mScatterOngoing || mBoardView->GetSpinAnimationState() != BoardView::SpinAnimationState::IDLE))
    {
        AnimateSceneObjectVisibility(mCreditsWagerMinusButton->GetSceneObjects().front(), dtMillis, false);
    }
    
    AnimateSceneObjectVisibility(scatterFreeSpins, dtMillis, mScatterOngoing);
    AnimateSceneObjectVisibility(scatterMultiplier, dtMillis, mScatterOngoing);
    AnimateSceneObjectVisibility(scatterBackground, dtMillis, mScatterOngoing, SCATTER_BACKGROUND_MAX_ALPHA);

    if (spinButton)
    {
        if (mScatterOngoing)
        {
            AnimateSceneObjectVisibility(spinButton, dtMillis, false);
            AnimateSceneObjectVisibility(spinButtonEffect, dtMillis, false);
                 
            if (mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
            {
                if (mBoardModel.GetOustandingScatterSpins() > 1)
                {
                    mBoardView->WaitForScatterStatsUpdate();
                    UpdateAndAnimateScatterStats();
                }
                else
                {
                    mScatterOngoing = false;
                }
            }
        }
        else
        {
            while (spinButton->mRotation.z < -2.0f * math::PI)
            {
                spinButton->mRotation.z += 2.0f * math::PI;
            }
            
            if (animationManager.GetAnimationCountPlayingForSceneObject(SPIN_BUTTON_NAME) == 0 && mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::IDLE)
            {
                AnimateSceneObjectVisibility(spinButton, dtMillis, true);
                AnimateSceneObjectVisibility(spinButtonEffect, dtMillis, true);
                
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
                    auto tumbleResolutionData = mBoardModel.ResolveBoardTumble(boardStateResolutionData);
                    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(boardStateResolutionData.mWinningPaylines.size() * PAYLINE_ANIMATION_DURATION), [this, tumbleResolutionData]()
                    {
                        mBoardView->BeginTumble(tumbleResolutionData);
                    });
                }
                else if (mBoardModel.GetSymbolCountInPlayableBoard(slots::SymbolType::SCATTER) >= 3)
                {
                    animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>((boardStateResolutionData.mWinningPaylines.size() + 1) * PAYLINE_ANIMATION_DURATION), [this]()
                    {
                        mBoardView->BeginScatter();
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
        else if (mBoardView->GetSpinAnimationState() == BoardView::SpinAnimationState::SCATTER_ANIMATION_FINISHED)
        {
            auto scatterFreeSpins = scene->FindSceneObject(SCATTER_REMAINING_SPINS_NAME);
            auto scatterMultiplier = scene->FindSceneObject(SCATTER_MULTIPLIER_NAME);
            
            std::get<scene::TextSceneObjectData>(scatterMultiplier->mSceneObjectTypeData).mText[0] = '1';
            std::get<scene::TextSceneObjectData>(scatterFreeSpins->mSceneObjectTypeData).mText.back() = '0' + (mBoardModel.GetOustandingScatterSpins() - 1);

            mScatterOngoing = true;
            OnSpinButtonPressed();
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
        std::get<scene::TextSceneObjectData>(creditsWagerSceneObject->mSceneObjectTypeData).mText = "Wager: " + std::to_string(static_cast<int>(mCreditsWagerPerSpin));
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
        offlinePlaySceneObject->mPosition.z += 0.226f;
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
        
        auto scatterBackground = scene->CreateSceneObject(SCATTER_BACKGROUND_NAME);
        scatterBackground->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        scatterBackground->mPosition = SCATTER_BACKGROUND_POSITION;
        scatterBackground->mScale = SCATTER_BACKGROUND_SCALE;
        scatterBackground->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_GRANDMA_TEXTURE_PATH);
        scatterBackground->mEffectTextureResourceIds[0] =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_MASK_TEXTURE_PATH);
        scatterBackground->mShaderResourceId =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SCATTER_MASK_SHADER_PATH);
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(scatterBackground, 1.1f, 2.0f, animation_flags::ANIMATE_CONTINUOUSLY), [](){});
        
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
    //mSpinId = 1030580430; // 3 Scatter and Combo madness after
    //mSpinId = 828030532; //roast_chicken and chicken_soup
    //mSpinId = 1539116524;
#else
    mSpinId = math::RandomInt();
#endif
    if (!mScatterOngoing)
    {
        mCredits -= mCreditsWagerPerSpin;
    }
    
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

void Game::UpdateAndAnimateScatterStats()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& sceneManager = systemsEngine.GetSceneManager();
    auto scene = sceneManager.FindScene(game_constants::WORLD_SCENE_NAME);
    auto scatterFreeSpins = scene->FindSceneObject(SCATTER_REMAINING_SPINS_NAME);
    auto scatterMultiplier = scene->FindSceneObject(SCATTER_MULTIPLIER_NAME);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(SCATTER_DELAY_PER_EVENT), [this, scatterMultiplier, scatterFreeSpins]()
    {
        std::get<scene::TextSceneObjectData>(scatterMultiplier->mSceneObjectTypeData).mText[0] = '0' + (mBoardModel.GetScatterMultiplier() + 1);
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(scatterMultiplier, 1.2f, 0.25f), [scatterFreeSpins, this]()
        {
            std::get<scene::TextSceneObjectData>(scatterFreeSpins->mSceneObjectTypeData).mText.back() = '0' + (mBoardModel.GetOustandingScatterSpins() - 1);
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(scatterFreeSpins, 0.9f, 0.25f), [this]()
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(SCATTER_DELAY_PER_EVENT), [this](){ OnSpinButtonPressed(); });
            });
        });
    });
}

///------------------------------------------------------------------------------------------------

void Game::AnimateSceneObjectVisibility(std::shared_ptr<scene::SceneObject> sceneObject, const float dtMillis, const bool isVisible, const float maxAlpha /* = 1.0f */)
{
    if (!sceneObject)
    {
        return;
    }

    if (isVisible)
    {
        if (sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] <= maxAlpha)
        {
            sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] += dtMillis/1000.0f;
        }
    }
    else
    {
        if (sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] >= 0.0f)
        {
            sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] -= dtMillis/1000.0f;
        }
    }
}

///------------------------------------------------------------------------------------------------
