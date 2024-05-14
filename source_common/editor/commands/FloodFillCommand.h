///------------------------------------------------------------------------------------------------
///  FloodFillCommand.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 14/05/2024.
///------------------------------------------------------------------------------------------------

#ifndef FloodFillCommand_h
#define FloodFillCommand_h

///------------------------------------------------------------------------------------------------

#include <editor/commands/IEditorCommand.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/StringUtils.h>
#include <map/MapConstants.h>
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

class FloodFillCommand final: public IEditorCommand
{
public:
    FloodFillCommand(std::shared_ptr<scene::Scene> scene, std::shared_ptr<scene::SceneObject> targetTileSceneObject, resources::ResourceId newTileTextureResourceId);
    
    void VExecute() override;
    void VUndo() override;
    bool VIsNoOp() const override;
    
private:
    std::shared_ptr<scene::Scene> mScene;
    const resources::ResourceId mNewTileTextureResourceId;
    const resources::ResourceId mOldTileTextureResourceId;
    const std::string mTileNamePostfix;
    std::vector<std::shared_ptr<scene::SceneObject>> mAffectedTiles;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* FloodFillCommand_h */
