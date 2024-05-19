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
#include <net_common/Navmap.h>

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
    void SetNavmap(const resources::ResourceId navmapImageResourceId, std::shared_ptr<networking::Navmap> navmap);
    void TerrainCollisionHandlingPostMapChange(networking::WorldObjectData& objectData, std::shared_ptr<scene::SceneObject> playerSceneObject, std::shared_ptr<scene::SceneObject> playerNameSceneObject, glm::vec3 impulseVector, const float dtMillis);
    void TerrainCollisionHandling(networking::WorldObjectData& objectData, std::shared_ptr<scene::SceneObject> playerSceneObject, std::shared_ptr<scene::SceneObject> playerNameSceneObject, glm::vec3 impulseVector, const float dtMillis);
private:
    strutils::StringId mCurrentMapName;
    std::shared_ptr<networking::Navmap> mCurrentNavmap;
    resources::ResourceId mCurrentNavmapResourceId;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerController_h */
