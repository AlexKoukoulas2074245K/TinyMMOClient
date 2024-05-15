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


#if defined(USE_IMGUI)
void Editor::CreateDebugWidgets()
{
    static std::vector<std::string> sMapFileNames;
    static std::vector<std::string> sMapFileNameOptions; // Mirrors above, includes "None"
    static std::vector<std::string> sOtherMapTextures {"None"};
    static std::unordered_map<std::string, glm::vec2> sMapTextureNamesToDimensions;
    static std::unordered_map<std::string, std::unordered_map<std::string, std::pair<int, std::string>>> sMapConnections;
    
    auto RefreshGlobalMapFilesLambda = [&]()
    {
        sMapFileNames = fileutils::GetAllFilenamesInDirectory(NON_SANDBOXED_MAPS_FOLDER);
        sMapFileNameOptions = sMapFileNames;
        sMapFileNameOptions.insert(sMapFileNameOptions.begin(), "None");
        
        sOtherMapTextures = {"None"};
        sMapTextureNamesToDimensions.clear();
        
        auto mapTextureFileNames = fileutils::GetAllFilenamesInDirectory(NON_SANDBOXED_MAP_TEXTURES_FOLDER);
        for (auto filePath: mapTextureFileNames)
        {
            if (strutils::StringEndsWith(filePath, "_bottom_layer.png"))
            {
                auto fileName = fileutils::GetFileName(filePath);
                auto mapName = fileName.substr(0, fileName.find("_bottom_layer.png"));
                sOtherMapTextures.push_back(mapName);
                
                std::ifstream mapDataFile(NON_SANDBOXED_MAPS_FOLDER + mapName + ".json");
                if (mapDataFile.is_open())
                {
                    std::stringstream buffer;
                    buffer << mapDataFile.rdbuf();
                    auto mapDataJson = nlohmann::json::parse(buffer.str());
                    sMapTextureNamesToDimensions[mapName] = glm::vec2(mapDataJson["metadata"]["cols"].get<float>() * TILE_DEFAULT_SCALE.x, mapDataJson["metadata"]["rows"].get<float>() * TILE_DEFAULT_SCALE.y);
                }
            }
        }
        
        sMapConnections.clear();
        std::ifstream mapConnectionsFile(NON_SANDBOXED_MAP_GLOBAL_DATA_PATH);
        if (mapConnectionsFile.is_open())
        {
            std::stringstream buffer;
            buffer << mapConnectionsFile.rdbuf();
            auto mapConnectionsJson = nlohmann::json::parse(buffer.str());
            for (auto it = mapConnectionsJson["map_connections"].begin(); it != mapConnectionsJson["map_connections"].end(); ++it)
            {
                auto mapName = it.key();
                if (std::find(sMapFileNames.begin(), sMapFileNames.end(), mapName) == sMapFileNames.end())
                {
                    continue;
                }
                    
                auto mapConnectionEntry = it.value();
                
                auto topConnection = mapConnectionEntry["top"].get<std::string>();
                auto rightConnection = mapConnectionEntry["right"].get<std::string>();
                auto bottomConnection = mapConnectionEntry["bottom"].get<std::string>();
                auto leftConnection = mapConnectionEntry["left"].get<std::string>();
                
                auto topConnectionNameFindIter = std::find(sMapFileNameOptions.begin(), sMapFileNameOptions.end(), topConnection);
                auto rightConnectionNameFindIter = std::find(sMapFileNameOptions.begin(), sMapFileNameOptions.end(), rightConnection);
                auto bottomConnectionNameFindIter = std::find(sMapFileNameOptions.begin(), sMapFileNameOptions.end(), bottomConnection);
                auto leftConnectionNameFindIter = std::find(sMapFileNameOptions.begin(), sMapFileNameOptions.end(), leftConnection);
                
                sMapConnections[mapName]["top"] = topConnectionNameFindIter != sMapFileNameOptions.end() ? std::make_pair(std::distance(sMapFileNameOptions.begin(), topConnectionNameFindIter), topConnection) : std::make_pair(0, "None");
                sMapConnections[mapName]["right"] = rightConnectionNameFindIter != sMapFileNameOptions.end() ? std::make_pair(std::distance(sMapFileNameOptions.begin(), rightConnectionNameFindIter), rightConnection) : std::make_pair(0, "None");
                sMapConnections[mapName]["bottom"] = bottomConnectionNameFindIter != sMapFileNameOptions.end() ? std::make_pair(std::distance(sMapFileNameOptions.begin(), bottomConnectionNameFindIter), bottomConnection) : std::make_pair(0, "None");
                sMapConnections[mapName]["left"] = leftConnectionNameFindIter != sMapFileNameOptions.end() ? std::make_pair(std::distance(sMapFileNameOptions.begin(), leftConnectionNameFindIter), leftConnection) : std::make_pair(0, "None");
            }
        }
        else
        {
            logging::Log(logging::LogType::INFO, "Could not find map_global_data.json file");
            
            for (auto mapFileNameConnection: sMapFileNames)
            {
                sMapConnections[mapFileNameConnection]["top"] = std::make_pair(0, "None");
                sMapConnections[mapFileNameConnection]["right"] = std::make_pair(0, "None");
                sMapConnections[mapFileNameConnection]["bottom"] = std::make_pair(0, "None");
                sMapConnections[mapFileNameConnection]["left"] = std::make_pair(0, "None");
            }
        }
    };
    
    {
        static constexpr int TILEMAP_NAME_BUFFER_SIZE = 64;
        static char sMapNameBuffer[TILEMAP_NAME_BUFFER_SIZE] = {};
        static size_t sSelectedMapFileIndex = 0;
        
        if (sMapFileNames.empty())
        {
            RefreshGlobalMapFilesLambda();
        }
        
        ImGui::Begin("Tile Map File", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        if (!sMapFileNames.empty())
        {
            ImGui::PushID("ExistingMapFiles");
            if (ImGui::BeginCombo(" ", sMapFileNames.at(sSelectedMapFileIndex).c_str()))
            {
                for (size_t n = 0U; n < sMapFileNames.size(); n++)
                {
                    const bool isSelected = (sSelectedMapFileIndex == n);
                    if (ImGui::Selectable(sMapFileNames.at(n).c_str(), isSelected))
                    {
                        sSelectedMapFileIndex = n;
                        memset(sMapNameBuffer, 0, TILEMAP_NAME_BUFFER_SIZE);
                        strcpy(sMapNameBuffer, sMapFileNames.at(n).c_str());
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("ExistingMaps");
        }
        
        if (strlen(sMapNameBuffer) == 0)
        {
            if (sMapFileNames.empty())
            {
                strcpy(sMapNameBuffer, ENTRY_MAP_NAME.c_str());
            }
            else
            {
                strcpy(sMapNameBuffer, sMapFileNames.front().c_str());
            }
        }
        
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("MapName", &sMapNameBuffer[0], TILEMAP_NAME_BUFFER_SIZE);
        
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("  Load  "))
        {
            std::ifstream dataFile(NON_SANDBOXED_MAPS_FOLDER + sMapNameBuffer);
            if (dataFile.is_open())
            {
                std::stringstream buffer;
                buffer << dataFile.rdbuf();
                auto contents = buffer.str();
                auto mapJson = nlohmann::json::parse(contents);
                
                auto& systemsEngine = CoreSystemsEngine::GetInstance();
                auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                
                DestroyMap();
                CreateMap(mapJson["metadata"]["rows"].get<int>(), mapJson["metadata"]["cols"]);
                
                {
                    auto rowCounter = 0;
                    auto colCounter = 0;
                    for (auto rowJson: mapJson["tiledata"]["bottomlayer"])
                    {
                        colCounter = 0;
                        for (auto tileJson: rowJson)
                        {
                            auto tileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(colCounter) + "," + std::to_string(rowCounter)));
                            tileSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_ROOT + tileJson["tile_texture"].get<std::string>());
                            colCounter++;
                        }
                        rowCounter++;
                    }
                }
                
                {
                    auto rowCounter = 0;
                    auto colCounter = 0;
                    
                    if (mapJson["tiledata"].contains("toplayer") && !mapJson["tiledata"]["toplayer"].is_null())
                    {
                        for (auto rowJson: mapJson["tiledata"]["toplayer"])
                        {
                            colCounter = 0;
                            for (auto tileJson: rowJson)
                            {
                                auto tileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(colCounter) + "," + std::to_string(rowCounter) + "_top"));
                                tileSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_ROOT + tileJson["tile_texture"].get<std::string>());
                                colCounter++;
                            }
                            rowCounter++;
                        }
                    }
                }
                
                logging::Log(logging::LogType::ERROR, "Successfully loaded %s", (resources::ResourceLoadingService::RES_DATA_ROOT + MAP_FILES_FOLDER + sMapNameBuffer).c_str());
            }
            else
            {
                logging::Log(logging::LogType::ERROR, "Could not load %s", (resources::ResourceLoadingService::RES_DATA_ROOT + MAP_FILES_FOLDER + sMapNameBuffer).c_str());
            }
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20.0f, 0.0f));
        ImGui::SameLine();
        if (ImGui::Button("  Save  "))
        {
            auto beginTimePoint = std::chrono::high_resolution_clock::now();
            
            nlohmann::json mapJson;
            nlohmann::json mapMetaDataJson;
            nlohmann::json mapTileDataJson;
            nlohmann::json bottomLayerMapTileDataJson;
            nlohmann::json topLayerMapTileDataJson;
            
            mapMetaDataJson["rows"] = mGridRows;
            mapMetaDataJson["cols"] = mGridCols;
            
            auto& systemsEngine = CoreSystemsEngine::GetInstance();
            auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
            
            for (auto y = 0; y < mGridRows; ++y)
            {
                nlohmann::json bottomLayerRowJson;
                nlohmann::json topLayerRowJson;
                for (auto x = 0; x < mGridCols; ++x)
                {
                    {
                        auto bottomLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                        nlohmann::json tileJson;
                        tileJson["tile_texture"] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourcePath(bottomLayerTileSceneObject->mTextureResourceId);
                        bottomLayerRowJson.push_back(tileJson);
                    }
                    
                    {
                        auto topLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                        nlohmann::json tileJson;
                        tileJson["tile_texture"] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourcePath(topLayerTileSceneObject->mTextureResourceId);
                        topLayerRowJson.push_back(tileJson);
                    }
                }
                bottomLayerMapTileDataJson.push_back(bottomLayerRowJson);
                topLayerMapTileDataJson.push_back(topLayerRowJson);
            }
            
            mapTileDataJson["bottomlayer"] = bottomLayerMapTileDataJson;
            mapTileDataJson["toplayer"] = topLayerMapTileDataJson;
            mapJson["tiledata"] = mapTileDataJson;
            mapJson["metadata"] = mapMetaDataJson;
            
            std::ofstream outputMapJsonFile(NON_SANDBOXED_MAPS_FOLDER + std::string(sMapNameBuffer));
            auto mapJsonString = mapJson.dump(4);
            outputMapJsonFile.write(mapJsonString.c_str(), mapJsonString.size());
            outputMapJsonFile.close();
            
            // Render map textures
            std::vector<std::shared_ptr<scene::SceneObject>> bottomLayerSceneObjects;
            std::vector<std::shared_ptr<scene::SceneObject>> topLayerSceneObjects;
            
            for (auto y = 0; y < mGridRows; ++y)
            {
                for (auto x = 0; x < mGridCols; ++x)
                {
                    auto bottomLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                    auto topLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                    
                    bottomLayerTileSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                    topLayerTileSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                    
                    bottomLayerSceneObjects.push_back(bottomLayerTileSceneObject);
                    topLayerSceneObjects.push_back(topLayerTileSceneObject);
                }
            }
            rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + "_bottom_layer.png", bottomLayerSceneObjects, rendering::BlurStep::DONT_BLUR);
            rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + "_top_layer.png", topLayerSceneObjects, rendering::BlurStep::DONT_BLUR);
            
            // Render map navmap texture
            for (auto y = 0; y < mGridRows; ++y)
            {
                nlohmann::json rowJson;
                for (auto x = 0; x < mGridCols; ++x)
                {
                    {
                        auto bottomLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                        auto tileTextureResourcePath = fileutils::GetFileName(systemsEngine.GetResourceLoadingService().GetResourcePath(bottomLayerTileSceneObject->mTextureResourceId));
                        
                        bottomLayerTileSceneObject->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + WORLD_MAP_TILE_NAVMAP_GEN_SHADER);
                        bottomLayerTileSceneObject->mShaderVec3UniformValues[NAVMAP_TILE_COLOR_UNIFORM_NAME] = SPECIAL_NAVMAP_TILES_TO_COLORS.count(tileTextureResourcePath) ? SPECIAL_NAVMAP_TILES_TO_COLORS.at(tileTextureResourcePath) : EMPTY_NAVMAP_TILE_COLOR;
                    }
                    
                    {
                        auto topLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                        auto tileTextureResourcePath = fileutils::GetFileName(systemsEngine.GetResourceLoadingService().GetResourcePath(topLayerTileSceneObject->mTextureResourceId));
                        
                        topLayerTileSceneObject->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + WORLD_MAP_TILE_NAVMAP_GEN_SHADER);
                        topLayerTileSceneObject->mShaderVec3UniformValues[NAVMAP_TILE_COLOR_UNIFORM_NAME] = SPECIAL_NAVMAP_TILES_TO_COLORS.count(tileTextureResourcePath) ? SPECIAL_NAVMAP_TILES_TO_COLORS.at(tileTextureResourcePath) : EMPTY_NAVMAP_TILE_COLOR;
                    }
                }
            }
            rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + "_navmap.png", bottomLayerSceneObjects, rendering::BlurStep::DONT_BLUR);
            
            logging::Log(logging::LogType::ERROR, "Successfully saved %s", (NON_SANDBOXED_MAPS_FOLDER + std::string(sMapNameBuffer)).c_str());
            
            auto endTimePoint = std::chrono::high_resolution_clock::now();
            
            ospopups::ShowMessageBox(ospopups::MessageBoxType::INFO, "Export complete", "Finished saving map file and exporting texture & navmap for " + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + ". Operation took " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - beginTimePoint).count()) + " secs");
            
            RefreshGlobalMapFilesLambda();
        }
        
        ImGui::SeparatorText("Modify/Create");
        static int sDimensionsX = DEFAULT_GRID_COLS;
        static int sDimensionsY = DEFAULT_GRID_ROWS;
        
        if (ImGui::InputInt("x", &sDimensionsX))
        {
            if (sDimensionsX > MAX_GRID_COLS)
            {
                sDimensionsX = MAX_GRID_COLS;
            }
            else if (sDimensionsX < 0)
            {
                sDimensionsX = 0;
            }
        }
        
        if (ImGui::InputInt("y", &sDimensionsY))
        {
            if (sDimensionsY > MAX_GRID_ROWS)
            {
                sDimensionsY = MAX_GRID_ROWS;
            }
            else if (sDimensionsY < 0)
            {
                sDimensionsY = 0;
            }
        }
        
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("  Create  "))
        {
            DestroyMap();
            CreateMap(sDimensionsY, sDimensionsX);
            memset(sMapNameBuffer, 0, TILEMAP_NAME_BUFFER_SIZE);
            strcpy(sMapNameBuffer, ENTRY_MAP_NAME.c_str());
        }
        
        ImGui::SeparatorText("Side Image References");
        
        auto& systemsEngine = CoreSystemsEngine::GetInstance();
        auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
        
        auto GetSideRefImageSceneObjectNameLambda = [&](const int& sideImageRefIndex)
        {
            if (&sideImageRefIndex == &mTopImageRefIndex)
            {
                return TOP_REF_IMAGE_SCENE_OBJECT_NAME;
            }
            else if (&sideImageRefIndex == &mRightImageRefIndex)
            {
                return RIGHT_REF_IMAGE_SCENE_OBJECT_NAME;
            }
            else if (&sideImageRefIndex == &mBottomImageRefIndex)
            {
                return BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME;
            }
            else
            {
                return LEFT_REF_IMAGE_SCENE_OBJECT_NAME;
            }
        };
        
        auto ImageRefComboLambda = [&](const std::string& sideRefLable, int& sideImageRefIndex)
        {
            ImGui::PushID((sideRefLable + "combo").c_str());
            if (ImGui::BeginCombo(" ", sOtherMapTextures.at(sideImageRefIndex).c_str()))
            {
                for (int n = 0; n < static_cast<int>(sOtherMapTextures.size()); n++)
                {
                    const bool isSelected = (sideImageRefIndex == n);
                    if (ImGui::Selectable(sOtherMapTextures.at(n).c_str(), isSelected))
                    {
                        sideImageRefIndex = n;
                        
                        auto sideRefImageSceneObjectName = GetSideRefImageSceneObjectNameLambda(sideImageRefIndex);
                        scene->RemoveSceneObject(sideRefImageSceneObjectName);
                        
                        if (sOtherMapTextures.at(sideImageRefIndex) != "None")
                        {
                            auto sideRefImageSceneObject = scene->CreateSceneObject(sideRefImageSceneObjectName);
                            
                            auto sideRefImagePath = NON_SANDBOXED_MAP_TEXTURES_FOLDER + sOtherMapTextures.at(sideImageRefIndex) + "_bottom_layer.png";
                            
                            systemsEngine.GetResourceLoadingService().UnloadResource(sideRefImagePath, resources::ResourceLoadingPathType::ABSOLUTE);
                            sideRefImageSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(sideRefImagePath, resources::ResourceReloadMode::DONT_RELOAD, resources::ResourceLoadingPathType::ABSOLUTE);
                            
                            sideRefImageSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.5f;
                            sideRefImageSceneObject->mPosition.x = -0.013f;
                            sideRefImageSceneObject->mPosition.y = 0.0115f;
                            sideRefImageSceneObject->mPosition.z = map_constants::TILE_TOP_LAYER_Z;
                            
                            if (&sideImageRefIndex == &mTopImageRefIndex)
                            {
                                sideRefImageSceneObject->mPosition.y += (mGridRows * TILE_DEFAULT_SCALE.y)/2.0f + sMapTextureNamesToDimensions.at(sOtherMapTextures.at(sideImageRefIndex)).y/2.0f;
                                sideRefImageSceneObject->mPosition.z += 0.1f;
                            }
                            else if (&sideImageRefIndex == &mRightImageRefIndex)
                            {
                                sideRefImageSceneObject->mPosition.x += (mGridCols * TILE_DEFAULT_SCALE.x)/2.0f + sMapTextureNamesToDimensions.at(sOtherMapTextures.at(sideImageRefIndex)).x/2.0f;
                                sideRefImageSceneObject->mPosition.z += 0.2f;
                            }
                            else if (&sideImageRefIndex == &mBottomImageRefIndex)
                            {
                                sideRefImageSceneObject->mPosition.y -= (mGridRows * TILE_DEFAULT_SCALE.y)/2.0f + sMapTextureNamesToDimensions.at(sOtherMapTextures.at(sideImageRefIndex)).y/2.0f;
                                sideRefImageSceneObject->mPosition.z += 0.3f;
                            }
                            else
                            {
                                sideRefImageSceneObject->mPosition.x -= (mGridCols * TILE_DEFAULT_SCALE.x)/2.0f + sMapTextureNamesToDimensions.at(sOtherMapTextures.at(sideImageRefIndex)).x/2.0f;
                                sideRefImageSceneObject->mPosition.z += 0.4f;
                            }
                        }
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Text("%s", sideRefLable.c_str());
        };
            
        ImageRefComboLambda("TopRef", mTopImageRefIndex);
        ImageRefComboLambda("RightRef", mRightImageRefIndex);
        ImageRefComboLambda("BottomRef", mBottomImageRefIndex);
        ImageRefComboLambda("LeftRef", mLeftImageRefIndex);
        
        ImGui::SeparatorText("Render Options");
        ImGui::Checkbox("Render Tile Connectors", &mViewOptions.mRenderConnectorTiles);
        ImGui::End();
    }
    
    {
        ImGui::Begin("Tile Map Palette", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        ImGui::SeparatorText("Painting Tools");
        
        static resources::GLuint sPencilIconGLTextureId = 0;
        static resources::GLuint sBucketIconGLTextureId = 0;
        
        if (sPencilIconGLTextureId == 0)
        {
            auto pencilIconResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "editor/pencil_icon.png");
            const auto& pencilTextureResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::TextureResource>(pencilIconResourceId);
            sPencilIconGLTextureId = pencilTextureResource.GetGLTextureId();
        }
        if (sBucketIconGLTextureId == 0)
        {
            auto bucketIconResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "editor/bucket_icon.png");
            const auto& bucketTextureResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::TextureResource>(bucketIconResourceId);
            sBucketIconGLTextureId = bucketTextureResource.GetGLTextureId();
        }
        ImGui::PushID("Pencil");
        {
            ImVec4 bgCol = mPaintingToolType == PaintingToolType::PENCIL ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImVec4 tintCol = mPaintingToolType == PaintingToolType::PENCIL ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 0.7f);
                                
            if (ImGui::ImageButton("Pencil", reinterpret_cast<void*>(sPencilIconGLTextureId), ImVec2(64.0f, 64.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), bgCol, tintCol))
            {
                mPaintingToolType = PaintingToolType::PENCIL;
            }
        }
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::PushID("Bucket");
        {
            ImVec4 bgCol = mPaintingToolType == PaintingToolType::BUCKET ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            ImVec4 tintCol = mPaintingToolType == PaintingToolType::BUCKET ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 0.7f);
                                
            if (ImGui::ImageButton("Bucket", reinterpret_cast<void*>(sBucketIconGLTextureId), ImVec2(64.0f, 64.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), bgCol, tintCol))
            {
                mPaintingToolType = PaintingToolType::BUCKET;
            }
        }
        ImGui::PopID();
        
        ImGui::SeparatorText("Tiles");
        
        static constexpr int GRID_COLS = 4;
        static int sGridRows = 1;
        
        if (mPaletteTileData.empty())
        {
            auto mapTileFileNames = fileutils::GetAllFilenamesInDirectory(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILES_FOLDER);
            
            for (const auto& mapTileFileName: mapTileFileNames)
            {
                auto loadedResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILES_FOLDER + mapTileFileName);
                const auto& tileTextureResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::TextureResource>(loadedResourceId);
                mPaletteTileData.push_back({ loadedResourceId, tileTextureResource.GetGLTextureId(), mapTileFileName });
            }
            
            sGridRows = 1 + static_cast<int>(mPaletteTileData.size()) / GRID_COLS;
        }
        
        ImGui::Text("Selected Tile: %s", mPaletteTileData[mSelectedPaletteTile].mTileName.c_str());
        for (int y = 0; y < sGridRows; y++)
        {
            for (int x = 0; x < GRID_COLS; x++)
            {
                if (x > 0)
                {
                    ImGui::SameLine();
                }
                    
                auto tileIndex = y * GRID_COLS + x;
                auto tileName = tileIndex >= mPaletteTileData.size() ? "Empty" : mPaletteTileData[tileIndex].mTileName;
                auto tileTextureId = tileIndex >= mPaletteTileData.size() ? 0 : mPaletteTileData[tileIndex].mTextureId;
                
                ImGui::PushID(tileIndex);
                
                if (tileTextureId)
                {
                    ImVec4 bgCol = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                    ImVec4 tintCol = mSelectedPaletteTile == tileIndex ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 0.7f);
                                        
                    if (ImGui::ImageButton(tileName.c_str(), reinterpret_cast<void*>(tileTextureId), ImVec2(64.0f, 64.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), bgCol, tintCol))
                    {
                        mSelectedPaletteTile = tileIndex;
                    }
                }
                
                
                ImGui::PopID();
            }
        }
        ImGui::End();
    }
    
    {
        static int layerIndex = 0;
        ImGui::Begin("Layers", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        if (ImGui::RadioButton("Bottom Layer", &layerIndex, static_cast<int>(map_constants::LayerType::BOTTOM_LAYER)))
        {
            mActiveLayer = static_cast<map_constants::LayerType>(layerIndex);
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20.0f, 0.0f));
        ImGui::SameLine();
        ImGui::PushID("BotLayerVisible");
        ImGui::SetNextItemWidth(100.0f);
        ImGui::SliderFloat("Visibility", &mBottomLayerVisibility, 0.0f, 1.0f);
        ImGui::PopID();
        
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        if (ImGui::RadioButton("Top Layer", &layerIndex, static_cast<int>(map_constants::LayerType::TOP_LAYER)))
        {
            mActiveLayer = static_cast<map_constants::LayerType>(layerIndex);
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(41.0f, 0.0f));
        ImGui::SameLine();
        ImGui::PushID("TopLayerVisible");
        ImGui::SetNextItemWidth(100.0f);
        ImGui::SliderFloat("Visibility", &mTopLayerVisibility, 0.0f, 1.0f);
        ImGui::PopID();
        ImGui::End();
    }
    
    {
        ImGui::Begin("Editor Debug", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        ImGui::Text("Executed Command History size = %lu", mExecutedCommandHistory.size());
        ImGui::End();
    }
    
    {
        ImGui::Begin("Map Global Data Editor", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        
        if (sMapFileNames.empty())
        {
            RefreshGlobalMapFilesLambda();
        }
        
        ImGui::SeparatorText("Map Connections");
        for (auto mapFileName: sMapFileNames)
        {
            if (ImGui::CollapsingHeader(mapFileName.c_str(), ImGuiTreeNodeFlags_None))
            {
                auto connectionCreationLambda = [](const std::string& mapName, const std::string& direction)
                {
                    auto& mapConnections = sMapConnections.at(mapName);
                    auto& mapDirectionConnections = mapConnections.at(direction);
                    
                    ImGui::PushID((mapName + direction + "combo").c_str());
                    if (ImGui::BeginCombo(" ", sMapFileNameOptions.at(mapDirectionConnections.first).c_str()))
                    {
                        for (int n = 0; n < static_cast<int>(sMapFileNameOptions.size()); n++)
                        {
                            const bool isSelected = (mapDirectionConnections.first == n);
                            const auto& currentMapFileNameOption = sMapFileNameOptions.at(n);
                            
                            if (ImGui::Selectable(currentMapFileNameOption.c_str(), isSelected))
                            {
                                const auto previousSelection = mapDirectionConnections.second;
                                
                                mapDirectionConnections.first = n;
                                mapDirectionConnections.second = currentMapFileNameOption;
                                
                                std::string oppositeDirection = "";
                                if (direction == "top")
                                {
                                    oppositeDirection = "bottom";
                                }
                                else if (direction == "right")
                                {
                                    oppositeDirection = "left";
                                }
                                else if (direction == "bottom")
                                {
                                    oppositeDirection = "top";
                                }
                                else
                                {
                                    oppositeDirection = "right";
                                }
                                
                                if (currentMapFileNameOption != "None")
                                {
                                    sMapConnections.at(currentMapFileNameOption).at(oppositeDirection).first = static_cast<int>(std::distance(sMapFileNameOptions.begin(), std::find(sMapFileNameOptions.begin(), sMapFileNameOptions.end(), mapName)));
                                    sMapConnections.at(currentMapFileNameOption).at(oppositeDirection).second = mapName;
                                }
                                else if (previousSelection != "None")
                                {
                                    sMapConnections.at(previousSelection).at(oppositeDirection) = std::make_pair(0, "None");
                                }
                            }
                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopID();
                    ImGui::SameLine();
                    ImGui::Text("%s", direction.c_str());
                };
                
                connectionCreationLambda(mapFileName, "top");
                connectionCreationLambda(mapFileName, "right");
                connectionCreationLambda(mapFileName, "bottom");
                connectionCreationLambda(mapFileName, "left");
            }
        }
        ImGui::SeparatorText("Export");
        if (ImGui::Button("Save Global Map Data"))
        {
            std::ofstream globalMapDataFile(NON_SANDBOXED_MAP_GLOBAL_DATA_PATH);
            if (globalMapDataFile.is_open())
            {
                nlohmann::json globalMapDataJson;
                
                // Serialize map connections to other maps in all directions
                nlohmann::json exportedConnectionsJson;
                for (auto mapConnectionEntry: sMapConnections)
                {
                    nlohmann::json connectionEntryJson;
                    connectionEntryJson["top"] = mapConnectionEntry.second["top"].second;
                    connectionEntryJson["right"] = mapConnectionEntry.second["right"].second;
                    connectionEntryJson["bottom"] = mapConnectionEntry.second["bottom"].second;
                    connectionEntryJson["left"] = mapConnectionEntry.second["left"].second;
                    exportedConnectionsJson[mapConnectionEntry.first] = connectionEntryJson;
                }
                
                // Serialize map positions in a floodfill fashion starting from a set map
                nlohmann::json mapPositionsJson;
                
                
                globalMapDataJson["map_connections"] = exportedConnectionsJson;
                globalMapDataJson["map_positions"] = mapPositionsJson;
                
                
                globalMapDataFile << globalMapDataJson.dump(4);
                globalMapDataFile.close();
            }
            ospopups::ShowMessageBox(ospopups::MessageBoxType::INFO, "Export complete", "Finished exporting global map data.");
            RefreshGlobalMapFilesLambda();
        }
        ImGui::End();
    }
    
    ImGui::ShowDemoWindow();
}
#else
void Editor::CreateDebugWidgets()
{
}
#endif

//#include <editor/EditorImGuiCommands.inc>

///------------------------------------------------------------------------------------------------
