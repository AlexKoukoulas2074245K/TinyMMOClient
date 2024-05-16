///------------------------------------------------------------------------------------------------
///  GlobalMapDataRepository.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 16/05/2024
///------------------------------------------------------------------------------------------------

#ifndef GlobalMapDataRepository_h
#define GlobalMapDataRepository_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <unordered_map>
#include <variant>

///------------------------------------------------------------------------------------------------

enum class MapConnectionDirection
{
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3,
    MAX = 4
};

///------------------------------------------------------------------------------------------------

using MapConnectionsType = std::array<strutils::StringId, static_cast<size_t>(MapConnectionDirection::MAX)>;

///------------------------------------------------------------------------------------------------

struct MapDefinition
{
    MapDefinition(const strutils::StringId& mapName, const MapConnectionsType& mapConnections, const glm::vec2& mapDimensions, const glm::vec2& mapPosition)
        : mMapName(mapName)
        , mMapConnections(mapConnections)
        , mMapDimensions(mapDimensions)
        , mMapPosition(mapPosition)
    {
    }
    
    const strutils::StringId mMapName;
    const MapConnectionsType mMapConnections;
    const glm::vec2 mMapDimensions;
    const glm::vec2 mMapPosition;
};

///------------------------------------------------------------------------------------------------

class GlobalMapDataRepository final
{
public:
    static GlobalMapDataRepository& GetInstance();
    ~GlobalMapDataRepository() = default;
    
    GlobalMapDataRepository(const GlobalMapDataRepository&) = delete;
    GlobalMapDataRepository(GlobalMapDataRepository&&) = delete;
    const GlobalMapDataRepository& operator = (const GlobalMapDataRepository&) = delete;
    GlobalMapDataRepository& operator = (GlobalMapDataRepository&&) = delete;
    
    const std::unordered_map<strutils::StringId, MapDefinition, strutils::StringIdHasher> GetMapDefinitions() const;
    const MapDefinition& GetMapDefinition(const strutils::StringId& mapName) const;
    const strutils::StringId& GetConnectedMapName(const strutils::StringId& mapName, const MapConnectionDirection direction) const;
    void LoadMapDefinitions();
    
private:
    GlobalMapDataRepository();
    
private:
    std::unordered_map<strutils::StringId, MapDefinition, strutils::StringIdHasher> mMapDefinitions;
};

///------------------------------------------------------------------------------------------------

#endif /* GlobalMapDataRepository_h */
