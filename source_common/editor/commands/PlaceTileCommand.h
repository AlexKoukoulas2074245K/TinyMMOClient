///------------------------------------------------------------------------------------------------
///  PlaceTileCommand.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 14/05/2024.
///------------------------------------------------------------------------------------------------

#ifndef PlaceTileCommand_h
#define PlaceTileCommand_h

///------------------------------------------------------------------------------------------------

#include <editor/commands/IEditorCommand.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <map/MapConstants.h>
#include <memory>

///------------------------------------------------------------------------------------------------

namespace scene
{
    struct SceneObject;
}

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

class PlaceTileCommand final: public IEditorCommand
{
public:
    PlaceTileCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, const glm::ivec2& tilesetCoords, const resources::ResourceId textureResourceId, const float tileUVSize);
    
    void VExecute() override;
    void VUndo() override;
    bool VIsNoOp() const override;
    
private:
    std::shared_ptr<scene::SceneObject> mTargetTileSceneObject;
    const glm::ivec2 mNewTilesetCoords;
    const glm::ivec2 mOldTilesetCoords;
    const resources::ResourceId mOldTextureResourceId;
    const resources::ResourceId mNewTextureResourceId;
    const float mTileUVSize;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* PlaceTileCommand_h */
