///------------------------------------------------------------------------------------------------
///  NavmapTileTypeFloodFillCommand.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/01/2026.
///------------------------------------------------------------------------------------------------

#ifndef NavmapTileTypeFloodFillCommand_h
#define NavmapTileTypeFloodFillCommand_h

///------------------------------------------------------------------------------------------------

#include <editor/commands/IEditorCommand.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/StringUtils.h>
#include <map/MapConstants.h>
#include <net_common/Navmap.h>
#include <memory>
#include <unordered_set>

///------------------------------------------------------------------------------------------------

namespace scene
{
    class Scene;
    struct SceneObject;
}

///------------------------------------------------------------------------------------------------

namespace commands
{

///------------------------------------------------------------------------------------------------

class NavmapTileTypeFloodFillCommand final: public IEditorCommand
{
public:
    NavmapTileTypeFloodFillCommand(std::shared_ptr<scene::Scene> scene, std::shared_ptr<scene::SceneObject> targetTileSceneObject, const networking::NavmapTileType navmapTileType);
    
    void VExecute() override;
    void VUndo() override;
    bool VIsNoOp() const override;
    
private:
    std::shared_ptr<scene::Scene> mScene;
    const networking::NavmapTileType mOldNavmapTileType;
    const networking::NavmapTileType mNewNavmapTileType;
    std::vector<std::shared_ptr<scene::SceneObject>> mAffectedTiles;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* NavmapTileTypeFloodFillCommand_h */
