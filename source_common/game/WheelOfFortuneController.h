///------------------------------------------------------------------------------------------------
///  WheelOfFortuneController.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 08/01/2024                                                       
///------------------------------------------------------------------------------------------------

#ifndef WheelOfFortuneController_h
#define WheelOfFortuneController_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/scene/Scene.h>
#include <engine/utils/StringUtils.h>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { struct SceneObject; }

///------------------------------------------------------------------------------------------------

class WheelOfFortuneController final
{
public:
    WheelOfFortuneController(scene::Scene& scene, const std::vector<strutils::StringId>& productNames, std::function<void(const int, const std::shared_ptr<scene::SceneObject>)> onItemSelectedCallback);
    
    void Spin();
    void Update(const float dtMillis);
    std::vector<std::shared_ptr<scene::SceneObject>> GetSceneObjects() const;
    
private:
    void ApplyRotationToItems();

private:
    enum class WheelState
    {
        INITIAL_SLOW_ROTATION,
        SPINNING,
        ROTATING_TO_SELECTED_ITEM,
        FINISHED
    };
    
private:
    scene::Scene& mScene;
    const std::vector<strutils::StringId> mItems;
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
    std::function<void(const int, const std::shared_ptr<scene::SceneObject>)> mOnItemSelectedCallback;
    WheelState mState;
    float mWheelRotationSpeed;
    float mWheelRotation;
};

///------------------------------------------------------------------------------------------------


#endif /* WheelOfFortuneController_h */
