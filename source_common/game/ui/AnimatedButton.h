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
#include <engine/utils/StringUtils.h>
#include <engine/scene/SceneObject.h>
#include <functional>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { struct Scene; }

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
    // Single texture buttton
    AnimatedButton
    (
        const glm::vec3& position,
        const glm::vec3& scale,
        const std::string& textureFilename,
        const strutils::StringId& buttonName,
        std::function<void()> onPressCallback,
        std::shared_ptr<scene::Scene> scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float snapToEdgeScaleOffsetFactor = 1.0f
    );
    
    // Single font string buttton
    AnimatedButton
    (
        const glm::vec3& position,
        const glm::vec3& scale,
        const strutils::StringId& fontName,
        const std::string& text,
        const strutils::StringId& buttonName,
        std::function<void()> onPressCallback,
        std::shared_ptr<scene::Scene> scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float snapToEdgeScaleOffsetFactor = 1.0f
    );
    
    // Texture transforms based on inner font string
    AnimatedButton
    (
        const glm::vec3& position,
        const glm::vec3& textScale,
        const float textureAspectRatio,
        const std::string& textureFilename,
        const strutils::StringId& fontName,
        const std::string& text,
        const strutils::StringId& buttonName,
        std::function<void()> onPressCallback,
        std::shared_ptr<scene::Scene> scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float snapToEdgeScaleOffsetFactor = 1.0f
    );
    
    // Inner font transforms based on texture
    AnimatedButton
    (
        const glm::vec3& texturePosition,
        const glm::vec3& textureScale,
        const std::string& textureFilename,
        const strutils::StringId& fontName,
        const std::string& text,
        const strutils::StringId& buttonName,
        std::function<void()> onPressCallback,
        std::shared_ptr<scene::Scene> scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float snapToEdgeScaleOffsetFactor = 1.0f
    );
    
    ~AnimatedButton();
    
    ButtonUpdateInteractionResult Update(const float dtMillis);
    std::vector<std::shared_ptr<scene::SceneObject>>& GetSceneObjects();
    
private:
    std::shared_ptr<scene::Scene> mScene;
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
    std::function<void()> mOnPressCallback;
    bool mAnimating;
};

///------------------------------------------------------------------------------------------------

#endif /* AnimatedButton_h */
