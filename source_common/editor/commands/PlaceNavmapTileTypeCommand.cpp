///------------------------------------------------------------------------------------------------
///  PlaceNavmapTileTypeCommand.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/10/2026.
///------------------------------------------------------------------------------------------------

#include <editor/commands/PlaceNavmapTileTypeCommand.h>
#include <engine/scene/SceneObject.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

PlaceNavmapTileTypeCommand::PlaceNavmapTileTypeCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, const network::NavmapTileType navmapTileType)
    : mTargetTileSceneObject(targetTileSceneObject)
    , mOldNavmapTileType(static_cast<network::NavmapTileType>(targetTileSceneObject->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)))
    , mNewNavmapTileType(navmapTileType)
{
    assert(targetTileSceneObject->mShaderBoolUniformValues.at(TILE_IS_NAVMAP_TILE_UNIFORM_NAME));
}

///------------------------------------------------------------------------------------------------

void PlaceNavmapTileTypeCommand::VExecute()
{
    mTargetTileSceneObject->mShaderIntUniformValues[TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME] = static_cast<int>(mNewNavmapTileType);
    editor_utils::SetNavmapTileUniforms(mTargetTileSceneObject);
}

///------------------------------------------------------------------------------------------------

void PlaceNavmapTileTypeCommand::VUndo()
{
    mTargetTileSceneObject->mShaderIntUniformValues[TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME] = static_cast<int>(mOldNavmapTileType);
    editor_utils::SetNavmapTileUniforms(mTargetTileSceneObject);
}

///------------------------------------------------------------------------------------------------

bool PlaceNavmapTileTypeCommand::VIsNoOp() const
{
    return mOldNavmapTileType == mNewNavmapTileType;
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

