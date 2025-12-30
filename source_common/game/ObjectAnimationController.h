///------------------------------------------------------------------------------------------------
///  ObjectAnimationController.h
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/12/2025                                                       
///------------------------------------------------------------------------------------------------

#ifndef ObjectAnimationController_h
#define ObjectAnimationController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <game/events/Events.h>
#include <game/events/EventSystem.h>
#include <net_common/NetworkCommon.h>
#include <memory>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

namespace scene { class SceneObject; }
class ObjectAnimationController: public events::IListener
{
public:
    struct ObjectAnimationInfo
    {
        int mFrameIndex = 0;
        int mAnimationRow = 0;
        bool mFlippedAnimation = false;
        float mAnimationTimeAccum = 0.0f;
        network::FacingDirection mFacingDirection;
    };
    
public:
    ObjectAnimationController();
    
    void OnObjectDestroyedEvent(const events::ObjectDestroyedEvent& objectDestroyedEvent);

    const ObjectAnimationInfo& UpdateObjectAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const glm::vec3& velocity, const float dtMillis, std::optional<network::FacingDirection> facingDirection);
    
private:
    std::unordered_map<strutils::StringId, ObjectAnimationInfo, strutils::StringIdHasher> mObjectAnimationInfoMap;
};

///------------------------------------------------------------------------------------------------

#endif /* ObjectAnimationController_h */
