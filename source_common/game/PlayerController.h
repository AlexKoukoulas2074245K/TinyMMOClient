///------------------------------------------------------------------------------------------------
///  PlayerController.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 09/05/2024
///------------------------------------------------------------------------------------------------

#ifndef PlayerController_h
#define PlayerController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
namespace scene { struct SceneObject; }
namespace networking { struct WorldObjectData; }
class PlayerController
{
public:
    void Update(const float dtMillis, const strutils::StringId& playerName, networking::WorldObjectData& objectData, scene::Scene& scene);
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerController_h */
