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
    void TerrainCollisionHandlingPostMapChange(networking::WorldObjectData& objectData, std::shared_ptr<scene::SceneObject> playerSceneObject, std::shared_ptr<scene::SceneObject> playerNameSceneObject, glm::vec3 impulseVector, const float dtMillis);
    void TerrainCollisionHandling(networking::WorldObjectData& objectData, std::shared_ptr<scene::SceneObject> playerSceneObject, std::shared_ptr<scene::SceneObject> playerNameSceneObject, glm::vec3 impulseVector, const float dtMillis);
private:
    strutils::StringId mCurrentMapName;
    resources::ResourceId mNavmapResourceId;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerController_h */
