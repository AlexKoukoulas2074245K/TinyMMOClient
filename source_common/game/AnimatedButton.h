///------------------------------------------------------------------------------------------------
///  AnimatedButton.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/12/2023
///------------------------------------------------------------------------------------------------

#ifndef AnimatedButton_h
#define AnimatedButton_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/scene/Scene.h>
#include <engine/utils/StringUtils.h>
#include <functional>
#include <memory>

///------------------------------------------------------------------------------------------------

namespace scene { struct SceneObject; }

///------------------------------------------------------------------------------------------------

enum class ButtonUpdateInteractionResult
{
    CLICKED,
    NOT_CLICKED
};

///------------------------------------------------------------------------------------------------

class AnimatedButton final
{
public:
    AnimatedButton
    (
        const glm::vec3& position,
        const glm::vec3& scale,
        const std::string& textureFilename,
        const strutils::StringId& buttonName,
        std::function<void()> onPressCallback,
        scene::Scene& scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float snapToEdgeScaleOffsetFactor = 1.0f
    );
    AnimatedButton
    (
        const glm::vec3& position,
        const glm::vec3& scale,
        const strutils::StringId& fontName,
        const std::string& text,
        const strutils::StringId& buttonName,
        std::function<void()> onPressCallback,
        scene::Scene& scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float snapToEdgeScaleOffsetFactor = 1.0f
    );
    ~AnimatedButton();
    
    ButtonUpdateInteractionResult Update(const float dtMillis);
    std::shared_ptr<scene::SceneObject> GetSceneObject();
    
private:
    scene::Scene& mScene;
    std::shared_ptr<scene::SceneObject> mSceneObject;
    std::function<void()> mOnPressCallback;
    bool mAnimating;
};

///------------------------------------------------------------------------------------------------

#endif /* AnimatedButton_h */
