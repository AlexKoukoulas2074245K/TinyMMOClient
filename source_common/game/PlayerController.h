///------------------------------------------------------------------------------------------------
///  PlayerController.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 09/05/2024
///------------------------------------------------------------------------------------------------

#ifndef PlayerController_h
#define PlayerController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <engine/resloading/ResourceLoadingService.h>

///------------------------------------------------------------------------------------------------

struct SDL_Surface;
namespace scene { class Scene; }
namespace scene { struct SceneObject; }
namespace networking { struct WorldObjectData; }
class PlayerController
{
public:
    PlayerController(const strutils::StringId& mapName);
    void Update(const float dtMillis, const strutils::StringId& playerName, networking::WorldObjectData& objectData, scene::Scene& scene);
    void CreateDebugWidgets();
    void ShowNavmapDebugView();
    void HideNavmapDebugView();
    
private:
    strutils::StringId mCurrentMapName;
    resources::ResourceId mNavmapResourceId;
    glm::ivec2 mPreviousNavmapCoords;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerController_h */
