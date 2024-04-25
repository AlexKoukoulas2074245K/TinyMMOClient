///------------------------------------------------------------------------------------------------
///  IRenderer.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 25/09/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef IRenderer_h
#define IRenderer_h

///------------------------------------------------------------------------------------------------

#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
namespace scene { struct SceneObject; }
namespace rendering { class Camera; }

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

class IRenderer
{
public:
    virtual ~IRenderer() = default;
    virtual void VBeginRenderPass() = 0;
    virtual void VRenderScene(scene::Scene& scene) = 0;
    virtual void VRenderSceneObjectsToTexture(const std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects, const rendering::Camera& camera) = 0;
    virtual void VEndRenderPass() = 0;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* IRenderer_h */
