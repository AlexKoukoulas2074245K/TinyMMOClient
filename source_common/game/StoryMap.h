///------------------------------------------------------------------------------------------------
///  StoryMap.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 19/12/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef StoryMap_h
#define StoryMap_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <map>
#include <memory>
#include <unordered_set>
#include <thread>

///------------------------------------------------------------------------------------------------

struct MapCoord
{
    MapCoord(const int col, const int row)
        : mCol(col)
        , mRow(row)
    {
    }
    
    std::string ToString() const { return std::to_string(mCol) + "_" + std::to_string(mRow); }
    
    int mCol;
    int mRow;
};

inline bool operator < (const MapCoord& lhs, const MapCoord& rhs)
{
    if (lhs.mCol == rhs.mCol)
    {
        return lhs.mRow < rhs.mRow;
    }
    else
    {
        return lhs.mCol < rhs.mCol;
    }
}

inline bool operator == (const MapCoord& lhs, const MapCoord& rhs)
{
    return lhs.mCol == rhs.mCol && lhs.mRow == rhs.mRow;
}

inline bool operator != (const MapCoord& lhs, const MapCoord& rhs)
{
    return !(lhs == rhs);
}

struct MapCoordHasher
{
    std::size_t operator()(const MapCoord& key) const
    {
        return strutils::StringId(key.ToString()).GetStringId();
    }
};

struct MapGenerationInfo
{
    int mMapGenerationAttempts = 0;
    int mCloseToStartingNodeErrors = 0;
    int mCloseToBossNodeErrors = 0;
    int mCloseToNorthEdgeErrors = 0;
    int mCloseToSouthEdgeErrors = 0;
    int mCloseToOtherNodesErrors = 0;
};

///------------------------------------------------------------------------------------------------

enum class StoryMapType
{
    TUTORIAL_MAP,
    NORMAL_MAP
};

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
class StoryMap final
{
public:
    enum class NodeType
    {
        NORMAL_ENCOUNTER = 0,
        ELITE_ENCOUNTER = 1,
        EVENT = 2,
        BOSS_ENCOUNTER = 3,
        SHOP = 4,
        STARTING_LOCATION = 5,
        COUNT = 6
    };
    
    struct NodeData
    {
        NodeType mNodeType;
        glm::vec3 mPosition;
        glm::ivec2 mCoords;
        std::unordered_set<MapCoord, MapCoordHasher> mNodeLinks;
        int mNodeRandomSeed;
    };
    
public:
    StoryMap(std::shared_ptr<scene::Scene> scene, const glm::ivec2& mapDimensions, const MapCoord& currentMapCoord);
    
    void GenerateMapNodes();
    void CreateMapSceneObjects();
    void DestroyParticleEmitters();
    bool HasCreatedSceneObjects() const;
    const std::map<MapCoord, StoryMap::NodeData>& GetMapData() const;
    const glm::ivec2& GetMapDimensions() const;
    const MapGenerationInfo& GetMapGenerationInfo() const;
    
private:
    void GenerateMapData();
    bool FoundCloseEnoughNodes() const; 
    bool DetectedCrossedEdge(const MapCoord& mapCoord, const MapCoord& targetTestCoord) const;
    glm::vec3 GenerateNodePositionForCoord(const MapCoord& mapCoord) const;
    NodeType SelectNodeTypeForCoord(const MapCoord& mapCoord) const;
    MapCoord RandomlySelectNextMapCoord(const MapCoord& mapCoord) const;
    void DepthFirstSearchOnCurrentCoords(const MapCoord& currentCoord, std::unordered_set<MapCoord, MapCoordHasher>& resultCoordsThatCanBeReached) const;
    
private:
    std::shared_ptr<scene::Scene> mScene;
    const glm::ivec2 mMapDimensions;
    const MapCoord mCurrentMapCoord;
    StoryMapType mCurrentStoryMapType;
    int mMapGenerationAttemptsRemaining;
    bool mHasCreatedSceneObjects;
    std::map<MapCoord, NodeData> mMapData;
    mutable MapGenerationInfo mMapGenerationInfo;
};

///------------------------------------------------------------------------------------------------

#endif /* StoryMap_h */

