///------------------------------------------------------------------------------------------------
///  Editor.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 10/05/2024
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
#include <editor/Editor.h>
#include <imgui/imgui.h>
#include <mutex>

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId EDITOR_SCENE = strutils::StringId("editor_scene");

///------------------------------------------------------------------------------------------------

Editor::Editor(const int argc, char** argv)
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

Editor::~Editor(){}

///------------------------------------------------------------------------------------------------

void Editor::Init()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    systemsEngine.GetFontRepository().LoadFont(game_constants::DEFAULT_FONT_NAME.GetString(), resources::ResourceReloadMode::DONT_RELOAD);
    systemsEngine.GetSoundManager().SetAudioEnabled(false);
    
    auto scene = systemsEngine.GetSceneManager().CreateScene(EDITOR_SCENE);
    scene->SetLoaded(true);

    auto grassTile = scene->CreateSceneObject(strutils::StringId("grassTile"));
    grassTile->mPosition.x -= 0.3f;
    grassTile->mPosition.z = 5.0f;
    grassTile->mScale *= 0.3f;
    grassTile->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/map_tiles/grass_tile.png");
    
    auto grassrockTile = scene->CreateSceneObject(strutils::StringId("grassrockTile"));
    grassrockTile->mPosition.z = 5.0f;
    grassrockTile->mScale *= 0.3f;
    grassrockTile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "tile_connector.vs");
    grassrockTile->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/map_tiles/grass_tile.png");
    grassrockTile->mEffectTextureResourceIds[0] = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/map_tiles/rock_tile.png");
    
    auto rockTile = scene->CreateSceneObject(strutils::StringId("rockTile"));
    rockTile->mPosition.x += 0.3f;
    rockTile->mPosition.z = 5.0f;
    rockTile->mScale *= 0.3f;
    rockTile->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/map_tiles/rock_tile.png");
}

///------------------------------------------------------------------------------------------------

void Editor::Update(const float dtMillis)
{
}

///------------------------------------------------------------------------------------------------

void Editor::ApplicationMovedToBackground()
{
}

///------------------------------------------------------------------------------------------------

void Editor::OnOneSecondElapsed()
{
}

///------------------------------------------------------------------------------------------------

void Editor::WindowResize()
{
}

///------------------------------------------------------------------------------------------------

#if defined(USE_IMGUI)
void Editor::CreateDebugWidgets()
{
    //static float sPlayerSpeedMultiplier = 1.0f;
    
    ImGui::Begin("Net Stats", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
    ImGui::Text("ABCD");
    ImGui::End();
}
#else
void Editor::CreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------
