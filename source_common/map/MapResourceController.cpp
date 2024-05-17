///------------------------------------------------------------------------------------------------
///  MapResourceController.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 17/05/2024
///------------------------------------------------------------------------------------------------

#include <game/events/EventSystem.h>
#include <map/GlobalMapDataRepository.h>
#include <map/MapConstants.h>
#include <map/MapResourceController.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

static constexpr int MAX_MAP_LOADING_RECURSE_LEVEL = 2;

///------------------------------------------------------------------------------------------------

MapResourceController::MapResourceController(const strutils::StringId& initialMapName)
    : mCurrentMapName(initialMapName)
{
    LoadMapResourceTree(mCurrentMapName, 0, false);
}

///------------------------------------------------------------------------------------------------

MapResources MapResourceController::GetMapResources(const strutils::StringId &mapName)
{
    std::lock_guard<std::mutex> mapResourceLock(mMapResourceMutex);
    assert(mLoadedMapResourceTree.contains(mapName));
    return mLoadedMapResourceTree.at(mapName);
}

///------------------------------------------------------------------------------------------------

std::unordered_map<strutils::StringId, MapResources, strutils::StringIdHasher> MapResourceController::GetAllLoadedMapResources()
{
    std::lock_guard<std::mutex> mapResourceLock(mMapResourceMutex);
    return mLoadedMapResourceTree;
}

///------------------------------------------------------------------------------------------------

void MapResourceController::Update(const strutils::StringId& currentMapName)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    
    // Map change invalidation and reloading
    if (currentMapName != mCurrentMapName)
    {
        mCurrentMapName = currentMapName;
        
        std::lock_guard<std::mutex> mapResourceLock(mMapResourceMutex);
        for (auto& mapResourceEntry: mLoadedMapResourceTree)
        {
            mapResourceEntry.second.mMapResourcesState = MapResourcesState::INVALIDATED;
        }
            
        systemsEngine.GetResourceLoadingService().SetAsyncLoading(true);
        LoadMapResourceTree(mCurrentMapName, 0, true);
        systemsEngine.GetResourceLoadingService().SetAsyncLoading(false);
        
        for (auto iter = mLoadedMapResourceTree.begin(); iter != mLoadedMapResourceTree.end();)
        {
            if (iter->second.mMapResourcesState == MapResourcesState::INVALIDATED)
            {
                systemsEngine.GetResourceLoadingService().UnloadResource(iter->second.mBottomLayerTextureResourceId);
                systemsEngine.GetResourceLoadingService().UnloadResource(iter->second.mTopLayerTextureResourceId);
                systemsEngine.GetResourceLoadingService().UnloadResource(iter->second.mNavmapImageResourceId);
                events::EventSystem::GetInstance().DispatchEvent<events::MapSupersessionEvent>(iter->first);
                iter = mLoadedMapResourceTree.erase(iter);
            }
            else
            {
                iter++;
            }
        }
    }
    
    // Check for loaded async map resources
    const auto& resourceService = systemsEngine.GetResourceLoadingService();
    for (auto& mapResourceEntry: mLoadedMapResourceTree)
    {
        if (mapResourceEntry.second.mMapResourcesState == MapResourcesState::PENDING)
        {
            if (resourceService.HasLoadedResource(mapResourceEntry.second.mBottomLayerTextureResourceId) &&
                resourceService.HasLoadedResource(mapResourceEntry.second.mTopLayerTextureResourceId) &&
                resourceService.HasLoadedResource(mapResourceEntry.second.mNavmapImageResourceId))
            {
                mapResourceEntry.second.mMapResourcesState = MapResourcesState::LOADED;
                events::EventSystem::GetInstance().DispatchEvent<events::MapResourcesReadyEvent>(mapResourceEntry.first);
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void MapResourceController::LoadMapResourceTree(const strutils::StringId& mapName, const int recurseLevel, const bool asyncLoading)
{
    if (recurseLevel > MAX_MAP_LOADING_RECURSE_LEVEL || mapName == map_constants::NO_CONNECTION_NAME)
    {
        return;
    }
    
    LoadMapResources(mapName, asyncLoading);
    
    auto& globalMapDataRepo = GlobalMapDataRepository::GetInstance();
    const auto& mapDefinition = globalMapDataRepo.GetMapDefinition(mapName);
    
    LoadMapResourceTree(mapDefinition.mMapConnections[static_cast<int>(MapConnectionDirection::NORTH)], recurseLevel + 1, asyncLoading);
    LoadMapResourceTree(mapDefinition.mMapConnections[static_cast<int>(MapConnectionDirection::EAST)], recurseLevel + 1, asyncLoading);
    LoadMapResourceTree(mapDefinition.mMapConnections[static_cast<int>(MapConnectionDirection::SOUTH)], recurseLevel + 1, asyncLoading);
    LoadMapResourceTree(mapDefinition.mMapConnections[static_cast<int>(MapConnectionDirection::WEST)], recurseLevel + 1, asyncLoading);
}


///------------------------------------------------------------------------------------------------

void MapResourceController::LoadMapResources(const strutils::StringId& mapName, const bool asyncLoading)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    const auto& mapTexturesPath = resources::ResourceLoadingService::RES_TEXTURES_ROOT + "world/maps/" + mapName.GetString() + "/" + mapName.GetString();
    
    // We've already loaded the resources for this map
    if (mLoadedMapResourceTree.contains(mapName))
    {
        mLoadedMapResourceTree[mapName].mMapResourcesState = MapResourcesState::LOADED;
        return;
    }
    
    auto mapTopLayerTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(mapTexturesPath + "_top_layer.png");
    auto mapBottomLayerTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(mapTexturesPath + "_bottom_layer.png");
    auto mapNavmapTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(mapTexturesPath + "_navmap.png");
    
    MapResources mapResources = { asyncLoading ? MapResourcesState::PENDING : MapResourcesState::LOADED, mapTopLayerTextureResourceId, mapBottomLayerTextureResourceId, mapNavmapTextureResourceId };
    mLoadedMapResourceTree.emplace(std::make_pair(mapName, std::move(mapResources)));
}
