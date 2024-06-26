///------------------------------------------------------------------------------------------------
///  SceneObjectUtils.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 11/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef SceneObjectUtils_h
#define SceneObjectUtils_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>

///------------------------------------------------------------------------------------------------

namespace scene { struct SceneObject; }


///------------------------------------------------------------------------------------------------

namespace scene_object_utils
{

///------------------------------------------------------------------------------------------------

math::Rectangle GetSceneObjectBoundingRect(const scene::SceneObject& sceneObject);

///------------------------------------------------------------------------------------------------

}
///------------------------------------------------------------------------------------------------

#endif /* SceneObjectUtils_h */
