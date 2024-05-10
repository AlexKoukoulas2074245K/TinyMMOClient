///------------------------------------------------------------------------------------------------
///  CoreSystemsEnginePlatformImpl.cpp
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023
///------------------------------------------------------------------------------------------------

#include <chrono>
#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Fonts.h>
#include <engine/rendering/OpenGL.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/BaseDataFileSerializer.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/Logging.h>
#include <engine/utils/OSMessageBox.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <platform_specific/RendererPlatformImpl.h>
#include <platform_specific/InputStateManagerPlatformImpl.h>
#include <SDL.h>
#include <thread>

///------------------------------------------------------------------------------------------------

#if defined(USE_EDITOR)
static constexpr int DEFAULT_WINDOW_WIDTH  = 1800;
static constexpr int DEFAULT_WINDOW_HEIGHT = 1200;
static constexpr int MIN_WINDOW_WIDTH      = 1800;
static constexpr int MIN_WINDOW_HEIGHT     = 1200;
#else
static constexpr int DEFAULT_WINDOW_WIDTH  = 1266;
static constexpr int DEFAULT_WINDOW_HEIGHT = 585;
static constexpr int MIN_WINDOW_WIDTH      = 750;
static constexpr int MIN_WINDOW_HEIGHT     = 585;
#endif

static const float DEFAULT_FRAME_MILLIS = 1000.0f/60.0f;

///------------------------------------------------------------------------------------------------

bool CoreSystemsEngine::mInitialized = false;

///------------------------------------------------------------------------------------------------

static float sGameSpeed = 1.0f;
static float sLastGameLogicDtMillis = 0.0f;
static bool sPrintFPS = false;
static bool sShuttingDown = false;

#if defined(USE_IMGUI)
static const strutils::StringId PLAYGROUND_SCENE_NAME = strutils::StringId("playground_scene");
static const int PROFILLING_SAMPLE_COUNT = 300;
static bool sParticlePaintEnabled = false;
static float sPitch = 1.0f;
static float sGain = 1.0f;
static size_t sSfxIndex = 0;
static size_t sParticleIndex = 0;
static std::vector<std::string> sAvailableSfx;
static std::vector<std::string> sAvailableParticleNames;
static float sUpdateLogicMillisSamples[PROFILLING_SAMPLE_COUNT];
static float sRenderingMillisSamples[PROFILLING_SAMPLE_COUNT];
#endif

///------------------------------------------------------------------------------------------------

struct CoreSystemsEngine::SystemsImpl
{
    rendering::AnimationManager mAnimationManager;
    rendering::RendererPlatformImpl mRenderer;
    rendering::ParticleManager mParticleManager;
    rendering::FontRepository mFontRepository;
    input::InputStateManagerPlatformImpl mInputStateManager;
    scene::SceneManager mSceneManager;
    resources::ResourceLoadingService mResourceLoadingService;
    sound::SoundManager mSoundManager;
};

///------------------------------------------------------------------------------------------------

CoreSystemsEngine& CoreSystemsEngine::GetInstance()
{
    static CoreSystemsEngine instance;
    if (!instance.mInitialized) instance.Initialize();
    return instance;
}

///------------------------------------------------------------------------------------------------

CoreSystemsEngine::~CoreSystemsEngine()
{
    sShuttingDown = true;
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::Initialize()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "SDL could not initialize!", SDL_GetError());
        return;
    }

    // Create window
    mWindow = SDL_CreateWindow("Realm of Beasts", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    // Set minimum window size
    SDL_SetWindowMinimumSize(mWindow, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        
    if (!mWindow)
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "SDL could not initialize!", SDL_GetError());
        return;
    }
  
#if __APPLE__
    // Set OpenGL desired attributes
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
#endif
    
    // Create OpenGL context
    mContext = SDL_GL_CreateContext(mWindow);
    if (!mContext || SDL_GL_MakeCurrent(mWindow, mContext) != 0)
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "SDL could not initialize!", SDL_GetError());
        return;
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    if (glewInit() != GLEW_OK)
    {
        ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, "GLEW could not initialize!", "GLEW Fatal Error");
        return;
    }
#endif
    
    // Vsync
    SDL_GL_SetSwapInterval(1);

    // Systems Initialization
    mSystems = std::make_unique<SystemsImpl>();
    mSystems->mResourceLoadingService.Initialize();
    mSystems->mSoundManager.Initialize();
    
    // Enable texture blending
    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // Enable depth test
    GL_CALL(glEnable(GL_DEPTH_TEST));
    GL_CALL(glDepthFunc(GL_LESS));

    int maxTextureSize;
    GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize));
    
    logging::Log(logging::LogType::INFO, "Vendor       : %s", GL_NO_CHECK_CALL(glGetString(GL_VENDOR)));
    logging::Log(logging::LogType::INFO, "Renderer     : %s", GL_NO_CHECK_CALL(glGetString(GL_RENDERER)));
    logging::Log(logging::LogType::INFO, "Version      : %s", GL_NO_CHECK_CALL(glGetString(GL_VERSION)));
    logging::Log(logging::LogType::INFO, "Version      : %s", GL_NO_CHECK_CALL(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    logging::Log(logging::LogType::INFO, "Max Tex Size : %d", maxTextureSize);

#if defined(USE_IMGUI)
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(mWindow, mContext);
    ImGui_ImplOpenGL3_Init();
#endif
    
    mInitialized = true;
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::Start(std::function<void()> clientInitFunction, std::function<void(const float)> clientUpdateFunction, std::function<void()> clientApplicationMovingToBackgroundFunction, std::function<void()> clientApplicationWindowResizeFunction, std::function<void()> clientCreateDebugWidgetsFunction, std::function<void()> clientOnOneSecondElapsedFunction)
{
    mSystems->mParticleManager.LoadParticleData();

    clientInitFunction();
    
    //While application is running
    SDL_Event event;
    auto lastFrameMillisSinceInit = 0.0f;
    auto secsAccumulator          = 0.0f;
    auto framesAccumulator        = 0LL;
    
    bool shouldQuit = false;
    bool freezeGame = false;
    
    const int refreshRate = rendering::GetDisplayRefreshRate();
    const float targetFpsMillis = 1000.0f / refreshRate;
    
    while(!shouldQuit)
    {
        bool windowSizeChanged = false;
        bool applicationMovingToBackground = false;
        bool applicationMovingToForeground = false;
        
        // Calculate frame delta
        const auto currentMillisSinceInit = static_cast<float>(SDL_GetTicks());  // the number of milliseconds since the SDL library initialized
        const auto dtMillis = currentMillisSinceInit - lastFrameMillisSinceInit; // millis diff between current and last frame
        
        lastFrameMillisSinceInit = currentMillisSinceInit;
        framesAccumulator++;
        secsAccumulator += dtMillis * 0.001f; // dt in seconds;
        
        //Handle events on queue
        while(SDL_PollEvent(&event) != 0)
        {
            mSystems->mInputStateManager.VProcessInputEvent(event, shouldQuit, windowSizeChanged, applicationMovingToBackground, applicationMovingToForeground);
            if (shouldQuit)
            {
                break;
            }
        }
        
        if (applicationMovingToBackground)
        {
            mSystems->mSoundManager.PauseAudio();
        }
        
        if (applicationMovingToForeground)
        {
            mSystems->mSoundManager.ResumeAudio();
        }
        
        if (mSystems->mInputStateManager.VButtonTapped(input::Button::SECONDARY_BUTTON))
        {
#if defined(USE_IMGUI)
            freezeGame = !freezeGame;
#endif
        }
            
        if (mSystems->mInputStateManager.VButtonTapped(input::Button::MIDDLE_BUTTON))
        {
#if defined(USE_IMGUI)
            GLOBAL_IMGUI_WINDOW_FLAGS = GLOBAL_IMGUI_WINDOW_FLAGS == ImGuiWindowFlags_NoMove ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoMove;
#endif
        }
        
        if (windowSizeChanged)
        {
            for (auto& scene: mSystems->mSceneManager.GetScenes())
            {
                scene->GetCamera().RecalculateMatrices();
                clientApplicationWindowResizeFunction();
            }
        }
        
        if (secsAccumulator > 1.0f)
        {
            if (sPrintFPS)
            {
                logging::Log(logging::LogType::INFO, "FPS: %d", framesAccumulator);
            }
            
            framesAccumulator = 0;
            secsAccumulator -= 1.0f;
            
            mSystems->mResourceLoadingService.ReloadMarkedResourcesFromDisk();
            mSystems->mFontRepository.ReloadMarkedFontsFromDisk();
            mSystems->mParticleManager.ReloadParticlesFromDisk();
            
            clientOnOneSecondElapsedFunction();
        }
        
        mSystems->mResourceLoadingService.Update();
        mSystems->mSoundManager.Update(dtMillis);
        
        float gameLogicMillis = math::Max(16.0f, math::Min(32.0f, dtMillis)) * sGameSpeed * targetFpsMillis/DEFAULT_FRAME_MILLIS;

        // Update logic
#if defined(USE_IMGUI)
        sLastGameLogicDtMillis = gameLogicMillis;
        const auto logicUpdateTimeStart = std::chrono::system_clock::now();
#else
        (void)sLastGameLogicDtMillis;
#endif
        
        if (!freezeGame)
        {
            mSystems->mAnimationManager.Update(gameLogicMillis);
            clientUpdateFunction(gameLogicMillis);
        }
        
        if (!freezeGame)
        {
            for (auto& scene: mSystems->mSceneManager.GetScenes())
            {
                if (scene->IsLoaded())
                {
                    if (scene->GetUpdateTimeSpeedFactor() >= 1.0f)
                    {
                        scene->GetCamera().Update(gameLogicMillis * scene->GetUpdateTimeSpeedFactor());
                    }
                    
                    mSystems->mParticleManager.UpdateSceneParticles(gameLogicMillis * scene->GetUpdateTimeSpeedFactor(), *scene);
                    mSystems->mSceneManager.SortSceneObjects(scene);
                }
            }
        }
        
#if defined(USE_IMGUI)
        const auto logicUpdateTimeEnd = std::chrono::system_clock::now();
        sUpdateLogicMillisSamples[PROFILLING_SAMPLE_COUNT - 1] = std::chrono::duration_cast<std::chrono::milliseconds>(logicUpdateTimeEnd - logicUpdateTimeStart).count();
#endif
        
        // Rendering Logic
        mSystems->mRenderer.VBeginRenderPass();
        
#if defined(USE_IMGUI)
        clientCreateDebugWidgetsFunction();
        CreateEngineDebugWidgets();
        
        const auto renderingTimeStart = std::chrono::system_clock::now();
#endif
        
        for (auto& scene: mSystems->mSceneManager.GetScenes())
        {
            if (scene->IsLoaded())
            {
                mSystems->mRenderer.VRenderScene(*scene);
            }
        }
        
#if defined(USE_IMGUI)
        const auto renderingTimeEnd = std::chrono::system_clock::now();
        sRenderingMillisSamples[PROFILLING_SAMPLE_COUNT - 1] = std::chrono::duration_cast<std::chrono::milliseconds>(renderingTimeEnd - renderingTimeStart).count();
        
        for (int i = 0; i < PROFILLING_SAMPLE_COUNT - 1; ++i)
        {
            if (!freezeGame)
            {
                sUpdateLogicMillisSamples[i] = sUpdateLogicMillisSamples[i + 1];
            }
            
            sRenderingMillisSamples[i] = sRenderingMillisSamples[i + 1];
        }
#else
        (void)clientCreateDebugWidgetsFunction;
#endif
        
        mSystems->mRenderer.VEndRenderPass();
        mSystems->mInputStateManager.VUpdate();
    }
    
    ImGui::DestroyContext();
    clientApplicationMovingToBackgroundFunction();
}

///------------------------------------------------------------------------------------------------

bool CoreSystemsEngine::IsShuttingDown()
{
    return sShuttingDown;
}

///------------------------------------------------------------------------------------------------

rendering::AnimationManager& CoreSystemsEngine::GetAnimationManager()
{
    return mSystems->mAnimationManager;
}

///------------------------------------------------------------------------------------------------

rendering::IRenderer& CoreSystemsEngine::GetRenderer()
{
    return mSystems->mRenderer;
}

///------------------------------------------------------------------------------------------------

rendering::ParticleManager& CoreSystemsEngine::GetParticleManager()
{
    return mSystems->mParticleManager;
}

///------------------------------------------------------------------------------------------------

rendering::FontRepository& CoreSystemsEngine::GetFontRepository()
{
    return mSystems->mFontRepository;
}

///------------------------------------------------------------------------------------------------

input::IInputStateManager& CoreSystemsEngine::GetInputStateManager()
{
    return mSystems->mInputStateManager;
}

///------------------------------------------------------------------------------------------------

scene::SceneManager& CoreSystemsEngine::GetSceneManager()
{
    return mSystems->mSceneManager;
}

///------------------------------------------------------------------------------------------------

resources::ResourceLoadingService& CoreSystemsEngine::GetResourceLoadingService()
{
    return mSystems->mResourceLoadingService;
}

///------------------------------------------------------------------------------------------------

sound::SoundManager& CoreSystemsEngine::GetSoundManager()
{
    return mSystems->mSoundManager;
}

///------------------------------------------------------------------------------------------------

float CoreSystemsEngine::GetDefaultAspectRatio() const
{
    return static_cast<float>(DEFAULT_WINDOW_WIDTH)/DEFAULT_WINDOW_HEIGHT;
}

///------------------------------------------------------------------------------------------------

SDL_Window& CoreSystemsEngine::GetContextWindow() const
{
    return *mWindow;
}

///------------------------------------------------------------------------------------------------

glm::vec2 CoreSystemsEngine::GetContextRenderableDimensions() const
{
    int w,h; SDL_GetWindowSize(mWindow, &w, &h); return glm::vec2(w, h);
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::SpecialEventHandling(SDL_Event& event)
{
#if defined(USE_IMGUI)
    ImGui_ImplSDL2_ProcessEvent(&event);
#else
    (void)event;
#endif
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::CreateEngineDebugWidgets()
{
#if defined(USE_IMGUI)
    if (!mSystems->mSceneManager.FindScene(PLAYGROUND_SCENE_NAME))
    {
        auto scene = mSystems->mSceneManager.CreateScene(PLAYGROUND_SCENE_NAME);
        scene->SetLoaded(true);
    }
    
    if (sAvailableSfx.empty())
    {
        auto soundFiles = fileutils::GetAllFilenamesInDirectory(resources::ResourceLoadingService::RES_MUSIC_ROOT);
        for (const auto& soundFile: soundFiles)
        {
            auto fileName = strutils::StringSplit(soundFile, '/').back();
            if (!strutils::StringStartsWith(fileName, "sfx_"))
            {
                continue;
            }
            
            sAvailableSfx.emplace_back(strutils::StringSplit(fileName, '.').front());
        }
    }
    
    if (sAvailableParticleNames.empty())
    {
        for (const auto& particleEntry: mSystems->mParticleManager.GetLoadedParticleNamesToData())
        {
            sAvailableParticleNames.push_back(particleEntry.first.GetString());
        }
    }
    
    // Particle effects
    ImGui::Begin("Particle Effects", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Checkbox("Particle Test Mode", &sParticlePaintEnabled);
    if (ImGui::BeginCombo(" ", sAvailableParticleNames.at(sParticleIndex).c_str()))
    {
        for (size_t n = 0U; n < sAvailableParticleNames.size(); n++)
        {
            const bool isSelected = (sParticleIndex == n);
            if (ImGui::Selectable(sAvailableParticleNames.at(n).c_str(), isSelected))
            {
                sParticleIndex = n;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::End();
    
    auto playgroundScene = mSystems->mSceneManager.FindScene(PLAYGROUND_SCENE_NAME);
    auto worldTouchPos = mSystems->mInputStateManager.VGetPointingPosInWorldSpace(playgroundScene->GetCamera().GetViewMatrix(), playgroundScene->GetCamera().GetProjMatrix());
    if (sParticlePaintEnabled && mSystems->mInputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
    {
        mSystems->mParticleManager.CreateParticleEmitterAtPosition(strutils::StringId(sAvailableParticleNames.at(sParticleIndex)), glm::vec3(worldTouchPos.x, worldTouchPos.y, 0.0f), *playgroundScene);
    }
    
    // Sound Effects
    ImGui::Begin("Sound Effects", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    if (ImGui::BeginCombo(" ", sAvailableSfx.at(sSfxIndex).c_str()))
    {
        for (size_t n = 0U; n < sAvailableSfx.size(); n++)
        {
            const bool isSelected = (sSfxIndex == n);
            if (ImGui::Selectable(sAvailableSfx.at(n).c_str(), isSelected))
            {
                sSfxIndex = n;
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SliderFloat("Pitch", &sPitch, 0.0f, 3.0f);
    ImGui::SliderFloat("Gain", &sGain, 0.0f, 2.0f);
    if (ImGui::Button("Play Sound"))
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(sAvailableSfx[sSfxIndex]);
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(sAvailableSfx[sSfxIndex], false, sGain, sPitch);
    }
    ImGui::End();
    
    // Create runtime configs
    ImGui::Begin("Engine Runtime", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::SeparatorText("General");
    ImGui::Text("Game Logic Dt %.3f", sLastGameLogicDtMillis);
    ImGui::Checkbox("Print FPS", &sPrintFPS);
    ImGui::SliderFloat("Game Speed", &sGameSpeed, 0.01f, 10.0f);
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        sGameSpeed = 1.0f;
    }
    ImGui::SeparatorText("Profilling");
    ImGui::PlotLines("Update Logic Samples", sUpdateLogicMillisSamples, PROFILLING_SAMPLE_COUNT);
    ImGui::PlotLines("Rendering Samples", sRenderingMillisSamples, PROFILLING_SAMPLE_COUNT);
    ImGui::SeparatorText("Input");
    const auto& cursorPos = CoreSystemsEngine::GetInstance().GetInputStateManager().VGetPointingPos();
    ImGui::Text("Cursor %.3f,%.3f",cursorPos.x, cursorPos.y);
    ImGui::End();
#endif
}

///------------------------------------------------------------------------------------------------
