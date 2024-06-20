///------------------------------------------------------------------------------------------------
///  CoreSystemsEnginePlatformImpl.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Fonts.h>
#include <engine/rendering/OpenGL.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/sound/SoundManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/utils/Logging.h>
#include <engine/utils/OSMessageBox.h>
#include <platform_specific/InputStateManagerPlatformImpl.h>
#include <platform_specific/IOSUtils.h>
#include <platform_specific/RendererPlatformImpl.h>
#include <SDL.h>


///------------------------------------------------------------------------------------------------

static constexpr int DEFAULT_WINDOW_WIDTH  = 585;
static constexpr int DEFAULT_WINDOW_HEIGHT = 1688;
static constexpr int MIN_WINDOW_WIDTH      = 390;
static constexpr int MIN_WINDOW_HEIGHT     = 844;
static constexpr int TARGET_GAME_LOGIC_FPS = 60;

///------------------------------------------------------------------------------------------------

bool CoreSystemsEngine::mInitialized = false;
static bool sIsShuttingDown = false;

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
    sIsShuttingDown = true;
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::Initialize()
{
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "SDL could not initialize!", SDL_GetError());
        return;
    }

    // Create window
    SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "2");
    
    mWindow = SDL_CreateWindow("Realm of Beasts", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    // Set minimum window size
    SDL_SetWindowMinimumSize(mWindow, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);

    if (!mWindow)
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "SDL could not initialize!", SDL_GetError());
        return;
    }
  
    // Set OpenGL desired attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    
    // Create OpenGL context
    mContext = SDL_GL_CreateContext(mWindow);
    if (!mContext || SDL_GL_MakeCurrent(mWindow, mContext) != 0)
    {
        ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "SDL could not initialize!", SDL_GetError());
        return;
    }
    
    // Vsync
    SDL_GL_SetSwapInterval(0);

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
    logging::Log(logging::LogType::INFO, "Max Tex Size : %d", maxTextureSize);
    
    mInitialized = true;
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::Start(std::function<void()> clientInitFunction, std::function<void(const float)> clientUpdateFunction, std::function<void()> clientApplicationMovedToBackgroundFunction, std::function<void()>, std::function<void()>, std::function<void()> clientOnOneSecondElapsedFunction)
{
    mSystems->mParticleManager.LoadParticleData();
    
    clientInitFunction();
    
    //While application is running
    SDL_Event event;
    auto lastFrameMillisSinceInit = 0.0f;
    auto secsAccumulator          = 0.0f;
    auto framesAccumulator        = 0LL;
    
    bool pausedExecution = false;
    bool shouldQuit = false;
    
    const int refreshRate = rendering::GetDisplayRefreshRate();
    const float targetFpsMillis = 1000.0f / refreshRate;
    
    while(!shouldQuit)
    {
        bool windowSizeChanged = false;
        bool applicationMovedToBackground = false;
        bool applicationMovedToForeground = true;
        
        // Calculate frame delta
        const auto currentMillisSinceInit = static_cast<float>(SDL_GetTicks());  // the number of milliseconds since the SDL library
        const auto dtMillis = currentMillisSinceInit - lastFrameMillisSinceInit; // millis diff between current and last frame
        
        lastFrameMillisSinceInit = currentMillisSinceInit;
        framesAccumulator++;
        secsAccumulator += dtMillis * 0.001f; // dt in seconds;
        
        //Handle events on queue
        while(SDL_PollEvent(&event) != 0)
        {
            mSystems->mInputStateManager.VProcessInputEvent(event, shouldQuit, windowSizeChanged, applicationMovedToBackground, applicationMovedToForeground);
            
            if (applicationMovedToBackground)
            {
                clientApplicationMovedToBackgroundFunction();
                pausedExecution = true;
                mSystems->mSoundManager.PauseAudio();
            }
            else if (applicationMovedToForeground)
            {
                pausedExecution = false;
                mSystems->mSoundManager.ResumeAudio();
            }
        }
        
        if (pausedExecution)
        {
            continue;
        }
        
        if (windowSizeChanged)
        {
            for (auto& scene: mSystems->mSceneManager.GetScenes())
            {
                scene->GetCamera().RecalculateMatrices();
            }
        }
        
        mSystems->mResourceLoadingService.Update();
        mSystems->mSoundManager.Update(dtMillis);
        
        float gameLogicMillis = math::Max(16.0f, math::Min(32.0f, dtMillis)) * (TARGET_GAME_LOGIC_FPS/static_cast<float>(refreshRate));
        if (secsAccumulator > 1.0f)
        {
            logging::Log(logging::LogType::INFO, "FPS: %d", framesAccumulator);
            framesAccumulator = 0;
            secsAccumulator -= 1.0f;
            
            mSystems->mResourceLoadingService.ReloadMarkedResourcesFromDisk();
            mSystems->mFontRepository.ReloadMarkedFontsFromDisk();
            
            clientOnOneSecondElapsedFunction();
        }
  
        mSystems->mAnimationManager.Update(gameLogicMillis);
        clientUpdateFunction(gameLogicMillis);
        mSystems->mInputStateManager.VUpdate();
        
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
        
        mSystems->mRenderer.VBeginRenderPass();
        
        for (auto& scene: mSystems->mSceneManager.GetScenes())
        {
            if (scene->IsLoaded())
            {
                mSystems->mRenderer.VRenderScene(*scene);
            }
        }
        
        mSystems->mRenderer.VEndRenderPass();
        
        auto frameEndMillisDiff = static_cast<float>(SDL_GetTicks()) - currentMillisSinceInit;
        if (frameEndMillisDiff < targetFpsMillis)
        {
            SDL_Delay(targetFpsMillis - frameEndMillisDiff);
        }
    }
}

///------------------------------------------------------------------------------------------------

bool CoreSystemsEngine::IsShuttingDown()
{
    return sIsShuttingDown;
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
    int w,h; SDL_GL_GetDrawableSize(mWindow, &w, &h); return glm::vec2(w, h);
}

///------------------------------------------------------------------------------------------------

void CoreSystemsEngine::SpecialEventHandling(SDL_Event&)
{
}

///------------------------------------------------------------------------------------------------
