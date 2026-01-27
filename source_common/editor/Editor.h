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
#include <net_common/Navmap.h>
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
    void DestroyMap();
    void CreateMap(const int gridRows, const int gridCols);
    void UpdateTile(std::shared_ptr<scene::SceneObject> tile, const map_constants::LayerType layer, const int tileCol, const int tileRow);
    void TryExecuteCommand(std::unique_ptr<commands::IEditorCommand> command);
    void TryUndoLastCommand();
    
public:
    struct MapTileData
    {
        std::string mTilesetName;
        glm::ivec2 mTileCoords; // row,col
        resources::ResourceId mTextureResourceId;
        resources::GLuint mTextureId;
    };
    
private:
    struct ViewOptions
    {
        float mCameraZoom = 0.0f;
        glm::vec3 mCameraPosition;
    };
    
    enum class PaintingToolType
    {
        PENCIL = 0,
        BUCKET = 1
    };
    
private:
    int mGridRows;
    int mGridCols;
    int mSelectedPaletteIndex;
    int mSelectedPaletteTile;
    int mTopImageRefIndex;
    int mRightImageRefIndex;
    int mBottomImageRefIndex;
    int mLeftImageRefIndex;
    int mActivePanel;
    networking::NavmapTileType mSelectedNavmapTileType;
    std::vector<std::vector<MapTileData>> mPaletteTileData;
    std::stack<std::unique_ptr<commands::IEditorCommand>> mExecutedCommandHistory;
    ViewOptions mViewOptions;
    PaintingToolType mPaintingToolType;
    std::array<float, static_cast<size_t>(map_constants::LayerType::LAYER_COUNT)> mLayersVisibility;
    map_constants::LayerType mActiveLayer;
};

///------------------------------------------------------------------------------------------------

#endif /* Editor_h */
