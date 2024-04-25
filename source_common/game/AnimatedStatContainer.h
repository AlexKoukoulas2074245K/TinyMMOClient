///------------------------------------------------------------------------------------------------
///  AnimatedStatContainer.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/11/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef AnimatedStatContainer_h
#define AnimatedStatContainer_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <engine/scene/Scene.h>
#include <memory>
#include <vector>

///------------------------------------------------------------------------------------------------

enum class AnimatedStatContainerUpdateResult
{
    FINISHED, ONGOING
};

///------------------------------------------------------------------------------------------------

class AnimatedStatContainer final
{
public:
    AnimatedStatContainer
    (
        const glm::vec3& position,
        const std::string& textureFilename,
        const std::string& crystalName,
        const int* valueToTrack,
        const bool startHidden,
        scene::Scene& scene,
        scene::SnapToEdgeBehavior snapToEdgeBehavior = scene::SnapToEdgeBehavior::NONE,
        const float customScaleFactor = 1.0f
    );
    ~AnimatedStatContainer();
    
    AnimatedStatContainerUpdateResult Update(const float dtMillis);
    
    std::vector<std::shared_ptr<scene::SceneObject>>& GetSceneObjects();
    int GetDisplayedValue() const;
    void ForceSetDisplayedValue(const int displayedValue);
    void RealignBaseAndValueSceneObjects();
    void ChangeTrackedValue(const int* newValueToTrack);
    
private:
    const glm::vec3 mInitCrystalBasePosition;
    const int* mValueToTrack;
    const float mScaleFactor;
    int mDisplayedValue;
    float mValueChangeDelaySecs;
    std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
    bool mFinishedAnimating;
};

///------------------------------------------------------------------------------------------------

#endif /* AnimatedStatContainer_h */
