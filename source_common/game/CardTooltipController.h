///------------------------------------------------------------------------------------------------
///  CardTooltipController.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/01/2024                                                       
///------------------------------------------------------------------------------------------------

#ifndef CardTooltipController_h
#define CardTooltipController_h

///------------------------------------------------------------------------------------------------

#include <engine/scene/Scene.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

class CardTooltipController final
{
public:
    CardTooltipController
    (
        const glm::vec3& position,
        const glm::vec3& scale,
        const std::string& tooltipText,
        const bool startHidden,
        const bool horFlipped,
        const bool verFlipped,
        scene::Scene& scene
    );
    ~CardTooltipController();
    
    void Update(const float dtMillis);
    
    std::vector<std::shared_ptr<scene::SceneObject>>& GetSceneObjects();
    
private:
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
};

///------------------------------------------------------------------------------------------------

#endif /* CardTooltipController_h */
