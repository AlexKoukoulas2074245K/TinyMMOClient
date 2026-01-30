///------------------------------------------------------------------------------------------------
///  PlaceNavmapTileTypeCommand.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/01/2026.
///------------------------------------------------------------------------------------------------

#ifndef PlaceNavmapTileTypeCommand_h
#define PlaceNavmapTileTypeCommand_h

///------------------------------------------------------------------------------------------------

#include <editor/commands/IEditorCommand.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <net_common/Navmap.h>
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

class PlaceNavmapTileTypeCommand final: public IEditorCommand
{
public:
    PlaceNavmapTileTypeCommand(std::shared_ptr<scene::SceneObject> targetTileSceneObject, const network::NavmapTileType navmapTileType);
    
    void VExecute() override;
    void VUndo() override;
    bool VIsNoOp() const override;
    
private:
    std::shared_ptr<scene::SceneObject> mTargetTileSceneObject;
    const network::NavmapTileType mOldNavmapTileType;
    const network::NavmapTileType mNewNavmapTileType;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* PlaceNavmapTileTypeCommand_h */
