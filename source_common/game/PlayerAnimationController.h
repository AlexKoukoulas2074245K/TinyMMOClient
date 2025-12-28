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
#include <unordered_map>

///------------------------------------------------------------------------------------------------

namespace scene { class SceneObject; }
class PlayerAnimationController
{
public:
    struct PlayerAnimationInfo
    {
        int mFrameIndex = 0;
        int mAnimationIndex = 0;
        bool mFlippedAnimation = false;
        float mAnimationTimeAccum = 0.0f;
    };
    
public:
    PlayerAnimationController(){};
    
    void OnPlayerDisconnected(const strutils::StringId& playerNameId);
    const PlayerAnimationInfo& UpdatePlayerAnimation(std::shared_ptr<scene::SceneObject> player, const float playerSpeed, const glm::vec3& velocity, const float dtMillis, const int animationIndexOverride = -1);
    
private:
    std::unordered_map<strutils::StringId, PlayerAnimationInfo, strutils::StringIdHasher> mPlayerAnimationInfo;
};

///------------------------------------------------------------------------------------------------

#endif /* PlayerAnimationController_h */
