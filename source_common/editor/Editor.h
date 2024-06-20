///------------------------------------------------------------------------------------------------
///  Editor.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 10/05/2024
///------------------------------------------------------------------------------------------------

#ifndef Editor_h
#define Editor_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/ImageSurfaceResource.h>
#include <engine/resloading/TextureResource.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
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
};

///------------------------------------------------------------------------------------------------

#endif /* Editor_h */
