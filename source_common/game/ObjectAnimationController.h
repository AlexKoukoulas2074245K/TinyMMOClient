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
        bool mAnimationFinished = false;
        float mAnimationTimeAccum = 0.0f;
        network::FacingDirection mFacingDirection;
        network::ObjectState mObjectState;
    };
    
public:
    ObjectAnimationController();
    
    void OnObjectDestroyedEvent(const events::ObjectDestroyedEvent& objectDestroyedEvent);

    const ObjectAnimationInfo& UpdateObjectAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const network::ObjectType objectType, const network::ObjectState objectState, const network::FacingDirection facingDirection, const glm::vec3& velocity, const float dtMillis);
    
private:
    void UpdateCharacterAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const network::ObjectState objectState, const network::FacingDirection facingDirection, const glm::vec3& velocity, const float dtMillis);
    void UpdateAttackAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const network::FacingDirection facingDirection, const float dtMillis);
    
private:
    std::unordered_map<strutils::StringId, ObjectAnimationInfo, strutils::StringIdHasher> mObjectAnimationInfoMap;
};

///------------------------------------------------------------------------------------------------

#endif /* ObjectAnimationController_h */
