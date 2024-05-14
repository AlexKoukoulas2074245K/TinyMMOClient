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

PlaceTileCommand::PlaceTileCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, resources::ResourceId newTileTextureResourceId)
    : mTargetTileSceneObject(targetTileSceneObject)
    , mNewTileTextureResourceId(newTileTextureResourceId)
    , mOldTileTextureResourceId(targetTileSceneObject->mTextureResourceId)
{
}

///------------------------------------------------------------------------------------------------

void PlaceTileCommand::VExecute()
{
    mTargetTileSceneObject->mTextureResourceId = mNewTileTextureResourceId;
}

///------------------------------------------------------------------------------------------------

void PlaceTileCommand::VUndo()
{
    mTargetTileSceneObject->mTextureResourceId = mOldTileTextureResourceId;
}

///------------------------------------------------------------------------------------------------

bool PlaceTileCommand::VIsNoOp() const
{
    return mOldTileTextureResourceId == mNewTileTextureResourceId;
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

