///------------------------------------------------------------------------------------------------
///  FloodFillCommand.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <editor/commands/FloodFillCommand.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

FloodFillCommand::FloodFillCommand(std::shared_ptr<scene::Scene> scene, std::shared_ptr<scene::SceneObject> targetTileSceneObject, resources::ResourceId newTileTextureResourceId)
    : mScene(scene)
    , mNewTileTextureResourceId(newTileTextureResourceId)
    , mOldTileTextureResourceId(targetTileSceneObject->mTextureResourceId)
    , mTileNamePostfix(strutils::StringEndsWith(targetTileSceneObject->mName.GetString(), "_top") ? "_top" : "")
{
    if (mOldTileTextureResourceId != mNewTileTextureResourceId)
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
            tileCoordsString = !mTileNamePostfix.empty() ? tileCoordsString.substr(0, tileCoordsString.find("_top")) : tileCoordsString;
            auto tileCoordsStringSplit = strutils::StringSplit(tileCoordsString, ',');
            glm::ivec2 tileCoords = { std::stoi(tileCoordsStringSplit[0]), std::stoi(tileCoordsStringSplit[1]) };
            
            auto topTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x) + "," + std::to_string(tileCoords.y + 1) + mTileNamePostfix));
            auto rightTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x + 1) + "," + std::to_string(tileCoords.y) + mTileNamePostfix));
            auto bottomTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x) + "," + std::to_string(tileCoords.y - 1) + mTileNamePostfix));
            auto leftTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x - 1) + "," + std::to_string(tileCoords.y) + mTileNamePostfix));
            
            if (topTileNeighbor && topTileNeighbor->mTextureResourceId == mOldTileTextureResourceId && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == topTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == topTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(topTileNeighbor);
            }
            
            if (rightTileNeighbor && rightTileNeighbor->mTextureResourceId == mOldTileTextureResourceId && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == rightTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == rightTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(rightTileNeighbor);
            }
            
            if (bottomTileNeighbor && bottomTileNeighbor->mTextureResourceId == mOldTileTextureResourceId && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == bottomTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == bottomTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(bottomTileNeighbor);
            }
            
            if (leftTileNeighbor && leftTileNeighbor->mTextureResourceId == mOldTileTextureResourceId && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == leftTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == leftTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(leftTileNeighbor);
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void FloodFillCommand::VExecute()
{
    for (auto tile: mAffectedTiles)
    {
        tile->mTextureResourceId = mNewTileTextureResourceId;
    }
}

///------------------------------------------------------------------------------------------------

void FloodFillCommand::VUndo()
{
    for (auto tile: mAffectedTiles)
    {
        tile->mTextureResourceId = mOldTileTextureResourceId;
    }
}

///------------------------------------------------------------------------------------------------

bool FloodFillCommand::VIsNoOp() const
{
    return mOldTileTextureResourceId == mNewTileTextureResourceId || mAffectedTiles.empty();
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

