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
static constexpr int DEFAULT_GRID_ROWS = 32;
static constexpr int DEFAULT_GRID_COLS = 32;
static const float TILE_SIZE = 0.013f;
static const float HIGHLIGHTED_TILE_SIZE = 0.014f;
static const float TILE_DEFAULT_Z = 0.1f;
static const float TILE_HIGHLIGHTED_Z = 0.2f;
static const glm::vec3 TILE_DEFAULT_SCALE = glm::vec3(TILE_SIZE);
static const glm::vec3 TILE_HIGHLIGHTED_SCALE = glm::vec3(HIGHLIGHTED_TILE_SIZE);

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
    
    mSelectedPaletteTile = 0;
    CreateGrid(DEFAULT_GRID_ROWS, DEFAULT_GRID_COLS);
}

///------------------------------------------------------------------------------------------------

void Editor::Update(const float dtMillis)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& inputStateManager = systemsEngine.GetInputStateManager();
    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
    
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
    
    std::vector<scene::SceneObject*> highlightedTileCandidates;
    
    for (auto y = 0; y < mGridRows; ++y)
    {
        for (auto x = 0; x < mGridCols; ++x)
        {
            auto tile = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
            auto rect = scene_object_utils::GetSceneObjectBoundingRect(*tile);
            
            tile->mPosition.z = TILE_DEFAULT_Z;
            tile->mScale = TILE_DEFAULT_SCALE;
            
            auto cursorInTile = math::IsPointInsideRectangle(rect.bottomLeft, rect.topRight, worldTouchPos);
            if (cursorInTile)
            {
                highlightedTileCandidates.push_back(tile.get());
            }
        }
    }
    
    if (!highlightedTileCandidates.empty())
    {
        std::sort(highlightedTileCandidates.begin(), highlightedTileCandidates.end(), [&](scene::SceneObject* lhs, scene::SceneObject* rhs){ return glm::distance(glm::vec2(lhs->mPosition.x, lhs->mPosition.y), worldTouchPos) < glm::distance(glm::vec2(rhs->mPosition.x, rhs->mPosition.y), worldTouchPos); });
        highlightedTileCandidates.front()->mPosition.z = TILE_HIGHLIGHTED_Z;
        highlightedTileCandidates.front()->mScale = TILE_HIGHLIGHTED_SCALE;
        
        if (inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON))
        {
            highlightedTileCandidates.front()->mTextureResourceId = mPaletteTileData[mSelectedPaletteTile].mResourceId;
        }
    }
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
    static const std::string MAP_FILES_FOLDER = resources::ResourceLoadingService::RES_DATA_ROOT + "world/maps/";
    static const std::string TILES_FOLDER = resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/map_tiles/";
    
    {
        static constexpr int TILEMAP_NAME_BUFFER_SIZE = 64;
        static char sTileMapNameBuffer[TILEMAP_NAME_BUFFER_SIZE] = {};
        static std::vector<std::string> sMapFileNames;
        static size_t sSelectedMapFileIndex = 0;
        
        if (sMapFileNames.empty())
        {
            sMapFileNames = fileutils::GetAllFilenamesInDirectory(MAP_FILES_FOLDER);
        }
        
        ImGui::Begin("Tile Map File", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        ImGui::SeparatorText("Import/Export");
        
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
                        memset(sTileMapNameBuffer, 0, TILEMAP_NAME_BUFFER_SIZE);
                        strcpy(sTileMapNameBuffer, sMapFileNames.at(n).c_str());
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
        
        if (strlen(sTileMapNameBuffer) == 0)
        {
            if (sMapFileNames.empty())
            {
                strcpy(sTileMapNameBuffer, "map.json");
            }
            else
            {
                strcpy(sTileMapNameBuffer, sMapFileNames.front().c_str());
            }
        }
        
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputText("MapName", &sTileMapNameBuffer[0], TILEMAP_NAME_BUFFER_SIZE);
        
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        if (ImGui::Button("  Load  "))
        {
            std::ifstream dataFile(MAP_FILES_FOLDER + sTileMapNameBuffer);
            if (dataFile.is_open())
            {
                std::stringstream buffer;
                buffer << dataFile.rdbuf();
                auto contents = buffer.str();
                auto mapJson = nlohmann::json::parse(contents);
                
                auto& systemsEngine = CoreSystemsEngine::GetInstance();
                auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
                
                for (auto y = 0; y < mGridRows; ++y)
                {
                    for (auto x = 0; x < mGridCols; ++x)
                    {
                        scene->RemoveSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                    }
                }
                
                CreateGrid(mapJson["metadata"]["rows"].get<int>(), mapJson["metadata"]["cols"]);
                
                auto rowCounter = 0;
                auto colCounter = 0;
                for (auto rowJson: mapJson["tiledata"])
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
        }
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(20.0f, 0.0f));
        ImGui::SameLine();
        if (ImGui::Button("  Save  "))
        {
            nlohmann::json mapJson;
            nlohmann::json mapMetaDataJson;
            nlohmann::json mapTileDataJson;
            
            mapMetaDataJson["rows"] = mGridRows;
            mapMetaDataJson["cols"] = mGridCols;
            
            auto& systemsEngine = CoreSystemsEngine::GetInstance();
            auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
            
            for (auto y = 0; y < mGridRows; ++y)
            {
                nlohmann::json rowJson;
                for (auto x = 0; x < mGridCols; ++x)
                {
                    auto tileSceneObject = scene->FindSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
                    nlohmann::json tileJson;
                    tileJson["tile_texture"] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourcePath(tileSceneObject->mTextureResourceId);
                    rowJson.push_back(tileJson);
                }
                mapTileDataJson.push_back(rowJson);
            }
            mapJson["tiledata"] = mapTileDataJson;
            mapJson["metadata"] = mapMetaDataJson;
            
            std::ofstream outputMapJsonFile("/Users/Code/TinyMMOClient/assets/data/world/maps/" + std::string(sTileMapNameBuffer));
            auto mapJsonString = mapJson.dump(4);
            outputMapJsonFile.write(mapJsonString.c_str(), mapJsonString.size());
        }
        
        ImGui::SeparatorText("Modify/Create");
        static int sDimensionsX = DEFAULT_GRID_COLS;
        static int sDimensionsY = DEFAULT_GRID_ROWS;
        
        ImGui::InputInt("x", &sDimensionsX);
        ImGui::InputInt("y", &sDimensionsY);
        ImGui::End();
    }
    
    {
        ImGui::Begin("Tile Map Palette", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        ImGui::SeparatorText("Tiles");
        
        static constexpr int GRID_COLS = 4;
        static int sGridRows = 1;
        
        if (mPaletteTileData.empty())
        {
            auto mapTileFileNames = fileutils::GetAllFilenamesInDirectory(TILES_FOLDER);
            
            for (const auto& mapTileFileName: mapTileFileNames)
            {
                auto loadedResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(TILES_FOLDER + mapTileFileName);
                const auto& tileTextureResource = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResource<resources::TextureResource>(loadedResourceId);
                
                if (mapTileFileName == "empty.png")
                {
                    mPaletteTileData.insert(mPaletteTileData.begin(), { loadedResourceId, tileTextureResource.GetGLTextureId(), strutils::StringSplit(mapTileFileName, '.')[0] });
                }
                else
                {
                    mPaletteTileData.push_back({ loadedResourceId, tileTextureResource.GetGLTextureId(), strutils::StringSplit(mapTileFileName, '.')[0] });
                }
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
                    ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // Black background
                    ImVec4 tint_col = mSelectedPaletteTile == tileIndex ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 0.8f);
                                        
                    if (ImGui::ImageButton(tileName.c_str(), reinterpret_cast<void*>(tileTextureId), ImVec2(64.0f, 64.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), bg_col, tint_col))
                    {
                        mSelectedPaletteTile = tileIndex;
                    }
                }
                
                
                ImGui::PopID();
            }
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

///------------------------------------------------------------------------------------------------

void Editor::CreateGrid(const int gridRows, const int gridCols)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(EDITOR_SCENE);
    
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
            auto tile = scene->CreateSceneObject(strutils::StringId(std::to_string(x) + "," + std::to_string(y)));
            tile->mPosition.x = gridStartingX + x * TILE_SIZE;
            tile->mPosition.y = gridStartingY - y * TILE_SIZE;
            tile->mPosition.z = TILE_DEFAULT_Z;
            tile->mScale = TILE_DEFAULT_SCALE;
            tile->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/map_tiles/empty.png");
        }
    }
}

///------------------------------------------------------------------------------------------------