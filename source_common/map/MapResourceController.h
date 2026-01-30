///------------------------------------------------------------------------------------------------
///  MapResourceController.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 17/05/2024
///------------------------------------------------------------------------------------------------

#ifndef MapResourceController_h
#define MapResourceController_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/ResourceLoadingService.h>
#include <memory>
#include <mutex>
#include <net_common/Navmap.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

enum class MapResourcesState
{
    LOADED,
    PENDING,
    INVALIDATED
};

///------------------------------------------------------------------------------------------------

struct MapResources
{
    MapResourcesState mMapResourcesState = MapResourcesState::INVALIDATED;
    resources::ResourceId mTopLayerTextureResourceId = 0;
    resources::ResourceId mBottomLayerTextureResourceId = 0;
    resources::ResourceId mNavmapImageResourceId = 0;
    std::shared_ptr<network::Navmap> mNavmap = nullptr;
};

///------------------------------------------------------------------------------------------------

class MapResourceController final
{
public:
    MapResourceController(const strutils::StringId& initialMapName);
    ~MapResourceController() = default;
    
    MapResourceController(const MapResourceController&) = delete;
    MapResourceController(MapResourceController&&) = delete;
    const MapResourceController& operator = (const MapResourceController&) = delete;
    MapResourceController& operator = (MapResourceController&&) = delete;
    
    MapResources GetMapResources(const strutils::StringId& mapName);
    std::unordered_map<strutils::StringId, MapResources, strutils::StringIdHasher> GetAllLoadedMapResources();
    
    void Update(const strutils::StringId& currentMapName);
    void LoadMapResourceTree(const strutils::StringId& mapName, const int recurseLevel, const bool asyncLoading);
    void LoadMapResources(const strutils::StringId& mapName, const bool asyncLoading);
    
    void CreateDebugWidgets();

private:
    std::mutex mMapResourceMutex;
    strutils::StringId mCurrentMapName;
    std::unordered_map<strutils::StringId, MapResources, strutils::StringIdHasher> mLoadedMapResourceTree;
};

///------------------------------------------------------------------------------------------------

#endif /* MapResourceController_h */
