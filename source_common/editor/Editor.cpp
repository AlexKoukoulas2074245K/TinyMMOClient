///------------------------------------------------------------------------------------------------
///  Editor.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 10/05/2024
///------------------------------------------------------------------------------------------------

#include <editor/commands/FloodFillCommand.h>
#include <editor/commands/PlaceTileCommand.h>
#include <editor/commands/NavmapTileTypeFloodFillCommand.h>
#include <editor/commands/PlaceNavmapTileTypeCommand.h>
#include <editor/EditorUtils.h>
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
#include <net_common/Navmap.h>
#include <mutex>
#include <SDL_image.h>

#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId EDITOR_SCENE = strutils::StringId("editor_scene");
static const strutils::StringId TILE_HIGHLIGHTED_UNIFORM_NAME = strutils::StringId("highlighted");
static const strutils::StringId TOP_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("top_ref_image");
static const strutils::StringId RIGHT_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("right_ref_image");
static const strutils::StringId BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("bottom_ref_image");
static const strutils::StringId LEFT_REF_IMAGE_SCENE_OBJECT_NAME = strutils::StringId("left_ref_image");

static const std::string NON_SANDBOXED_MAPS_FOLDER = "/Users/Code/TinyMMOClient/assets/data/editor/maps/";
static const std::string NON_SANDBOXED_MAP_TEXTURES_FOLDER = "/Users/Code/TinyMMOClient/assets/textures/world/maps/";
static const std::string NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH = "/Users/Code/TinyMMOClient/source_net_common/net_assets/map_global_data.json";
static const std::string NON_SANDBOXED_NET_ASSETS_NAVMAPS_FOLDER = "/Users/Code/TinyMMOClient/source_net_common/net_assets/navmaps/";

#if defined(USE_IMGUI)
static const std::string MAP_FILES_FOLDER = "world/maps/";
#endif

static const std::string TILESETS_FOLDER = "editor/map_tilesets/";
static const std::string BASE_TILESET_NAME = "base_tileset";
static const std::string EDITOR_MAP_TILE_SHADER = "editor_map_tile.vs";

static Editor::MapTileData BLANK_TILE_DATA = {};
static Editor::MapTileData BLANK_TRANSPARENT_TILE_DATA = {};

static constexpr int TILESET_SIZE = 64;
static constexpr int TILESET_TILE_SIZE = 16;
static const float TILE_UV_SIZE = static_cast<float>(TILESET_TILE_SIZE)/static_cast<float>(TILESET_SIZE);

static constexpr int DEFAULT_GRID_ROWS = 32;
static constexpr int DEFAULT_GRID_COLS = 32;

#if defined(USE_IMGUI)
static constexpr int MAX_GRID_ROWS = 64;
static constexpr int MAX_GRID_COLS = 64;
#endif

static const float TILE_SIZE = 0.015625f;
static const float ZOOM_SPEED = 1.25f;
static const float MOVE_SPEED = 0.01f;

static const glm::vec3 TILE_DEFAULT_SCALE = glm::vec3(TILE_SIZE);

///------------------------------------------------------------------------------------------------

Editor::Editor(const int argc, char** argv)
    : mTopImageRefIndex(0)
    , mRightImageRefIndex(0)
    , mBottomImageRefIndex(0)
    , mLeftImageRefIndex(0)
    , mActivePanel(0)
    , mPaintingToolType(PaintingToolType::PENCIL)
    , mLayersVisibility({1.0f, 0.5f, 0.25f})
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

    BLANK_TILE_DATA.mTilesetName = BASE_TILESET_NAME;
    BLANK_TILE_DATA.mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILESETS_FOLDER + BASE_TILESET_NAME + ".png");
    BLANK_TILE_DATA.mTextureId = systemsEngine.GetResourceLoadingService().GetResource<resources::TextureResource>(BLANK_TILE_DATA.mTextureResourceId).GetGLTextureId();
    BLANK_TILE_DATA.mTileCoords = {0, 0};
    
    BLANK_TRANSPARENT_TILE_DATA.mTilesetName = BASE_TILESET_NAME;
    BLANK_TRANSPARENT_TILE_DATA.mTextureResourceId = BLANK_TILE_DATA.mTextureResourceId;
    BLANK_TRANSPARENT_TILE_DATA.mTextureId = BLANK_TILE_DATA.mTextureId;
    BLANK_TRANSPARENT_TILE_DATA.mTileCoords = {0, 1};
    
    mSelectedPaletteIndex = 0;
    mSelectedPaletteTile = 0;
    mSelectedNavmapTileType = networking::NavmapTileType::WALKABLE;

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
                
                UpdateTile(bottomLayerTile, map_constants::LayerType::BOTTOM_LAYER, x, y);
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
                
                UpdateTile(topLayerTile, map_constants::LayerType::TOP_LAYER, x, y);
            }
            
            {
                
                auto navmapLayerTile = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_navmap"));
                auto rect = scene_object_utils::GetSceneObjectBoundingRect(*navmapLayerTile);
                
                navmapLayerTile->mShaderBoolUniformValues[TILE_HIGHLIGHTED_UNIFORM_NAME] = false;
                
                auto cursorInTile = math::IsPointInsideRectangle(rect.bottomLeft, rect.topRight, worldTouchPos);
                if (mActiveLayer == map_constants::LayerType::NAVMAP && cursorInTile && !imGuiMouseInput)
                {
                    highlightedTileCandidates.push_back(navmapLayerTile);
                }
                
                UpdateTile(navmapLayerTile, map_constants::LayerType::NAVMAP, x, y);
            }
        }
    }
    
    if (!highlightedTileCandidates.empty())
    {
        std::sort(highlightedTileCandidates.begin(), highlightedTileCandidates.end(), [&](std::shared_ptr<scene::SceneObject> lhs, std::shared_ptr<scene::SceneObject> rhs){ return glm::distance(glm::vec2(lhs->mPosition.x, lhs->mPosition.y), worldTouchPos) < glm::distance(glm::vec2(rhs->mPosition.x, rhs->mPosition.y), worldTouchPos); });
        highlightedTileCandidates.front()->mShaderBoolUniformValues[TILE_HIGHLIGHTED_UNIFORM_NAME] = true;
        
        if (inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON))
        {
            const auto& selectedPaletteTile = mPaletteTileData[mSelectedPaletteIndex][mSelectedPaletteTile];
            
            if (mActiveLayer != map_constants::LayerType::NAVMAP)
            {
                switch (mPaintingToolType)
                {
                    case PaintingToolType::PENCIL: TryExecuteCommand(std::make_unique<commands::PlaceTileCommand>(highlightedTileCandidates.front(), selectedPaletteTile.mTileCoords, selectedPaletteTile.mTextureResourceId, TILE_UV_SIZE)); break;
                    case PaintingToolType::BUCKET: TryExecuteCommand(std::make_unique<commands::FloodFillCommand>(scene, highlightedTileCandidates.front(), mActiveLayer, selectedPaletteTile.mTileCoords, selectedPaletteTile.mTextureResourceId, TILE_UV_SIZE)); break;
                }
            }
            else
            {
                switch (mPaintingToolType)
                {
                    case PaintingToolType::PENCIL: TryExecuteCommand(std::make_unique<commands::PlaceNavmapTileTypeCommand>(highlightedTileCandidates.front(), mSelectedNavmapTileType)); break;
                    case PaintingToolType::BUCKET: TryExecuteCommand(std::make_unique<commands::NavmapTileTypeFloodFillCommand>(scene, highlightedTileCandidates.front(), mSelectedNavmapTileType)); break;
                }
            }
            
        }
        // "Eraser" button, assuming bottom layer blank in index 0 and top layer blank in index 1
        else if (inputStateManager.VButtonPressed(input::Button::SECONDARY_BUTTON))
        {
            if (mActiveLayer != map_constants::LayerType::NAVMAP)
            {
                TryExecuteCommand(std::make_unique<commands::PlaceTileCommand>(highlightedTileCandidates.front(), mActiveLayer == map_constants::LayerType::BOTTOM_LAYER ? BLANK_TILE_DATA.mTileCoords : BLANK_TRANSPARENT_TILE_DATA.mTileCoords, mActiveLayer == map_constants::LayerType::BOTTOM_LAYER ? BLANK_TILE_DATA.mTextureResourceId : BLANK_TRANSPARENT_TILE_DATA.mTextureResourceId, TILE_UV_SIZE));
            }
            else
            {
                TryExecuteCommand(std::make_unique<commands::PlaceNavmapTileTypeCommand>(highlightedTileCandidates.front(), networking::NavmapTileType::WALKABLE));
            }
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
            scene->RemoveSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_navmap"));
        }
    }
}

///------------------------------------------------------------------------------------------------

void Editor::CreateMap(const int gridRows, const int gridCols)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
    
    mViewOptions.mCameraPosition.x = 0.0f;
    mViewOptions.mCameraPosition.y = 0.0f;
    
    mExecutedCommandHistory = {};
    mGridRows = gridRows;
    mGridCols = gridCols;

    auto gridStartingX = -(gridCols * TILE_SIZE)/2;
    auto gridStartingY = (gridRows * TILE_SIZE)/2;
    
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
                tile->mTextureResourceId = BLANK_TILE_DATA.mTextureResourceId;
                tile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + EDITOR_MAP_TILE_SHADER);
                editor_utils::SetNormalTileUniforms(tile, BLANK_TILE_DATA.mTileCoords, TILE_UV_SIZE);
            }
            
            {
                auto topLayerTile = scene->CreateSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                topLayerTile->mPosition.x = gridStartingX + x * TILE_SIZE;
                topLayerTile->mPosition.y = gridStartingY - y * TILE_SIZE;
                topLayerTile->mPosition.z = map_constants::TILE_TOP_LAYER_Z;
                topLayerTile->mScale = TILE_DEFAULT_SCALE;
                topLayerTile->mTextureResourceId = BLANK_TRANSPARENT_TILE_DATA.mTextureResourceId;
                topLayerTile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + EDITOR_MAP_TILE_SHADER);
                editor_utils::SetNormalTileUniforms(topLayerTile, BLANK_TRANSPARENT_TILE_DATA.mTileCoords, TILE_UV_SIZE);
            }
            
            {
                auto navmapTile = scene->CreateSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_navmap"));
                navmapTile->mPosition.x = gridStartingX + x * TILE_SIZE;
                navmapTile->mPosition.y = gridStartingY - y * TILE_SIZE;
                navmapTile->mPosition.z = map_constants::TILE_NAVMAP_LAYER_Z;
                navmapTile->mScale = TILE_DEFAULT_SCALE;
                navmapTile->mShaderIntUniformValues[TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME] = static_cast<int>(networking::NavmapTileType::WALKABLE);
                navmapTile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + EDITOR_MAP_TILE_SHADER);
                editor_utils::SetNavmapTileUniforms(navmapTile);
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void Editor::UpdateTile(std::shared_ptr<scene::SceneObject> tile, const map_constants::LayerType layer, const int tileCol, const int tileRow)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    
    tile->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + EDITOR_MAP_TILE_SHADER);
    tile->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = mLayersVisibility[static_cast<int>(layer)];
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
    static int sDimensionsX = DEFAULT_GRID_COLS;
    static int sDimensionsY = DEFAULT_GRID_ROWS;
    static std::vector<std::pair<std::string, resources::ResourceId>> paletteNamesAndTextures;
    static int sSelectedExportEntryMapIndex = 0;

    if (mPaletteTileData.empty() && paletteNamesAndTextures.empty())
    {
        auto mapTilesetFileNames = fileutils::GetAllFilenamesAndFolderNamesInDirectory(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILESETS_FOLDER);
        
        for (const auto& mapTilesetFileName: mapTilesetFileNames)
        {
            auto loadedResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILESETS_FOLDER + mapTilesetFileName);
            const auto& tileTextureResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::TextureResource>(loadedResourceId);
            paletteNamesAndTextures.emplace_back(strutils::StringSplit(mapTilesetFileName, '.').front(), loadedResourceId);
            
            mPaletteTileData.emplace_back();
            for (int row = 0; row < TILESET_SIZE/TILESET_TILE_SIZE; ++row)
            {
                for (int col = 0; col < TILESET_SIZE/TILESET_TILE_SIZE; ++col)
                {
                    mPaletteTileData.back().push_back({ mapTilesetFileName, glm::ivec2(row, col), loadedResourceId, tileTextureResource.GetGLTextureId() });
                }
            }
        }
    }
    
    auto RefreshGlobalMapFilesLambda = [&]()
    {
        sMapFileNames = fileutils::GetAllFilenamesAndFolderNamesInDirectory(NON_SANDBOXED_MAPS_FOLDER);
        sMapFileNameOptions = sMapFileNames;
        sMapFileNameOptions.insert(sMapFileNameOptions.begin(), "None");
        
        sOtherMapTextures = {"None"};
        sMapTextureNamesToDimensions.clear();
        
        auto mapTextureFileNames = fileutils::GetAllFilenamesAndFolderNamesInDirectory(NON_SANDBOXED_MAP_TEXTURES_FOLDER);
        for (auto mapName: mapTextureFileNames)
        {
            if (fileutils::IsDirectory(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName))
            {
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
        
        for (auto mapFileNameConnection: sMapFileNames)
        {
            sMapConnections[mapFileNameConnection]["top"] = std::make_pair(0, "None");
            sMapConnections[mapFileNameConnection]["right"] = std::make_pair(0, "None");
            sMapConnections[mapFileNameConnection]["bottom"] = std::make_pair(0, "None");
            sMapConnections[mapFileNameConnection]["left"] = std::make_pair(0, "None");
        }
        
        std::ifstream globalMapDataFile(NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH);
        
        if (globalMapDataFile.is_open())
        {
            std::stringstream buffer;
            buffer << globalMapDataFile.rdbuf();
            auto globalMapDataJson = nlohmann::json::parse(buffer.str());
            for (auto it = globalMapDataJson["map_connections"].begin(); it != globalMapDataJson["map_connections"].end(); ++it)
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
    };
    
    {
        static constexpr int TILEMAP_NAME_BUFFER_SIZE = 64;
        static char sMapNameBuffer[TILEMAP_NAME_BUFFER_SIZE] = { "" };
        static size_t sSelectedMapFileIndex = 0;
        static int sActivePanelType = 0;
        static std::string sLastLoadedMap;
        
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
        
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("MapName", &sMapNameBuffer[0], TILEMAP_NAME_BUFFER_SIZE);
        
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("  Load  "))
        {
            std::ifstream dataFile(NON_SANDBOXED_MAPS_FOLDER + sMapNameBuffer);
            if (strlen(sMapNameBuffer) > 0 && dataFile.is_open())
            {
                std::stringstream buffer;
                buffer << dataFile.rdbuf();
                auto contents = buffer.str();
                auto mapJson = nlohmann::json::parse(contents);
                
                auto& systemsEngine = CoreSystemsEngine::GetInstance();
                auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                
                DestroyMap();
                
                sDimensionsY = mapJson["metadata"]["rows"].get<int>();
                sDimensionsX = mapJson["metadata"]["cols"].get<int>();
                CreateMap(sDimensionsY, sDimensionsX);
                
                if (mapJson["metadata"].contains("palettes") && !mapJson["metadata"]["palettes"].is_null())
                {
                    for (const auto& paletteJson: mapJson["metadata"]["palettes"])
                    {
                        if (std::find_if(paletteNamesAndTextures.cbegin(), paletteNamesAndTextures.cend(), [=](const std::pair<std::string, resources::ResourceId>& entry)
                        {
                            return entry.first == paletteJson["name"].get<std::string>();
                        }) == paletteNamesAndTextures.cend())
                        {
                            ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::WARNING, "Map Loading Issue", "Missing palette: " + paletteJson["name"].get<std::string>());
                        }
                    }
                }
                
                {
                    auto rowCounter = 0;
                    auto colCounter = 0;
                    for (auto rowJson: mapJson["tiledata"]["bottomlayer"])
                    {
                        colCounter = 0;
                        for (auto tileJson: rowJson)
                        {
                            auto tileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(colCounter) + "," + std::to_string(rowCounter)));
                            tileSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILESETS_FOLDER + tileJson["plt"].get<std::string>() + ".png");
                            auto coordsString = strutils::StringSplit(tileJson["crd"].get<std::string>(), ',');
                            assert(coordsString.size() == 2);
                            editor_utils::SetNormalTileUniforms(tileSceneObject, glm::ivec2(std::stoi(coordsString[0]), std::stoi(coordsString[1])), TILE_UV_SIZE);
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
                                tileSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILESETS_FOLDER + tileJson["plt"].get<std::string>() + ".png");
                                auto coordsString = strutils::StringSplit(tileJson["crd"].get<std::string>(), ',');
                                assert(coordsString.size() == 2);
                                editor_utils::SetNormalTileUniforms(tileSceneObject, glm::ivec2(std::stoi(coordsString[0]), std::stoi(coordsString[1])), TILE_UV_SIZE);
                                colCounter++;
                            }
                            rowCounter++;
                        }
                    }
                }
                
                {
                    auto rowCounter = 0;
                    auto colCounter = 0;
                    
                    if (mapJson["tiledata"].contains("navmaplayer") && !mapJson["tiledata"]["navmaplayer"].is_null())
                    {
                        for (auto rowJson: mapJson["tiledata"]["navmaplayer"])
                        {
                            colCounter = 0;
                            for (auto tileJson: rowJson)
                            {
                                auto tileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(colCounter) + "," + std::to_string(rowCounter) + "_navmap"));
                                tileSceneObject->mShaderIntUniformValues[TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME] = tileJson["nvt"].get<int>();
                                editor_utils::SetNavmapTileUniforms(tileSceneObject);
                                colCounter++;
                            }
                            rowCounter++;
                        }
                    }
                }
                
                sLastLoadedMap = std::string(sMapNameBuffer);
                logging::Log(logging::LogType::INFO, "Successfully loaded %s", (resources::ResourceLoadingService::RES_DATA_ROOT + MAP_FILES_FOLDER + sMapNameBuffer).c_str());
            }
            else if (strlen(sMapNameBuffer) == 0 || std::string(sMapNameBuffer) == ".json")
            {
                ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "A name for the map must be specified");
            }
            else
            {
                ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Could not load map: " +  (resources::ResourceLoadingService::RES_DATA_ROOT + MAP_FILES_FOLDER + sMapNameBuffer));
            }
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(10.0f, 0.0f));
        ImGui::SameLine();
        if (ImGui::Button("  Save  "))
        {
            if (strlen(sMapNameBuffer) == 0 || std::string(sMapNameBuffer) == ".json")
            {
                ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "A name for the map must be specified");
            }
            else
            {
                bool shouldProceed = true;
                
                if (std::find(sMapFileNames.begin(), sMapFileNames.end(), std::string(sMapNameBuffer)) != sMapFileNames.end())
                {
                    shouldProceed = ospopups::ShowOkayCancelMessageBox(ospopups::MessageBoxType::INFO, "Overwrite Confirmation", "The existing map data and textures for " + std::string(sMapNameBuffer) + " will be overwritten. Proceed?" ) == 1;
                }
                
                if (shouldProceed)
                {
                    auto beginTimePoint = std::chrono::high_resolution_clock::now();
                    
                    nlohmann::json mapJson;
                    nlohmann::json mapMetaDataJson;
                    nlohmann::json mapPaletteDataJson;
                    nlohmann::json mapTileDataJson;
                    nlohmann::json bottomLayerMapTileDataJson;
                    nlohmann::json topLayerMapTileDataJson;
                    nlohmann::json navmapTileDataJson;
                    
                    mapMetaDataJson["rows"] = mGridRows;
                    mapMetaDataJson["cols"] = mGridCols;
                    
                    // Write palette lookup data
                    for (int i = 0; i < paletteNamesAndTextures.size(); ++i)
                    {
                        nlohmann::json paletteJson;
                        paletteJson["name"] = paletteNamesAndTextures[i].first;
                        mapPaletteDataJson.push_back(paletteJson);
                    }
                    mapMetaDataJson["palettes"] = mapPaletteDataJson;
                    
                    auto& systemsEngine = CoreSystemsEngine::GetInstance();
                    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                    
                    auto paletteNameLookup = [&](const resources::ResourceId textureResourceId)
                    {
                        auto iter = std::find_if(paletteNamesAndTextures.cbegin(), paletteNamesAndTextures.cend(), [=](const std::pair<std::string, resources::ResourceId>& entry)
                                                 {
                            return entry.second == textureResourceId;
                        });
                        
                        assert(iter != paletteNamesAndTextures.cend());
                        return iter->first;
                    };
                    
                    for (auto y = 0; y < mGridRows; ++y)
                    {
                        nlohmann::json bottomLayerRowJson;
                        nlohmann::json topLayerRowJson;
                        nlohmann::json navmapRowJson;
                        
                        for (auto x = 0; x < mGridCols; ++x)
                        {
                            {
                                auto bottomLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                                nlohmann::json tileJson;
                                
                                auto coords = editor_utils::GetTilesetCoords(bottomLayerTileSceneObject, TILE_UV_SIZE);
                                tileJson["crd"] = std::to_string(coords.r) + "," + std::to_string(coords.g);
                                tileJson["plt"] = paletteNameLookup(bottomLayerTileSceneObject->mTextureResourceId);
                                
                                bottomLayerRowJson.push_back(tileJson);
                            }
                            
                            {
                                auto topLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                                nlohmann::json tileJson;
                                
                                auto coords = editor_utils::GetTilesetCoords(topLayerTileSceneObject, TILE_UV_SIZE);
                                tileJson["crd"] = std::to_string(coords.r) + "," + std::to_string(coords.g);
                                tileJson["plt"] = paletteNameLookup(topLayerTileSceneObject->mTextureResourceId);
                                topLayerRowJson.push_back(tileJson);
                            }
                            
                            {
                                auto navmapTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_navmap"));
                                nlohmann::json tileJson;
                                tileJson["nvt"] = navmapTileSceneObject->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME);
                                navmapRowJson.push_back(tileJson);
                            }
                        }
                        bottomLayerMapTileDataJson.push_back(bottomLayerRowJson);
                        topLayerMapTileDataJson.push_back(topLayerRowJson);
                        navmapTileDataJson.push_back(navmapRowJson);
                    }
                    
                    mapTileDataJson["bottomlayer"] = bottomLayerMapTileDataJson;
                    mapTileDataJson["toplayer"] = topLayerMapTileDataJson;
                    mapTileDataJson["navmaplayer"] = navmapTileDataJson;
                    mapJson["tiledata"] = mapTileDataJson;
                    mapJson["metadata"] = mapMetaDataJson;
                    
                    std::ofstream outputMapJsonFile(NON_SANDBOXED_MAPS_FOLDER + std::string(sMapNameBuffer));
                    auto mapJsonString = mapJson.dump(4);
                    outputMapJsonFile.write(mapJsonString.c_str(), mapJsonString.size());
                    outputMapJsonFile.close();
                    
                    // Render map textures
                    std::vector<unsigned char> topLayerPixels(map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * 4);
                    std::vector<unsigned char> botLayerPixels(map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * 4);
                    
                    auto colOffset = (MAX_GRID_COLS - mGridCols)/2;
                    auto rowOffset = (MAX_GRID_ROWS - mGridRows)/2;
                    auto oddWidth = mGridCols % 2 != 0;
                    auto oddHeight = mGridRows % 2 != 0;
                    
                    std::unordered_map<std::string, SDL_Surface*> cachedTileImages;
                    
                    for (auto y = 0; y < mGridRows; ++y)
                    {
                        for (auto x = 0; x < mGridCols; ++x)
                        {
                            auto bottomLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                            auto topLayerTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_top"));
                            
                            auto botTileTexturePath = systemsEngine.GetResourceLoadingService().GetResourcePath(bottomLayerTileSceneObject->mTextureResourceId);
                            auto topTileTexturePath = systemsEngine.GetResourceLoadingService().GetResourcePath(topLayerTileSceneObject->mTextureResourceId);
                            
                            SDL_Surface* topTileSurface = cachedTileImages.contains(topTileTexturePath) ? cachedTileImages.at(topTileTexturePath) : IMG_Load((resources::ResourceLoadingService::RES_ROOT + topTileTexturePath).c_str());
                            SDL_Surface* botTileSurface = cachedTileImages.contains(botTileTexturePath) ? cachedTileImages.at(botTileTexturePath) : IMG_Load((resources::ResourceLoadingService::RES_ROOT + botTileTexturePath).c_str());
                            
                            cachedTileImages[topTileTexturePath] = topTileSurface;
                            cachedTileImages[botTileTexturePath] = botTileSurface;
                            
                            auto RenderTileOnSurface = [](SDL_Surface* surface, unsigned char* pixels, glm::ivec2 tilesetCoords, int x, int y, int colOffset, int rowOffset, bool oddWidth, bool oddHeight)
                            {
                                SDL_LockSurface(surface);
                                int tileRowCounter = 0;
                                for (auto tileImageY = tilesetCoords.r * TILESET_TILE_SIZE; tileImageY < (tilesetCoords.r + 1) * TILESET_TILE_SIZE; ++tileImageY)
                                {
                                    int tileColCounter = 0;
                                    for (auto tileImageX = tilesetCoords.g * TILESET_TILE_SIZE; tileImageX < (tilesetCoords.g + 1) * TILESET_TILE_SIZE; ++tileImageX)
                                    {
                                        Uint8 r,g,b,a;
                                        auto pixel = *(Uint32*)((Uint8*)surface->pixels + tileImageY * surface->pitch + tileImageX * surface->format->BytesPerPixel);
                                        SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
                                        
                                        pixels[(((y + rowOffset) * 16 + (oddHeight ? 8 : 0) + tileRowCounter) * map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * 4) + ((x + colOffset) * 16 + (oddWidth ? 8 : 0) + tileColCounter) * 4 + 0] = r;
                                        pixels[(((y + rowOffset) * 16 + (oddHeight ? 8 : 0) + tileRowCounter) * map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * 4) + ((x + colOffset) * 16 + (oddWidth ? 8 : 0) + tileColCounter) * 4 + 1] = g;
                                        pixels[(((y + rowOffset) * 16 + (oddHeight ? 8 : 0) + tileRowCounter) * map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * 4) + ((x + colOffset) * 16 + (oddWidth ? 8 : 0) + tileColCounter) * 4 + 2] = b;
                                        pixels[(((y + rowOffset) * 16 + (oddHeight ? 8 : 0) + tileRowCounter) * map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE * 4) + ((x + colOffset) * 16 + (oddWidth ? 8 : 0) + tileColCounter) * 4 + 3] = a;
                                        
                                        tileColCounter++;
                                    }
                                    tileRowCounter++;
                                }
                                SDL_UnlockSurface(surface);
                            };
                            
                            RenderTileOnSurface(topTileSurface, &topLayerPixels[0], editor_utils::GetTilesetCoords(topLayerTileSceneObject, TILE_UV_SIZE), x, y, colOffset, rowOffset, oddWidth, oddHeight);
                            RenderTileOnSurface(botTileSurface, &botLayerPixels[0], editor_utils::GetTilesetCoords(bottomLayerTileSceneObject, TILE_UV_SIZE), x, y, colOffset, rowOffset, oddWidth, oddHeight);
                        }
                    }
                    
                    for (const auto& cachedTileImageEntry: cachedTileImages)
                    {
                        SDL_FreeSurface(cachedTileImageEntry.second);
                    }
                    cachedTileImages.clear();
                    
                    auto mapName = fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer));
                    fileutils::CreateDirectory(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName);
                    rendering::ExportPixelsToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_bottom_layer.png", &botLayerPixels[0], map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE);
                    rendering::ExportPixelsToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_top_layer.png", &topLayerPixels[0], map_constants::CLIENT_WORLD_MAP_IMAGE_SIZE);
                    
                    // Render map navmap texture
                    unsigned char navmapPixels[map_constants::CLIENT_NAVMAP_IMAGE_SIZE * map_constants::CLIENT_NAVMAP_IMAGE_SIZE * 4] = { 0 };
                    
                    for (auto y = 0; y < mGridRows; ++y)
                    {
                        for (auto x = 0; x < mGridCols; ++x)
                        {
                            auto navmapTileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y) + "_navmap"));
                            
                            auto navmapTileTypeColor = networking::GetColorFromNavmapTileType(static_cast<networking::NavmapTileType>(navmapTileSceneObject->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)));
                            
                            for (auto tileImageY = 0; tileImageY < map_constants::CLIENT_NAVMAP_IMAGE_SIZE/64; ++tileImageY)
                            {
                                for (auto tileImageX = 0; tileImageX < map_constants::CLIENT_NAVMAP_IMAGE_SIZE/64; ++tileImageX)
                                {
                                    navmapPixels[(((y + rowOffset) * 2 + (oddHeight ? 1 : 0) + tileImageY) * map_constants::CLIENT_NAVMAP_IMAGE_SIZE * 4) + ((x + colOffset) * 2 + (oddWidth ? 1 : 0) + tileImageX) * 4 + 0] = navmapTileTypeColor.r;
                                    navmapPixels[(((y + rowOffset) * 2 + (oddHeight ? 1 : 0) + tileImageY) * map_constants::CLIENT_NAVMAP_IMAGE_SIZE * 4) + ((x + colOffset) * 2 + (oddWidth ? 1 : 0) + tileImageX) * 4 + 1] = navmapTileTypeColor.g;
                                    navmapPixels[(((y + rowOffset) * 2 + (oddHeight ? 1 : 0) + tileImageY) * map_constants::CLIENT_NAVMAP_IMAGE_SIZE * 4) + ((x + colOffset) * 2 + (oddWidth ? 1 : 0) + tileImageX) * 4 + 2] = navmapTileTypeColor.b;
                                    navmapPixels[(((y + rowOffset) * 2 + (oddHeight ? 1 : 0) + tileImageY) * map_constants::CLIENT_NAVMAP_IMAGE_SIZE * 4) + ((x + colOffset) * 2 + (oddWidth ? 1 : 0) + tileImageX) * 4 + 3] = navmapTileTypeColor.a;
                                }
                            }
                        }
                    }
                    
                    //                    rendering::ExportPixelsToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_navmap.png", navmapPixels, map_constants::CLIENT_NAVMAP_IMAGE_SIZE);
                    rendering::ExportPixelsToPNG(NON_SANDBOXED_NET_ASSETS_NAVMAPS_FOLDER + mapName + "_navmap.png", navmapPixels, map_constants::CLIENT_NAVMAP_IMAGE_SIZE);
                    
                    logging::Log(logging::LogType::ERROR, "Successfully saved %s", (NON_SANDBOXED_MAPS_FOLDER + std::string(sMapNameBuffer)).c_str());
                    
                    auto endTimePoint = std::chrono::high_resolution_clock::now();
                    
                    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::INFO, "Export complete", "Finished saving map file and exporting texture & navmap for " + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + ". Operation took " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - beginTimePoint).count()) + " secs");
                    
                    RefreshGlobalMapFilesLambda();
                }
            }
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(80.0f, 0.0f));
        ImGui::SameLine();
        ImGui::PushID("DeleteButton");
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button("  Delete  "))
        {
            if (strlen(sMapNameBuffer) == 0 || std::string(sMapNameBuffer) == ".json")
            {
                ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Map Deletion Error", "A name for the map must be specified.");
            }
            else if (std::find(sMapFileNames.begin(), sMapFileNames.end(), std::string(sMapNameBuffer)) == sMapFileNames.end())
            {
                ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Map Deletion Error", "The specified map could not be found.");
            }
            else if (ospopups::ShowOkayCancelMessageBox(ospopups::MessageBoxType::INFO, "Deletion Confirmation", "The existing map data, textures and associated connections for " + std::string(sMapNameBuffer) + " will be permanently deleted. Proceed? ") == 1)
            {
                const auto& mapName = std::string(sMapNameBuffer);
                if (sLastLoadedMap == std::string(sMapNameBuffer))
                {
                    memset(sMapNameBuffer, 0, sizeof(sMapNameBuffer));
                    DestroyMap();
                    CreateMap(sDimensionsY, sDimensionsX);
                }
                
                auto& systemsEngine = CoreSystemsEngine::GetInstance();
                auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                
                // Cleanup visual references
                mTopImageRefIndex = 0;
                mRightImageRefIndex = 0;
                mBottomImageRefIndex = 0;
                mLeftImageRefIndex = 0;
                sSelectedMapFileIndex = 0;
                sSelectedExportEntryMapIndex = 0;
                scene->RemoveSceneObject(TOP_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(RIGHT_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(LEFT_REF_IMAGE_SCENE_OBJECT_NAME);
                
                std::error_code errorCode;
                if (!std::filesystem::remove(NON_SANDBOXED_MAPS_FOLDER + mapName, errorCode))
                {
                    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Map Deletion Error", "The .json map file could not be deleted:\n" + errorCode.message());
                }
                
                if (!std::filesystem::remove_all(NON_SANDBOXED_MAP_TEXTURES_FOLDER + strutils::StringSplit(mapName, '.')[0], errorCode))
                {
                    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Map Deletion Error", "The map texture files could not be deleted:\n" + errorCode.message());
                }
                
                if (!std::filesystem::remove_all(NON_SANDBOXED_NET_ASSETS_NAVMAPS_FOLDER + strutils::StringSplit(mapName, '.')[0] + "_navmap.png", errorCode))
                {
                    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::ERROR, "Map Deletion Error", "The navmap files could not be deleted:\n" + errorCode.message());
                }

                
                std::ifstream globalMapDataFile(NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH);
                
                if (globalMapDataFile.is_open())
                {
                    std::stringstream buffer;
                    buffer << globalMapDataFile.rdbuf();
                    auto globalMapDataJson = nlohmann::json::parse(buffer.str());
                    
                    globalMapDataJson["map_connections"].erase(mapName);
                    globalMapDataJson["map_transforms"].erase(mapName);
                    
                    for (auto& connectionEntry: globalMapDataJson["map_connections"].items())
                    {
                        if (connectionEntry.value()["bottom"].get<std::string>() == mapName)
                        {
                            connectionEntry.value()["bottom"] = "None";
                        }
                        
                        if (connectionEntry.value()["left"].get<std::string>() == mapName)
                        {
                            connectionEntry.value()["left"] = "None";
                        }
                        
                        if (connectionEntry.value()["right"].get<std::string>() == mapName)
                        {
                            connectionEntry.value()["right"] = "None";
                        }
                        
                        if (connectionEntry.value()["top"].get<std::string>() == mapName)
                        {
                            connectionEntry.value()["top"] = "None";
                        }
                    }
                    
                    globalMapDataFile.close();
                    
                    std::ofstream outputGMDFile(NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH);
                    if (outputGMDFile.is_open())
                    {
                        outputGMDFile << globalMapDataJson.dump(4);
                        outputGMDFile.close();
                    }
                    
                    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::INFO, "Deleted all data for map " + mapName + " successfully.");
                }
                    
                RefreshGlobalMapFilesLambda();
            }
        }
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SeparatorText("Modify/Create");
        
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
        }
        
        ImGui::SeparatorText("Active Panel");
        if (ImGui::RadioButton("Editor", &sActivePanelType, 0))
        {
            if (mActivePanel != sActivePanelType)
            {
                mActivePanel = sActivePanelType;
                auto& systemsEngine = CoreSystemsEngine::GetInstance();
                auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                
                std::vector<strutils::StringId> sceneObjectNamesToRemove;
                
                scene->RemoveSceneObject(TOP_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(RIGHT_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(LEFT_REF_IMAGE_SCENE_OBJECT_NAME);
                
                mTopImageRefIndex = mRightImageRefIndex = mBottomImageRefIndex = mLeftImageRefIndex = 0;
                
                // Destroy all new (map stiching textures & scene objects)
                for (auto& sceneObject: scene->GetSceneObjects())
                {
                    if (strutils::StringEndsWith(sceneObject->mName.GetString(), "_stich"))
                    {
                        systemsEngine.GetResourceLoadingService().UnloadResource(sceneObject->mTextureResourceId);
                        sceneObjectNamesToRemove.push_back(sceneObject->mName);
                    }
                    else
                    {
                        sceneObject->mInvisible = false;
                    }
                }
                
                for (const auto& sceneObjectName: sceneObjectNamesToRemove)
                {
                    scene->RemoveSceneObject(sceneObjectName);
                }
                
                // Reset Camera Position
                mViewOptions.mCameraPosition.x = 0.0f;
                mViewOptions.mCameraPosition.y = 0.0f;
            }
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("MapStiching", &sActivePanelType, 1))
        {
            if (mActivePanel != sActivePanelType)
            {
                mActivePanel = sActivePanelType;
                auto& systemsEngine = CoreSystemsEngine::GetInstance();
                auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                
                for (auto& sceneObject: scene->GetSceneObjects())
                {
                    sceneObject->mInvisible = true;
                }
                
                scene->RemoveSceneObject(TOP_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(RIGHT_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(BOTTOM_REF_IMAGE_SCENE_OBJECT_NAME);
                scene->RemoveSceneObject(LEFT_REF_IMAGE_SCENE_OBJECT_NAME);
                
                mTopImageRefIndex = mRightImageRefIndex = mBottomImageRefIndex = mLeftImageRefIndex = 0;
                
                std::ifstream globalMapDataFile(NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH);
                if (globalMapDataFile.is_open())
                {
                    std::stringstream buffer;
                    buffer << globalMapDataFile.rdbuf();
                    auto globalMapDataJson = nlohmann::json::parse(buffer.str());
                    
                    for (auto mapTransformIter = globalMapDataJson["map_transforms"].begin(); mapTransformIter != globalMapDataJson["map_transforms"].end(); ++mapTransformIter)
                    {
                        auto mapName = mapTransformIter.key().substr(0, mapTransformIter.key().find(".json"));
                        
                        auto mapBottomLayer = scene->CreateSceneObject(strutils::StringId(mapName  + "_bottom_stich"));
                        mapBottomLayer->mPosition.x = mapTransformIter.value()["x"].get<float>() * game_constants::MAP_RENDERED_SCALE;
                        mapBottomLayer->mPosition.y = mapTransformIter.value()["y"].get<float>() * game_constants::MAP_RENDERED_SCALE;
                        mapBottomLayer->mPosition.z = map_constants::TILE_TOP_LAYER_Z + math::RandomFloat(0.01f, 0.05f);
                        mapBottomLayer->mScale *= game_constants::MAP_RENDERED_SCALE;
                        mapBottomLayer->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_bottom_layer.png", resources::ResourceReloadMode::DONT_RELOAD, resources::ResourceLoadingPathType::ABSOLUTE);
                        mapBottomLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
                        mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapTransformIter.value()["width"].get<float>();
                        mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapTransformIter.value()["height"].get<float>();
                        
                        auto mapTopLayer = scene->CreateSceneObject(strutils::StringId(mapName  + "_top_stich"));
                        mapTopLayer->mPosition.x = mapTransformIter.value()["x"].get<float>() * game_constants::MAP_RENDERED_SCALE;
                        mapTopLayer->mPosition.y = mapTransformIter.value()["y"].get<float>() * game_constants::MAP_RENDERED_SCALE;
                        mapTopLayer->mPosition.z = map_constants::TILE_TOP_LAYER_Z + math::RandomFloat(0.1f, 0.5f);
                        mapTopLayer->mScale *= game_constants::MAP_RENDERED_SCALE;
                        mapTopLayer->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_top_layer.png", resources::ResourceReloadMode::DONT_RELOAD, resources::ResourceLoadingPathType::ABSOLUTE);
                        mapTopLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
                        mapTopLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapTransformIter.value()["width"].get<float>();
                        mapTopLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapTransformIter.value()["height"].get<float>();
                        
                    }
                }
            }
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
                            
                            auto sideRefImagePath = NON_SANDBOXED_MAP_TEXTURES_FOLDER + sOtherMapTextures.at(sideImageRefIndex) + "/" + sOtherMapTextures.at(sideImageRefIndex) + "_bottom_layer.png";
                            
                            systemsEngine.GetResourceLoadingService().UnloadResource(sideRefImagePath, resources::ResourceLoadingPathType::ABSOLUTE);
                            sideRefImageSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(sideRefImagePath, resources::ResourceReloadMode::DONT_RELOAD, resources::ResourceLoadingPathType::ABSOLUTE);
                            
                            sideRefImageSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.5f;
                            sideRefImageSceneObject->mPosition.z = map_constants::TILE_TOP_LAYER_Z;
                            sideRefImageSceneObject->mPosition.x -= TILE_SIZE/2.0f;
                            sideRefImageSceneObject->mPosition.y += TILE_SIZE/2.0f;
                            
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
        
        ImGui::SeparatorText("Tilesets");
        ImGui::PushID("Tilesets");
        if (ImGui::BeginCombo(" ", paletteNamesAndTextures.at(mSelectedPaletteIndex).first.c_str()))
        {
            for (size_t n = 0U; n < paletteNamesAndTextures.size(); n++)
            {
                const bool isSelected = (mSelectedPaletteIndex == n);
                if (ImGui::Selectable(paletteNamesAndTextures.at(n).first.c_str(), isSelected))
                {
                    mSelectedPaletteIndex = static_cast<int>(n);
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
        
        if (mActiveLayer == map_constants::LayerType::NAVMAP)
        {
            ImGui::Text("Selected Tile: %s", networking::GetNavmapTileTypeName(static_cast<networking::NavmapTileType>(mSelectedNavmapTileType)));

            for (int i = 0; i < static_cast<int>(networking::NavmapTileType::COUNT); ++i)
            {
                if ((i % (TILESET_SIZE/TILESET_TILE_SIZE)) != 0)
                {
                    ImGui::SameLine();
                }
                
                auto navmapColor = networking::GetColorFromNavmapTileType(static_cast<networking::NavmapTileType>(i));
                if (static_cast<networking::NavmapTileType>(i) == networking::NavmapTileType::WALKABLE)
                {
                    // Distinguish walkable from solid in the ImGUI background
                    navmapColor = glm::ivec4(255.0f, 255.0f, 255.0f, 255.0f);
                }
                
                auto tileIndex = i;
                std::string tileName = std::to_string(i);
                
                ImGui::PushID(tileIndex);
                ImVec4 bgCol = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                ImVec4 tintCol = mSelectedPaletteTile == tileIndex ?
                    ImVec4(navmapColor.r/255.0f, navmapColor.g/255.0f, navmapColor.b/255.0f, navmapColor.a/255.0f) :
                    ImVec4(navmapColor.r/400.0f, navmapColor.g/400.0f, navmapColor.b/400.0f, navmapColor.a/400.0f);
                
                ImVec2 minUVs(0, 1.0f - ((TILESET_SIZE/TILESET_TILE_SIZE - 1) + 1) * TILE_UV_SIZE);
                ImVec2 maxUVs(TILE_UV_SIZE, 1.0f - (TILESET_SIZE/TILESET_TILE_SIZE - 1) * TILE_UV_SIZE);
                
                if (ImGui::ImageButton(tileName.c_str(), reinterpret_cast<void*>(BLANK_TILE_DATA.mTextureId), ImVec2(48.0f, 48.0f), minUVs, maxUVs, bgCol, tintCol))
                {
                    mSelectedNavmapTileType = static_cast<networking::NavmapTileType>(i);
                    mSelectedPaletteTile = tileIndex;
                }
                ImGui::PopID();
            }
            
        }
        else
        {
            ImGui::Text("Selected Tile: %d,%d", mPaletteTileData[mSelectedPaletteIndex][mSelectedPaletteTile].mTileCoords.x, mPaletteTileData[mSelectedPaletteIndex][mSelectedPaletteTile].mTileCoords.y);

            for (int row = 0; row < TILESET_SIZE/TILESET_TILE_SIZE; row++)
            {
                for (int col = 0; col < TILESET_SIZE/TILESET_TILE_SIZE; col++)
                {
                    if (col > 0)
                    {
                        ImGui::SameLine();
                    }
                    
                    auto tileIndex = row * (TILESET_SIZE/TILESET_TILE_SIZE) + col;
                    std::string tileName = std::to_string(row) + "," + std::to_string(col);
                    auto tileTextureId = tileIndex >= mPaletteTileData[mSelectedPaletteIndex].size() ? 0 : mPaletteTileData[mSelectedPaletteIndex][tileIndex].mTextureId;
                    
                    ImGui::PushID(tileIndex);
                    
                    if (tileTextureId)
                    {
                        ImVec4 bgCol = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                        ImVec4 tintCol = mSelectedPaletteTile == tileIndex ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 0.7f);
                        
                        ImVec2 minUVs(mPaletteTileData[mSelectedPaletteIndex][tileIndex].mTileCoords.g * TILE_UV_SIZE, 1.0f - ((TILESET_SIZE/TILESET_TILE_SIZE - 1 - mPaletteTileData[mSelectedPaletteIndex][tileIndex].mTileCoords.r) + 1) * TILE_UV_SIZE);
                        ImVec2 maxUVs((mPaletteTileData[mSelectedPaletteIndex][tileIndex].mTileCoords.g + 1) * TILE_UV_SIZE, 1.0f - (TILESET_SIZE/TILESET_TILE_SIZE - 1 - mPaletteTileData[mSelectedPaletteIndex][tileIndex].mTileCoords.r) * TILE_UV_SIZE);
                        
                        if (ImGui::ImageButton(tileName.c_str(), reinterpret_cast<void*>(tileTextureId), ImVec2(48.0f, 48.0f), minUVs, maxUVs, bgCol, tintCol))
                        {
                            mSelectedPaletteTile = tileIndex;
                        }
                    }
                    
                    
                    ImGui::PopID();
                }
            }
        }
        
        ImGui::End();
    }
    
    {
        static int layerIndex = 0;
        ImGui::Begin("Layers", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        for (int i = 0; i < static_cast<int>(map_constants::LayerType::LAYER_COUNT); ++i)
        {
            if (i != 0)
            {
                ImGui::SameLine();
                ImGui::Dummy(ImVec2(20.0f * i, 0.0f));
            }
            
            std::string layerName;
            switch (static_cast<map_constants::LayerType>(i))
            {
                case map_constants::LayerType::BOTTOM_LAYER: layerName = "Bottom Layer"; break;
                case map_constants::LayerType::TOP_LAYER: layerName = "Top Layer"; break;
                case map_constants::LayerType::NAVMAP: layerName = "Navmap"; break;
                default: break;
            }
            
            if (ImGui::RadioButton(layerName.c_str(), &layerIndex, i))
            {
                mActiveLayer = static_cast<map_constants::LayerType>(layerIndex);
            }
            
            ImGui::SameLine();
            ImGui::PushID(std::string(std::to_string(i) + "LayerVisible").c_str());
            ImGui::SetNextItemWidth(100.0f);
            ImGui::SliderFloat("Visibility", &mLayersVisibility[i], 0.0f, 1.0f);
            ImGui::PopID();
            
        }
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
        ImGui::PushID("ExportEntryMap");

        if (ImGui::BeginCombo(" ", sMapFileNames.at(sSelectedExportEntryMapIndex).c_str()))
        {
            for (int n = 0; n < static_cast<int>(sMapFileNames.size()); n++)
            {
                const bool isSelected = (sSelectedExportEntryMapIndex == n);
                if (ImGui::Selectable(sMapFileNames.at(n).c_str(), isSelected))
                {
                    sSelectedExportEntryMapIndex = n;
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
        if (ImGui::Button("Save Global Map Data"))
        {
            std::ofstream globalMapDataFile(NON_SANDBOXED_NET_ASSETS_MAP_GLOBAL_DATA_PATH);
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
                nlohmann::json mapTransformsJson;
                std::function<void(const std::string&, const std::string&, const std::string&)> MapPositionCalculationLambda = [&](const std::string& mapName, const std::string& previousMapName, const std::string& previousToCurrentConnectionDirection) -> void
                {
                    if (mapTransformsJson.count(mapName))
                    {
                        return;
                    }
                    
                    glm::vec3 previousMapPosition(0.0f, 0.0f, map_constants::TILE_BOTTOM_LAYER_Z);
                    glm::vec2 previousMapDimensions(0.0f, 0.0f);
                                        
                    nlohmann::json currentMapTransformJson;
                    const auto& mapDimensions = sMapTextureNamesToDimensions.at(mapName.substr(0, mapName.find(".json")));
                    currentMapTransformJson["x"] = 0.0f;
                    currentMapTransformJson["y"] = 0.0f;
                    currentMapTransformJson["width"] = mapDimensions.x;
                    currentMapTransformJson["height"] = mapDimensions.y;
                                        
                    if (!previousMapName.empty())
                    {
                        assert(mapTransformsJson.count(previousMapName));
                        previousMapPosition.x = mapTransformsJson[previousMapName]["x"].get<float>();
                        previousMapPosition.y = mapTransformsJson[previousMapName]["y"].get<float>();
                        
                        previousMapDimensions.x = mapTransformsJson[previousMapName]["width"].get<float>();
                        previousMapDimensions.y = mapTransformsJson[previousMapName]["height"].get<float>();
                        
                        glm::vec3 updatedMapPosition = previousMapPosition;
                        
                        logging::Log(logging::LogType::INFO, "Processing %s", mapName.c_str());
                        
                        if (previousToCurrentConnectionDirection == "top")
                        {
                            updatedMapPosition.y += (mapDimensions.y/2.0f + previousMapDimensions.y/2.0f);
                        }
                        else if (previousToCurrentConnectionDirection == "right")
                        {
                            updatedMapPosition.x += (mapDimensions.x/2.0f + previousMapDimensions.x/2.0f);
                        }
                        else if (previousToCurrentConnectionDirection == "bottom")
                        {
                            updatedMapPosition.y -= (mapDimensions.y/2.0f + previousMapDimensions.y/2.0f);
                        }
                        else if (previousToCurrentConnectionDirection == "left")
                        {
                            updatedMapPosition.x -= (mapDimensions.x/2.0f + previousMapDimensions.x/2.0f);
                        }
                        
                        currentMapTransformJson["x"] = updatedMapPosition.x;
                        currentMapTransformJson["y"] = updatedMapPosition.y;
                    }
                    
                    mapTransformsJson[mapName] = currentMapTransformJson;
                    
                    const auto& currentMapConnections = sMapConnections.at(mapName);
                    if (currentMapConnections.at("top").second != "None")
                    {
                        MapPositionCalculationLambda(currentMapConnections.at("top").second, mapName, "top");
                    }
                    
                    if (currentMapConnections.at("right").second != "None")
                    {
                        MapPositionCalculationLambda(currentMapConnections.at("right").second, mapName, "right");
                    }
                    
                    if (currentMapConnections.at("bottom").second != "None")
                    {
                        MapPositionCalculationLambda(currentMapConnections.at("bottom").second, mapName, "bottom");
                    }
                    
                    if (currentMapConnections.at("left").second != "None")
                    {
                        MapPositionCalculationLambda(currentMapConnections.at("left").second, mapName, "left");
                    }
                };
                
                MapPositionCalculationLambda(sMapFileNames.at(sSelectedExportEntryMapIndex), "", "");
                
                globalMapDataJson["map_connections"] = exportedConnectionsJson;
                globalMapDataJson["map_transforms"] = mapTransformsJson;

                globalMapDataFile << globalMapDataJson.dump(4);
                globalMapDataFile.close();
            }
            ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::INFO, "Export complete", "Finished exporting global map data.");
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


#include <editor/EditorImGuiCommands.inc>

///------------------------------------------------------------------------------------------------
