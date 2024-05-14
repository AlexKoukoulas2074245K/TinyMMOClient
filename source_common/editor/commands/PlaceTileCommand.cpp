///------------------------------------------------------------------------------------------------
///  PlaceTileCommand.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <editor/commands/PlaceTileCommand.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

PlaceTileCommand::PlaceTileCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, resources::ResourceId newTileTextureresourceId)
    : mTargetTileSceneObject(targetTileSceneObject)
    , mNewTileTextureresourceId(newTileTextureresourceId)
    , mOldTileTextureresourceId(targetTileSceneObject->mTextureResourceId)
{
}

///------------------------------------------------------------------------------------------------

void PlaceTileCommand::VExecute()
{
    mTargetTileSceneObject->mTextureResourceId = mNewTileTextureresourceId;
}

///------------------------------------------------------------------------------------------------

void PlaceTileCommand::VUndo()
{
    mTargetTileSceneObject->mTextureResourceId = mOldTileTextureresourceId;
}

///------------------------------------------------------------------------------------------------

strutils::StringId PlaceTileCommand::VGetCommandStringId() const
{
    return strutils::StringId("PlaceTileCommand_" + mTargetTileSceneObject->mName.GetString() + "_" + std::to_string(mNewTileTextureresourceId));
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

