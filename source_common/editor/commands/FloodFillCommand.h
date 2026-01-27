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
    FloodFillCommand(std::shared_ptr<scene::Scene> scene, std::shared_ptr<scene::SceneObject> targetTileSceneObject, const map_constants::LayerType layerType, const glm::ivec2& tilesetCoords, const resources::ResourceId textureResourceId, const float tileUVSize);
    
    void VExecute() override;
    void VUndo() override;
    bool VIsNoOp() const override;
    
private:
    std::shared_ptr<scene::Scene> mScene;
    const map_constants::LayerType mLayerType;
    const glm::ivec2 mNewTilesetCoords;
    const glm::ivec2 mOldTilesetCoords;
    const resources::ResourceId mOldTextureResourceId;
    const resources::ResourceId mNewTextureResourceId;
    const float mTileUVSize;
    std::vector<std::shared_ptr<scene::SceneObject>> mAffectedTiles;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* FloodFillCommand_h */
