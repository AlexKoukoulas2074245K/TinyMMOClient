///------------------------------------------------------------------------------------------------
///  FillableBar.h
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 03/02/2026
///------------------------------------------------------------------------------------------------

#ifndef FillableBar_h
#define FillableBar_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <functional>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene { class Scene; }
namespace scene { struct SceneObject; }

///------------------------------------------------------------------------------------------------

class FillableBar final
{
public:
    FillableBar
    (
        const glm::vec3& position,
        const glm::vec3& scale,
        const strutils::StringId& name,
        std::shared_ptr<scene::Scene> scene,
        const glm::vec4 colorFactor = glm::vec4(0.0f),
        const float fillProgress = 0.0f
    );
    
    ~FillableBar();
    
    void AddTextElement(const std::string& text, const glm::vec3& offset, const glm::vec3& scale, const strutils::StringId& name);
    void SetFillProgress(const float fillProgress);
    
    std::vector<std::shared_ptr<scene::SceneObject>>& GetSceneObjects();
    
private:
    std::shared_ptr<scene::Scene> mScene;
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
};

///------------------------------------------------------------------------------------------------

#endif /* FillableBar_h */
