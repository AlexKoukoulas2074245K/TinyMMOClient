
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
        
        std::ifstream globalMapDataFile(NON_SANDBOXED_MAP_GLOBAL_DATA_PATH);
        
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
        static char sMapNameBuffer[TILEMAP_NAME_BUFFER_SIZE] = { "entry_map.json" };
        static size_t sSelectedMapFileIndex = 0;
        static int sActivePanelType = 0;
        
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
                    
                    auto mapName = fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer));
                    fileutils::CreateDirectory(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName);
                    rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_bottom_layer.png", bottomLayerSceneObjects, rendering::BlurStep::DONT_BLUR);
                    rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_top_layer.png", topLayerSceneObjects, rendering::BlurStep::DONT_BLUR);
                    
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
                    rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_navmap.png", bottomLayerSceneObjects, rendering::BlurStep::DONT_BLUR);
                    
                    logging::Log(logging::LogType::ERROR, "Successfully saved %s", (NON_SANDBOXED_MAPS_FOLDER + std::string(sMapNameBuffer)).c_str());
                    
                    auto endTimePoint = std::chrono::high_resolution_clock::now();
                    
                    ospopups::ShowInfoMessageBox(ospopups::MessageBoxType::INFO, "Export complete", "Finished saving map file and exporting texture & navmap for " + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + ". Operation took " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - beginTimePoint).count()) + " secs");
                    
                    RefreshGlobalMapFilesLambda();
                }
            }
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
                
                // Destroy all new (map stiching textures & scene objects)
                for (auto& sceneObject: scene->GetSceneObjects())
                {
                    if (sceneObject->mInvisible)
                    {
                        sceneObject->mInvisible = false;
                    }
                    else
                    {
                        systemsEngine.GetResourceLoadingService().UnloadResource(sceneObject->mTextureResourceId);
                        sceneObjectNamesToRemove.push_back(sceneObject->mName);
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
                
                std::ifstream globalMapDataFile(NON_SANDBOXED_MAP_GLOBAL_DATA_PATH);
                if (globalMapDataFile.is_open())
                {
                    std::stringstream buffer;
                    buffer << globalMapDataFile.rdbuf();
                    auto globalMapDataJson = nlohmann::json::parse(buffer.str());
                    
                    for (auto mapTransformIter = globalMapDataJson["map_transforms"].begin(); mapTransformIter != globalMapDataJson["map_transforms"].end(); ++mapTransformIter)
                    {
                        auto mapName = mapTransformIter.key().substr(0, mapTransformIter.key().find(".json"));
                        
                        auto mapBottomLayer = scene->CreateSceneObject(strutils::StringId(mapName  + "_bottom"));
                        mapBottomLayer->mPosition.x = mapTransformIter.value()["x"].get<float>() * game_constants::MAP_RENDERED_SCALE;
                        mapBottomLayer->mPosition.y = mapTransformIter.value()["y"].get<float>() * game_constants::MAP_RENDERED_SCALE;
                        mapBottomLayer->mPosition.z = map_constants::TILE_TOP_LAYER_Z + math::RandomFloat(0.01f, 0.05f);
                        mapBottomLayer->mScale *= game_constants::MAP_RENDERED_SCALE;
                        mapBottomLayer->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(NON_SANDBOXED_MAP_TEXTURES_FOLDER + mapName + "/" + mapName + "_bottom_layer.png", resources::ResourceReloadMode::DONT_RELOAD, resources::ResourceLoadingPathType::ABSOLUTE);
                        mapBottomLayer->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + "world_map.vs");
                        mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_width")] = mapTransformIter.value()["width"].get<float>();
                        mapBottomLayer->mShaderFloatUniformValues[strutils::StringId("map_height")] = mapTransformIter.value()["height"].get<float>();
                        
                        auto mapTopLayer = scene->CreateSceneObject(strutils::StringId(mapName  + "_top"));
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
            auto mapTileFileNames = fileutils::GetAllFilenamesAndFolderNamesInDirectory(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TILES_FOLDER);
            
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
                
                MapPositionCalculationLambda(ENTRY_MAP_NAME, "", "");
                
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

