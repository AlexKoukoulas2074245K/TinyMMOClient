
#if defined(USE_IMGUI)
void Editor::CreateDebugWidgets()
{
    {
        static constexpr int TILEMAP_NAME_BUFFER_SIZE = 64;
        static char sMapNameBuffer[TILEMAP_NAME_BUFFER_SIZE] = {};
        static std::vector<std::string> sMapFileNames;
        static size_t sSelectedMapFileIndex = 0;
        
        if (sMapFileNames.empty())
        {
            sMapFileNames = fileutils::GetAllFilenamesInDirectory(resources::ResourceLoadingService::RES_DATA_ROOT + MAP_FILES_FOLDER);
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
                strcpy(sMapNameBuffer, "map.json");
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
            rendering::ExportToPNG(NON_SANDBOXED_MAP_TEXTURES_FOLDER + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + "_navmap.png", bottomLayerSceneObjects, rendering::BlurStep::BLUR);
            
            logging::Log(logging::LogType::ERROR, "Successfully saved %s", (NON_SANDBOXED_MAPS_FOLDER + std::string(sMapNameBuffer)).c_str());
            
            auto endTimePoint = std::chrono::high_resolution_clock::now();
            
            ospopups::ShowMessageBox(ospopups::MessageBoxType::INFO, "Export complete", "Finished saving map file and exporting texture & navmap for " + fileutils::GetFileNameWithoutExtension(std::string(sMapNameBuffer)) + ". Operation took " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(endTimePoint - beginTimePoint).count()) + " secs");
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
            strcpy(sMapNameBuffer, "map.json");
        }
        
        ImGui::SeparatorText("Side Image References");
        static std::vector<std::string> sOtherMapTextures {"None"};
        static std::unordered_map<std::string, glm::vec2> sMapTextureNamesToDimensions;
        
        if (sOtherMapTextures.size() == 1)
        {
            auto mapTextureFileNames = fileutils::GetAllFilenamesInDirectory(resources::ResourceLoadingService::RES_TEXTURES_ROOT + MAP_FILES_FOLDER);
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
        }
        
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
                            sideRefImageSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + MAP_FILES_FOLDER + sOtherMapTextures.at(sideImageRefIndex) + "_bottom_layer.png");
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
        ImGui::Begin("Map Connections Editor", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
        static std::vector<std::string> sMapFileNames;
        static std::vector<std::string> sMapFileNameConnections;
        static std::vector<std::string> sMapFileNameConnections;
        static std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sMapConnections;
        
        auto RefreshMapFilesLambda = [&]()
        {
            sMapFileNames = fileutils::GetAllFilenamesInDirectory(NON_SANDBOXED_MAPS_FOLDER);
            sMapFileNameConnections = sMapFileNames;
            sMapFileNameConnections.insert(sMapFileNameConnections.begin(), "None");
            
            sMapConnections.clear();
            
        };
        
        if (sMapFileNames.empty())
        {
            RefreshMapFilesLambda();
        }
        
        ImGui::SeparatorText("Utilities");
        if (ImGui::Button("Refresh Map Data"))
        {
            RefreshMapFilesLambda();
        }
        
        ImGui::SeparatorText("Map Connections");
        for (auto mapFileName: sMapFileNames)
        {
            if (ImGui::CollapsingHeader(mapFileName.c_str(), ImGuiTreeNodeFlags_None))
            {
                auto connectionCreationLambda = [](const std::string& mapName, const std::string& direction)
                {
                    ImGui::PushID((mapName + direction + "combo").c_str());
                    if (ImGui::BeginCombo(" ", sMapFileNameConnections.at(sideImageRefIndex).c_str()))
                    {
                        for (int n = 0; n < static_cast<int>(sOtherMapTextures.size()); n++)
                        {
                            const bool isSelected = (sideImageRefIndex == n);
                            if (ImGui::Selectable(sOtherMapTextures.at(n).c_str(), isSelected))
                            {
                                sideImageRefIndex = n;
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
                
                connectionCreationLambda("Top");
                connectionCreationLambda("Right");
                connectionCreationLambda("Bottom");
                connectionCreationLambda("Left");
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
