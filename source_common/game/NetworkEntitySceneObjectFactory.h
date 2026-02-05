///------------------------------------------------------------------------------------------------
///  NetworkEntitySceneObjectFactory.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 05/02/2026
///------------------------------------------------------------------------------------------------

#ifndef NetworkEntitySceneObjectFactory_h
#define NetworkEntitySceneObjectFactory_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace network { struct ObjectData; }
namespace scene { struct SceneObject; }
class NetworkEntitySceneObjectFactory
{
public:
    static void CreateSceneObjects(const network::ObjectData& objectData, const bool collidersVisible, std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects);
    
private:
    NetworkEntitySceneObjectFactory(){};
};

///------------------------------------------------------------------------------------------------

#endif /* NetworkEntitySceneObjectFactory_h */
