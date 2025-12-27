///------------------------------------------------------------------------------------------------
///  PlayerAnimationController.h
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/12/2025                                                       
///------------------------------------------------------------------------------------------------

#ifndef PlayerAnimationController_h
#define PlayerAnimationController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <memory>

///------------------------------------------------------------------------------------------------

namespace scene { class SceneObject; }
class PlayerAnimationController
{
public:
    static void UpdatePlayerAnimation(std::shared_ptr<scene::SceneObject> player, const glm::vec3& velocity, const float dtMillis);
    
private:
    PlayerAnimationController(){};
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerAnimationController_h */
