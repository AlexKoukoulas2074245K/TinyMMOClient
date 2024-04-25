///------------------------------------------------------------------------------------------------
///  RenderingUtils.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 31/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef RenderingUtils_h
#define RenderingUtils_h

///------------------------------------------------------------------------------------------------

#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/MathUtils.h>
#include <memory>
#include <vector>

namespace scene { struct SceneObject; }
namespace scene { class Scene; }

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

void CollateSceneObjectsIntoOne(const std::string& dynamicTextureResourceName, const glm::vec3& positionOffset, std::vector<std::shared_ptr<scene::SceneObject>>& sceneObjects, const std::string& exportFilePath, scene::Scene& scene);

int GetDisplayRefreshRate();

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* RenderingUtils_h */
