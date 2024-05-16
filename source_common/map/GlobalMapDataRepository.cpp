///------------------------------------------------------------------------------------------------
///  GlobalMapDataRepository.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 16/05/2024
///------------------------------------------------------------------------------------------------

#include <map/GlobalMapDataRepository.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

static const std::string GLOBAL_MAP_DATA_FILE_PATH = "world/map_global_data.json";
static const std::string MAP_TRANSFORMS_JSON = "map_transforms";
static const std::string MAP_CONNECTIONS_JSON = "map_connections";

///------------------------------------------------------------------------------------------------

GlobalMapDataRepository& GlobalMapDataRepository::GetInstance()
{
    static GlobalMapDataRepository instance;
    return instance;
}

///------------------------------------------------------------------------------------------------

GlobalMapDataRepository::GlobalMapDataRepository()
{
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, MapDefinition, strutils::StringIdHasher> GlobalMapDataRepository::GetMapDefinitions() const
{
    return mMapDefinitions;
}

///------------------------------------------------------------------------------------------------

const MapDefinition& GlobalMapDataRepository::GetMapDefinition(const strutils::StringId& mapDefinition) const
{
    auto foundIter = mMapDefinitions.find(mapDefinition);
    if (foundIter != mMapDefinitions.cend())
    {
        return foundIter->second;
    }
    
    assert(false);
    
    static MapDefinition emptyMapDefinition(strutils::StringId(), {}, {}, {});
    return emptyMapDefinition;
}

///------------------------------------------------------------------------------------------------

const strutils::StringId& GlobalMapDataRepository::GetConnectedMapName(const strutils::StringId& mapName, const MapConnectionDirection direction) const
{
    return mMapDefinitions.at(mapName).mMapConnections.at(static_cast<int>(direction));
}

///------------------------------------------------------------------------------------------------

void GlobalMapDataRepository::LoadMapDefinitions()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto globalMapDataResourceId = systemsEngine.GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + GLOBAL_MAP_DATA_FILE_PATH);
    auto& globalMapDataFileResource = systemsEngine.GetInstance().GetResourceLoadingService().GetResource<resources::DataFileResource>(globalMapDataResourceId);
    auto globalMapDataJson = nlohmann::json::parse(globalMapDataFileResource.GetContents());
    
    mMapDefinitions.clear();
    for (auto mapTransformIter = globalMapDataJson[MAP_TRANSFORMS_JSON].begin(); mapTransformIter != globalMapDataJson[MAP_TRANSFORMS_JSON].end(); ++mapTransformIter)
    {
        auto mapFileName = mapTransformIter.key();
        auto mapName = mapTransformIter.key().substr(0, mapFileName.find(".json"));
        auto mapNameId = strutils::StringId(mapName);
        auto mapPosition = glm::vec2(mapTransformIter.value()["x"].get<float>(), mapTransformIter.value()["y"].get<float>());
        auto mapDimensions = glm::vec2(mapTransformIter.value()["width"].get<float>(), mapTransformIter.value()["height"].get<float>());
        
        auto northConnectionMapName = globalMapDataJson[MAP_CONNECTIONS_JSON][mapFileName]["top"].get<std::string>();
        auto eastConnectionMapName = globalMapDataJson[MAP_CONNECTIONS_JSON][mapFileName]["right"].get<std::string>();
        auto southConnectionMapName = globalMapDataJson[MAP_CONNECTIONS_JSON][mapFileName]["bottom"].get<std::string>();
        auto westConnectionMapName = globalMapDataJson[MAP_CONNECTIONS_JSON][mapFileName]["left"].get<std::string>();
        
        MapConnectionsType mapConnections;
        mapConnections[static_cast<int>(MapConnectionDirection::NORTH)] = strutils::StringId(northConnectionMapName.substr(0, northConnectionMapName.find(".json")));
        mapConnections[static_cast<int>(MapConnectionDirection::EAST)]  = strutils::StringId(eastConnectionMapName.substr(0, eastConnectionMapName.find(".json")));
        mapConnections[static_cast<int>(MapConnectionDirection::SOUTH)] = strutils::StringId(southConnectionMapName.substr(0, southConnectionMapName.find(".json")));
        mapConnections[static_cast<int>(MapConnectionDirection::WEST)]  = strutils::StringId(westConnectionMapName.substr(0, westConnectionMapName.find(".json")));
        
        mMapDefinitions.emplace(std::make_pair(mapNameId, MapDefinition(strutils::StringId(mapNameId), mapConnections, mapDimensions, mapPosition)));
    }
}

///------------------------------------------------------------------------------------------------
