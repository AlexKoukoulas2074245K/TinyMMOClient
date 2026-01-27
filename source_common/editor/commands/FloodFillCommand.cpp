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

FloodFillCommand::FloodFillCommand(std::shared_ptr<scene::Scene> scene, std::shared_ptr<scene::SceneObject> targetTileSceneObject, const map_constants::LayerType layerType, const glm::ivec2& tilesetCoords, const resources::ResourceId textureResourceId, const float tileUVSize)
    : mScene(scene)
    , mLayerType(layerType)
    , mNewTilesetCoords(tilesetCoords)
    , mOldTilesetCoords(editor_utils::GetTilesetCoords(targetTileSceneObject, tileUVSize))
    , mOldTextureResourceId(targetTileSceneObject->mTextureResourceId)
    , mNewTextureResourceId(textureResourceId)
    , mTileUVSize(tileUVSize)
{
    if (mOldTilesetCoords != mNewTilesetCoords || mOldTextureResourceId != mNewTextureResourceId)
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
            auto tileNamePostfix = std::string();

            switch (mLayerType)
            {
                case map_constants::LayerType::TOP_LAYER:
                {
                    tileNamePostfix = "_top";
                    tileCoordsString = tileCoordsString.substr(0, tileCoordsString.find(tileNamePostfix));
                } break;
                    
                case map_constants::LayerType::NAVMAP:
                {
                    tileNamePostfix = "_navmap";
                    tileCoordsString = tileCoordsString.substr(0, tileCoordsString.find(tileNamePostfix));
                } break;
        
                default: break;
            }

            auto tileCoordsStringSplit = strutils::StringSplit(tileCoordsString, ',');
            glm::ivec2 tileCoords = { std::stoi(tileCoordsStringSplit[0]), std::stoi(tileCoordsStringSplit[1]) };
            
            auto topTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x) + "," + std::to_string(tileCoords.y + 1) + tileNamePostfix));
            auto rightTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x + 1) + "," + std::to_string(tileCoords.y) + tileNamePostfix));
            auto bottomTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x) + "," + std::to_string(tileCoords.y - 1) + tileNamePostfix));
            auto leftTileNeighbor = scene->FindSceneObject(strutils::StringId(std::to_string(tileCoords.x - 1) + "," + std::to_string(tileCoords.y) + tileNamePostfix));
            
            if (topTileNeighbor && topTileNeighbor->mTextureResourceId == mOldTextureResourceId && editor_utils::GetTilesetCoords(topTileNeighbor, tileUVSize) == mOldTilesetCoords && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == topTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == topTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(topTileNeighbor);
            }
            
            if (rightTileNeighbor && rightTileNeighbor->mTextureResourceId == mOldTextureResourceId && editor_utils::GetTilesetCoords(rightTileNeighbor, tileUVSize) == mOldTilesetCoords && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == rightTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == rightTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(rightTileNeighbor);
            }
            
            if (bottomTileNeighbor && bottomTileNeighbor->mTextureResourceId == mOldTextureResourceId && editor_utils::GetTilesetCoords(bottomTileNeighbor, tileUVSize) == mOldTilesetCoords && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == bottomTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == bottomTileNeighbor->mName; }) == mAffectedTiles.end())
            {
                unprocessedTiles.push_back(bottomTileNeighbor);
            }
            
            if (leftTileNeighbor && leftTileNeighbor->mTextureResourceId == mOldTextureResourceId && editor_utils::GetTilesetCoords(leftTileNeighbor, tileUVSize) == mOldTilesetCoords && std::find_if(unprocessedTiles.cbegin(), unprocessedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == leftTileNeighbor->mName; }) == unprocessedTiles.end() && std::find_if(mAffectedTiles.cbegin(), mAffectedTiles.cend(), [&](std::shared_ptr<scene::SceneObject> otherTile){ return otherTile->mName == leftTileNeighbor->mName; }) == mAffectedTiles.end())
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
        tile->mTextureResourceId = mNewTextureResourceId;
        editor_utils::SetTilesetUVs(tile, mNewTilesetCoords, mTileUVSize);
    }
}

///------------------------------------------------------------------------------------------------

void FloodFillCommand::VUndo()
{
    for (auto tile: mAffectedTiles)
    {
        tile->mTextureResourceId = mOldTextureResourceId;
        editor_utils::SetTilesetUVs(tile, mOldTilesetCoords, mTileUVSize);
    }
}

///------------------------------------------------------------------------------------------------

bool FloodFillCommand::VIsNoOp() const
{
    return (mOldTilesetCoords == mNewTilesetCoords && mOldTextureResourceId == mNewTextureResourceId) || mAffectedTiles.empty();
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

