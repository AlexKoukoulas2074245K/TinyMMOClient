///------------------------------------------------------------------------------------------------
///  PlaceTileCommand.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/09/2023.
///------------------------------------------------------------------------------------------------

#include <editor/commands/PlaceTileCommand.h>
#include <engine/scene/SceneObject.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

PlaceTileCommand::PlaceTileCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, const glm::ivec2& tilesetCoords, const resources::ResourceId textureResourceId, const float tileUVSize)
    : mTargetTileSceneObject(targetTileSceneObject)
    , mNewTilesetCoords(tilesetCoords)
    , mOldTilesetCoords(editor_utils::GetTilesetCoords(targetTileSceneObject, tileUVSize))
    , mNewTextureResourceId(textureResourceId)
    , mOldTextureResourceId(targetTileSceneObject->mTextureResourceId)
    , mTileUVSize(tileUVSize)
{
}

///------------------------------------------------------------------------------------------------

void PlaceTileCommand::VExecute()
{
    mTargetTileSceneObject->mTextureResourceId = mNewTextureResourceId;
    editor_utils::SetNormalTileUniforms(mTargetTileSceneObject, mNewTilesetCoords, mTileUVSize);
}

///------------------------------------------------------------------------------------------------

void PlaceTileCommand::VUndo()
{
    mTargetTileSceneObject->mTextureResourceId = mOldTextureResourceId;
    editor_utils::SetNormalTileUniforms(mTargetTileSceneObject, mOldTilesetCoords, mTileUVSize);
}

///------------------------------------------------------------------------------------------------

bool PlaceTileCommand::VIsNoOp() const
{
    return mOldTilesetCoords == mNewTilesetCoords && mNewTextureResourceId == mOldTextureResourceId;
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

