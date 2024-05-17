///------------------------------------------------------------------------------------------------
///  PlayerController.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 09/05/2024
///------------------------------------------------------------------------------------------------

#ifndef PlayerController_h
#define PlayerController_h

///------------------------------------------------------------------------------------------------

#include <game/events/EventSystem.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <engine/resloading/ResourceLoadingService.h>

///------------------------------------------------------------------------------------------------

struct SDL_Surface;
namespace scene { class Scene; }
namespace scene { struct SceneObject; }
namespace networking { struct WorldObjectData; }

///------------------------------------------------------------------------------------------------

class PlayerController
{
public:
    PlayerController(const strutils::StringId& mapName);
    
    const strutils::StringId& GetCurrentMapName() const;
    
    void Update(const float dtMillis, const strutils::StringId& playerName, networking::WorldObjectData& objectData, scene::Scene& scene);
    void CreateDebugWidgets();
    void ShowNavmapDebugView();
    void HideNavmapDebugView();
    void SetNavmapResourceId(const resources::ResourceId navmapResourceId);

private:
    enum class MovementState
    {
        IDLE, MOVING, BUMPING
    };
    
    strutils::StringId mCurrentMapName;
    resources::ResourceId mNavmapResourceId;
    MovementState mMovementState;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerController_h */
