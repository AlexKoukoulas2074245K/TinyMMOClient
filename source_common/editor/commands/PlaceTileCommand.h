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
    PlaceTileCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, resources::ResourceId newTileTextureResourceId);
    
    void VExecute() override;
    void VUndo() override;
    bool VIsNoOp() const override;
    
private:
    std::shared_ptr<scene::SceneObject> mTargetTileSceneObject;
    const resources::ResourceId mNewTileTextureResourceId;
    const resources::ResourceId mOldTileTextureResourceId;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* PlaceTileCommand_h */
