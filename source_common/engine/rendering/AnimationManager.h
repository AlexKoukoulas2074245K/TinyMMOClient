///------------------------------------------------------------------------------------------------
///  AnimationManager.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 09/10/2023
///------------------------------------------------------------------------------------------------

#ifndef AnimationManager_h
#define AnimationManager_h

///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/utils/StringUtils.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

class AnimationManager final
{
    friend struct CoreSystemsEngine::SystemsImpl;
public:
    void StartAnimation(std::unique_ptr<IAnimation> animation, std::function<void()> onCompleteCallback, const strutils::StringId animationName = strutils::StringId());
    void StopAnimation(const strutils::StringId& animationName);
    void StopAllAnimationsPlayingForSceneObject(const strutils::StringId& sceneObjectName);
    void StopAllAnimations();
    void Update(const float dtMillis);
    
    bool IsAnimationPlaying(const strutils::StringId& animationName) const;
    int GetAnimationCountPlayingForSceneObject(const strutils::StringId& sceneObjectName);
    int GetAnimationsPlayingCount() const;
    int GetAnimationCountPlayingWithName(const strutils::StringId& animationName) const;
    
private:
    AnimationManager() = default;
    
private:
    struct AnimationEntry
    {
        std::unique_ptr<IAnimation> mAnimation;
        std::function<void()> mCompletionCallback;
        strutils::StringId mAnimationName;
    };
    
    std::vector<AnimationEntry> mAnimations;
    std::vector<AnimationEntry> mAnimationsToAdd;
    std::vector<strutils::StringId> mAnimationNamesToRemove;
    bool mAnimationContainerLocked = false;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* AnimationManager_h */
