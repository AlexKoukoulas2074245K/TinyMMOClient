///------------------------------------------------------------------------------------------------
///  Editor.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 10/05/2024
///------------------------------------------------------------------------------------------------

#include <editor/commands/FloodFillCommand.h>
#include <editor/commands/PlaceTileCommand.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Camera.h>
#include <engine/rendering/Fonts.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/rendering/RenderingUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/resloading/TextureResource.h>
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
static const strutils::StringId TILE_CONNECTOR_TYPE_UNIFORM_NAME = strutils::StringId("connector_type");
static const strutils::StringId CUSTOM_ALPHA_UNIFORM_NAME = strutils::StringId("custom_alpha");
static const strutils::StringId TILE_HIGHLIGHTED_UNIFORM_NAME = strutils::StringId("highlighted");
static const strutils::StringId NAVMAP_TILE_COLOR_UNIFORM_NAME = strutils::StringId("navmap_tile_color");
static const strutils::StringId TOP_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("top_ref_image");
static const strutils::StringId RIGHT_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("right_ref_image");
static const strutils::StringId BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("bottom_ref_image");
static const strutils::StringId LEFT_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("left_ref_image");

static const std::string NON_SANDBOXED_MAP_GLOBAL_DATA_PATH = "/Users/Code/TinyMMOClient/assets/data/world/map_global_data.json";
static const std::string NON_SANDBOXED_MAPS_FOLDER = "/Users/Code/TinyMMOClient/assets/data/world/maps/";
static const std::string NON_SANDBOXED_MAP_TEXTURES_FOLDER = "/Users/Code/TinyMMOClient/assets/textures/world/maps/";
static const std::string NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH = "/Users/Code/TinyMMOClient/source_net_common/net_assets/data/map_global_data.json";
static const std::string NON_SANDBOXED_NET_ASSETS_NAVMAPS_FOLDER = "/Users/Code/TinyMMOClient/source_net_common/net_assets/navmaps/";
static const std::string ENTRY_MAP_NAME = "entry_map.json";
static const std::string MAP_FILES_FOLDER = "world/maps/";
static const std::string TILES_FOLDER = "world/map_tiles/";
static const std::string ZERO_BLANK_TILE_FILE_NAME = "0_blank.png";
static const std::string ZERO_BLANK_TRANSPARENT_TILE_FILE_NAME = "0_blank_transparent.png";
static const std::string ZERO_TOPLEFT_CONNECTOR_TILE_FILE_NAME = "0_topleft_connector.png";
static const std::string ZERO_TOPRIGHT_CONNECTOR_TILE_FILE_NAME = "0_topright_connector.png";
static const std::string ZERO_BOTRIGHT_CONNECTOR_TILE_FILE_NAME = "0_botright_connector.png";
static const std::string ZERO_BOTLEFT_CONNECTOR_TILE_FILE_NAME = "0_botleft_connector.png";
static const std::string ZERO_HOR_CONNECTOR_TILE_FILE_NAME = "0_hor_connector.png";
static const std::string ZERO_VER_CONNECTOR_TILE_FILE_NAME = "0_ver_connector.png";
static const std::string WORLD_MAP_TILE_SHADER = "world_map_tile.vs";
static const std::string WORLD_MAP_TILE_NAVMAP_GEN_SHADER = "world_map_tile_navmap_gen.vs";

static constexpr int DEFAULT_GRID_ROWS = 32;
static constexpr int DEFAULT_GRID_COLS = 32;
static constexpr int MAX_GRID_ROWS = 64;
static constexpr int MAX_GRID_COLS = 64;

static const float TILE_SIZE = 0.013f;
static const float ZOOM_SPEED = 1.25f;
static const float MOVE_SPEED = 0.01f;

static const glm::vec3 TILE_DEFAULT_SCALE = glm::vec3(TILE_SIZE);
static const glm::vec3 EMPTY_NAVMAP_TILE_COLOR = {1.0f, 1.0f, 1.0f};

static std::unordered_map<std::string, glm::vec3> SPECIAL_NAVMAP_TILES_TO_COLORS =
{
    { "wall_top.png", { 0.0f, 0.0f, 0.0f } },
    { "wall_body.png", { 0.0f, 0.0f, 0.0f } },
    { "water.png", { 0.0f, 0.0f, 1.0f } }
};

namespace TileConnectorType
{
    static constexpr int NONE     = 0;
    static constexpr int VER      = 1;
    static constexpr int HOR      = 2;
    static constexpr int TOPLEFT  = 3;
    static constexpr int TOPRIGHT = 4;
    static constexpr int BOTRIGHT = 5;
    static constexpr int BOTLEFT  = 6;
    static constexpr int INVALID  = 7;
};

///------------------------------------------------------------------------------------------------

Editor::Editor(const int argc, char** argv)
    : mTopImageRefIndex(0)
    , mRightImageRefIndex(0)
    , mBottomImageRefIndex(0)
    , mLeftImageRefIndex(0)
    , mActivePanel(0)
    , mBottomLayerVisibility(1.0f)
    , mTopLayerVisibility(0.5f)
    , mPaintingToolType(PaintingToolType::PENCIL)
    , mActiveLayer(map_constants::LayerType::BOTTOM_LAYER)
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
    
    mSelectedPaletteTile = 0;
    mViewOptions.mCameraZoom = scene->GetCamera().GetZoomFactor();
    mViewOptions.mCameraPosition = scene->GetCamera().GetPosition();
    CreateMap(DEFAULT_GRID_ROWS, DEFAULT_GRID_COLS);
}

///------------------------------------------------------------------------------------------------

void Editor::Update(const float dtMillis)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& inputStateManager = systemsEngine.GetInputStateManager();
    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
    
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
    
    // Skip input handling if ImGUI wants it
    ImGuiIO& io = ImGui::GetIO();
    bool imGuiMouseInput = io.WantCaptureMouse;
    
    std::vector<std::shared_ptr<scene::SceneObject>> highlightedTileCandidates;
    
    for (auto y = 0; y < mGridRows; ++y)
    {
        for (auto x = 0; x < mGridCols; ++x)
        {
            {
                auto bottomLayerTile = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                auto rect = scene_object_utils::GetSceneObjectBoundingRect(*bottomLayerTile);
                
                bottomLayerTile->mShaderBoolUniformValues[TILE_HIGHLIGHTED_UNIFORM_NAME] = false;
                
                auto cursorInTile = math::IsPointInsideRectangle(rect.bottomLeft, rect.topRight, worldTouchPos);
                if (mActiveLayer == map_constants::LayerType::BOTTOM_LAYER && cursorInTile && !imGuiMouseInput)
                {
                    highlightedTileCandidates.push_back(bottomLayerTile);
                }
                
                UpdateTile(bottomLayerTile, scene, "", x, y);
            }
            
            {
                
                auto topLayerTile = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                auto rect = scene_object_utils::GetSceneObjectBoundingRect(*topLayerTile);
                
                topLayerTile->mShaderBoolUniformValues[TILE_HIGHLIGHTED_UNIFORM_NAME] = false;
                
                auto cursorInTile = math::IsPointInsideRectangle(rect.bottomLeft, rect.topRight, worldTouchPos);
                if (mActiveLayer == map_constants::LayerType::TOP_LAYER && cursorInTile && !imGuiMouseInput)
                {
                    highlightedTileCandidates.push_back(topLayerTile);
                }
                
                UpdateTile(topLayerTile, scene, "_top", x, y);
            }
        }
    }
    
    if (!highlightedTileCandidates.empty())
    {
        std::sort(highlightedTileCandidates.begin(), highlightedTileCandidates.end(), [&](std::shared_ptr<scene::SceneObject> lhs, std::shared_ptr<scene::SceneObject> rhs){ return glm::distance(glm::vec2(lhs->mPosition.x, lhs->mPosition.y), worldTouchPos) < glm::distance(glm::vec2(rhs->mPosition.x, rhs->mPosition.y), worldTouchPos); });
        highlightedTileCandidates.front()->mShaderBoolUniformValues[TILE_HIGHLIGHTED_UNIFORM_NAME] = true;
        
        if (inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON))
        {
            const auto& selectedPaletteTile = mPaletteTileData[mSelectedPaletteTile];
            
            switch (mPaintingToolType)
            {
                case PaintingToolType::PENCIL: TryExecuteCommand(std::make_unique<commands::PlaceTileCommand>(highlightedTileCandidates.front(), selectedPaletteTile.mResourceId)); break;
                case PaintingToolType::BUCKET: TryExecuteCommand(std::make_unique<commands::FloodFillCommand>(scene, highlightedTileCandidates.front(), selectedPaletteTile.mResourceId)); break;
            }
            
        }
        else if (inputStateManager.VButtonTapped(input::Button::SECONDARY_BUTTON))
        {
            TryExecuteCommand(std::make_unique<commands::PlaceTileCommand>(highlightedTileCandidates.front(), mActiveLayer == map_constants::LayerType::BOTTOM_LAYER ? mPaletteTileData[0].mResourceId : mPaletteTileData[1].mResourceId));
        }
    }
 
    bool commandModifierDown =
#if defined(MACOS)
        (inputStateManager.VKeyPressed(input::Key::LCMD) || inputStateManager.VKeyPressed(input::Key::RCMD));
#elif defined(WINDOWS)
        (inputStateManager.VKeyPressed(input::Key::LCTL) || inputStateManager.VKeyPressed(input::Key::RCTL));
#endif
    bool shiftModifierDown = (inputStateManager.VKeyPressed(input::Key::LSFT) || inputStateManager.VKeyPressed(input::Key::RSFT));
    
    auto scrollDelta = inputStateManager.VGetScrollDelta();
    if ((scrollDelta.y != 0 || scrollDelta.x != 0) && !imGuiMouseInput)
    {
        // Cam zoom
        if (commandModifierDown)
        {
            mViewOptions.mCameraZoom = mViewOptions.mCameraZoom * (scrollDelta.y > 0 ? ZOOM_SPEED : 1/ZOOM_SPEED);
            scene->GetCamera().SetZoomFactor(mViewOptions.mCameraZoom);
            
            auto newWorldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
            mViewOptions.mCameraPosition.x -= newWorldTouchPos.x - worldTouchPos.x;
            mViewOptions.mCameraPosition.y -= newWorldTouchPos.y - worldTouchPos.y;
        }
        // Horizontal cam translation
        else if (shiftModifierDown)
        {
            mViewOptions.mCameraPosition.x -= scrollDelta.x * MOVE_SPEED;
        }
        // Vertical cam translation
        else
        {
            mViewOptions.mCameraPosition.y += scrollDelta.y * MOVE_SPEED;
        }
    }

    if (commandModifierDown && inputStateManager.VKeyTapped(input::Key::Z))
    {
        TryUndoLastCommand();
    }
    
    scene->GetCamera().SetPosition(mViewOptions.mCameraPosition);
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

void Editor::DestroyMap()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
    
    mTopImageRefIndex = 0;
    mRightImageRefIndex = 0;
    mBottomImageRefIndex = 0;
    mLeftImageRefIndex = 0;
    
    scene->RemoveSceneObject(TOP_REF_IMAGE_SCENE_OBJECT_NAME);
    scene->RemoveSceneObject(RIGHT_REF_IMAGE_SCENE_OBJECT_NAME);
    scene->RemoveSceneObject(BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME);
    scene->RemoveSceneObject(LEFT_REF_IMAGE_SCENE_OBJECT_NAME);
    
    for (auto y = 0; y < mGridRows; ++y)
    {
        for (auto x = 0; x < mGridCols; ++x)
        {
            scene->RemoveSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
            scene->RemoveSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
        }
    }
}

///------------------------------------------------------------------------------------------------

void Editor::CreateMap(const int gridRows, const int gridCols)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
    
    mExecutedCommandHistory = {};
    mGridRows = gridRows;
    mGridCols = gridCols;
    
    auto gridWidth = gridCols * TILE_SIZE;
    auto gridHeight = gridRows * TILE_SIZE;
    
    auto gridStartingX = -gridWidth/2;
    auto gridStartingY = gridHeight/2;
    
    for (auto y = 0; y < gridRows; ++y)
    {
        for (auto x = 0; x < gridCols; ++x)
        {
            {
                auto tile = scene->CreateSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                tile->mPosition.x = gridStartingX + x * TILE_SIZE;
                tile->mPosition.y = gridStartingY - y * TILE_SIZE;
                tile->mPosition.z = map_constants::TILE_BOTTOM_LAYER_Z;
                tile->mScale = TILE_DEFAULT_SCALE;
                tile->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILES_FOLDER + ZERO_BLANK_TILE_FILE_NAME);
                tile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + WORLD_MAP_TILE_SHADER);
                tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::NONE;
            }
            
            {
                auto topLayerTile = scene->CreateSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                topLayerTile->mPosition.x = gridStartingX + x * TILE_SIZE;
                topLayerTile->mPosition.y = gridStartingY - y * TILE_SIZE;
                topLayerTile->mPosition.z = map_constants::TILE_TOP_LAYER_Z;
                topLayerTile->mScale = TILE_DEFAULT_SCALE;
                topLayerTile->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILES_FOLDER + ZERO_BLANK_TRANSPARENT_TILE_FILE_NAME);
                topLayerTile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + WORLD_MAP_TILE_SHADER);
                topLayerTile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::NONE;
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void Editor::UpdateTile(std::shared_ptr<scene::SceneObject> tile, std::shared_ptr<scene::Scene> scene, const std::string& tileNamePostfix, const int tileCol, const int tileRow)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    
    auto tileTextureFileName = fileutils::GetFileName(systemsEngine.GetResourceLoadingService().GetResourcePath(tile->mTextureResourceId));
    if (tileTextureFileName == ZERO_HOR_CONNECTOR_TILE_FILE_NAME)
    {
        if (tileCol == 0 || tileCol == mGridCols - 1)
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::INVALID;
        }
        else
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::HOR;
            tile->mEffectTextureResourceIds[0] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol - 1) + "," + std::to_string(tileRow) + tileNamePostfix))->mTextureResourceId;
            tile->mEffectTextureResourceIds[1] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol + 1) + "," + std::to_string(tileRow) + tileNamePostfix))->mTextureResourceId;
        }
    }
    else if (tileTextureFileName == ZERO_VER_CONNECTOR_TILE_FILE_NAME)
    {
        if (tileRow == 0 || tileRow == mGridRows - 1)
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::INVALID;
        }
        else
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::VER;
            tile->mEffectTextureResourceIds[0] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol) + "," + std::to_string(tileRow - 1) + tileNamePostfix))->mTextureResourceId;
            tile->mEffectTextureResourceIds[1] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol) + "," + std::to_string(tileRow + 1) + tileNamePostfix))->mTextureResourceId;
        }
    }
    else if (tileTextureFileName == ZERO_TOPLEFT_CONNECTOR_TILE_FILE_NAME || tileTextureFileName == ZERO_BOTRIGHT_CONNECTOR_TILE_FILE_NAME)
    {
        if (tileRow == 0 || tileRow == mGridRows - 1 || tileCol == 0 || tileCol == mGridCols - 1)
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::INVALID;
        }
        else
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = tileTextureFileName == ZERO_TOPLEFT_CONNECTOR_TILE_FILE_NAME ? TileConnectorType::TOPLEFT : TileConnectorType::BOTRIGHT;
            tile->mEffectTextureResourceIds[0] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol - 1) + "," + std::to_string(tileRow - 1) + tileNamePostfix))->mTextureResourceId;
            tile->mEffectTextureResourceIds[1] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol + 1) + "," + std::to_string(tileRow + 1) + tileNamePostfix))->mTextureResourceId;
        }
    }
    else if (tileTextureFileName == ZERO_TOPRIGHT_CONNECTOR_TILE_FILE_NAME || tileTextureFileName == ZERO_BOTLEFT_CONNECTOR_TILE_FILE_NAME)
    {
        if (tileRow == 0 || tileRow == mGridRows - 1 || tileCol == 0 || tileCol == mGridCols - 1)
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::INVALID;
        }
        else
        {
            tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = tileTextureFileName == ZERO_TOPRIGHT_CONNECTOR_TILE_FILE_NAME ? TileConnectorType::TOPRIGHT : TileConnectorType::BOTLEFT;
            tile->mEffectTextureResourceIds[0] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol - 1) + "," + std::to_string(tileRow + 1) + tileNamePostfix))->mTextureResourceId;
            tile->mEffectTextureResourceIds[1] = scene->FindSceneObject(strutils::StringId(std::to_string(tileCol + 1) + "," + std::to_string(tileRow - 1) + tileNamePostfix))->mTextureResourceId;
        }
    }
    else
    {
        tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::NONE;
    }
    
    if (!mViewOptions.mRenderConnectorTiles && tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] != TileConnectorType::INVALID)
    {
        tile->mShaderIntUniformValues[TILE_CONNECTOR_TYPE_UNIFORM_NAME] = TileConnectorType::NONE;
    }
    
    tile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + WORLD_MAP_TILE_SHADER);
    tile->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = !tileNamePostfix.empty() ? mTopLayerVisibility : mBottomLayerVisibility;
}

///------------------------------------------------------------------------------------------------

void Editor::TryExecuteCommand(std::unique_ptr<commands::IEditorCommand> command)
{
    if (!command->VIsNoOp())
    {
        command->VExecute();
        mExecutedCommandHistory.push(std::move(command));
    }
}

///------------------------------------------------------------------------------------------------

void Editor::TryUndoLastCommand()
{
    if (!mExecutedCommandHistory.empty())
    {
        auto poppedCommand = std::move(mExecutedCommandHistory.top());
        mExecutedCommandHistory.pop();
        poppedCommand->VUndo();
    }
}

///------------------------------------------------------------------------------------------------

#include <editor/EditorImGuiCommands.inc>

///------------------------------------------------------------------------------------------------
