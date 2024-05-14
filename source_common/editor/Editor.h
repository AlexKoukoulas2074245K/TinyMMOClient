///------------------------------------------------------------------------------------------------
///  Editor.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 10/05/2024
///------------------------------------------------------------------------------------------------

#ifndef Editor_h
#define Editor_h

///------------------------------------------------------------------------------------------------

#include <editor/commands/IEditorCommand.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/resloading/TextureResource.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <map/MapConstants.h>
#include <vector>
#include <stack>

///------------------------------------------------------------------------------------------------

namespace scene
{
    struct SceneObject;
    class Scene;
}

class Editor final
{
public:
    Editor(const int argc, char** argv);
    ~Editor();
    
    void Init();
    void Update(const float dtMillis);
    void ApplicationMovedToBackground();
    void WindowResize();
    void OnOneSecondElapsed();
    void CreateDebugWidgets();
    
private:
    void CreateMap(const int gridRows, const int gridCols);
    void UpdateTile(std::shared_ptr<scene::SceneObject> tile, std::shared_ptr<scene::Scene> scene, const int tileCol, const int tileRow);
    void TryExecuteCommand(std::unique_ptr<commands::IEditorCommand> command);
    void TryUndoLastCommand();
    
private:
    struct MapTileData
    {
        resources::ResourceId mResourceId;
        resources::GLuint mTextureId;
        std::string mTileName;
    };
    
    struct ViewOptions
    {
        bool mRenderConnectorTiles = true;
        float mCameraZoom = 0.0f;
        glm::vec3 mCameraPosition;
    };
    
    int mGridRows, mGridCols;
    int mSelectedPaletteTile;
    bool mBottomLayerVisible;
    bool mTopLayerVisible;
    std::vector<MapTileData> mPaletteTileData;
    std::stack<std::unique_ptr<commands::IEditorCommand>> mExecutedCommandHistory;
    ViewOptions mViewOptions;
    map_constants::LayerType mActiveLayer;
};

///------------------------------------------------------------------------------------------------

#endif /* Editor_h */
