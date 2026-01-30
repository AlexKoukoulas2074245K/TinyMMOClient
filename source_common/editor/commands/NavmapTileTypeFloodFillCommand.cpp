///------------------------------------------------------------------------------------------------
///  NavmapTileTypeFloodFillCommand.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/01/2026.
///------------------------------------------------------------------------------------------------

#include <editor/commands/NavmapTileTypeFloodFillCommand.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

NavmapTileTypeFloodFillCommand::NavmapTileTypeFloodFillCommand(std::shared_ptr<scene::Scene> scene, std::shared_ptr<scene::SceneObject> targetTileSceneObject, const network::NavmapTileType navmapTileType)
    : mScene(scene)
    , mOldNavmapTileType(static_cast<network::NavmapTileType>(targetTileSceneObject->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)))
    , mNewNavmapTileType(navmapTileType)
{
    assert(targetTileSceneObject->mShaderBoolUniformValues.at(TILE_IS_NAVMAP_TILE_UNIFORM_NAME));

    if (mNewNavmapTileType != mOldNavmapTileType)
    {
        std::vector<std::shared_ptr<scene::SceneObject>> unprocessedTiles;
        unprocessedTiles.push_back(targetTileSceneObject);
        
        while (!unprocessedTiles.empty())
        {
            auto tile = unprocessedTiles.front();
            unprocessedTiles.erase(unprocessedTiles.begin());
            mAffectedTiles.push_back(tile);
            
            // Extract tile coords
            auto tileCoordsString = tile->mName.GetString();
            auto tileNamePostfix = "_navmap";
            tileCoordsString = tileCoordsString.substr(0, tileCoordsString.find(tileNamePostfix));

            auto tileCoordsStringSplit = strutils::StringSplit(tileCoordsString, ',');
            glm::ivec2 tileCoords = { std::stoi(tileCoordsStringSplit[0]), std::stoi(tileCoordsStringSplit[1]) };
            
            auto topTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x) + "," + std::to_string(tileCoords.y + 1) + tileNamePostfix));
            auto rightTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x + 1) + "," + std::to_string(tileCoords.y) + tileNamePostfix));
            auto bottomTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x) + "," + std::to_string(tileCoords.y - 1) + tileNamePostfix));
            auto leftTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x - 1) + "," + std::to_string(tileCoords.y) + tileNamePostfix));
            
            if (topTileNeighbor && static_cast<network::NavmapTileType>(topTileNeighbor->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)) == mOldNavmapTileType && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == topTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == topTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(topTileNeighbor);
            }
            
            if (rightTileNeighbor && static_cast<network::NavmapTileType>(rightTileNeighbor->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)) == mOldNavmapTileType && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == rightTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == rightTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(rightTileNeighbor);
            }
            
            if (bottomTileNeighbor && static_cast<network::NavmapTileType>(bottomTileNeighbor->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)) == mOldNavmapTileType && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == bottomTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == bottomTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(bottomTileNeighbor);
            }
            
            if (leftTileNeighbor && static_cast<network::NavmapTileType>(leftTileNeighbor->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)) == mOldNavmapTileType && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == leftTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == leftTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(leftTileNeighbor);
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void NavmapTileTypeFloodFillCommand::VExecute()
{
    for (auto tile: mAffectedTiles)
    {
        tile->mShaderIntUniformValues[TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME] = static_cast<int>(mNewNavmapTileType);
        editor_utils::SetNavmapTileUniforms(tile);
    }
}

///------------------------------------------------------------------------------------------------

void NavmapTileTypeFloodFillCommand::VUndo()
{
    for (auto tile: mAffectedTiles)
    {
        tile->mShaderIntUniformValues[TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME] = static_cast<int>(mOldNavmapTileType);
        editor_utils::SetNavmapTileUniforms(tile);
    }
}

///------------------------------------------------------------------------------------------------

bool NavmapTileTypeFloodFillCommand::VIsNoOp() const
{
    return mNewNavmapTileType == mOldNavmapTileType || mAffectedTiles.empty();
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

