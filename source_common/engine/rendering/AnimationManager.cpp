///------------------------------------------------------------------------------------------------
///  AnimationManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 09/10/2023
///------------------------------------------------------------------------------------------------

#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/Scene.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

namespace rendering
{

///------------------------------------------------------------------------------------------------

void AnimationManager::StartAnimation(std::unique_ptr<IAnimation> animation, std::function<void()> onCompleteCallback, const strutils::StringId animationName /* = strutils::StringId() */)
{
    if (mAnimationContainerLocked)
    {
        mAnimationsToAdd.emplace_back(AnimationEntry{ std::move(animation), onCompleteCallback, animationName });
    }
    else
    {
        mAnimations.emplace_back(AnimationEntry{ std::move(animation), onCompleteCallback, animationName });
    }
}

///------------------------------------------------------------------------------------------------

void AnimationManager::StopAnimation(const strutils::StringId& animationName)
{
    if (mAnimationContainerLocked)
    {
        mAnimationNamesToRemove.emplace_back(animationName);
    }
    else
    {
        auto findIter = std::find_if(mAnimations.cbegin(), mAnimations.cend(), [&](const AnimationEntry& entry) { return entry.mAnimationName == animationName; });
        if (findIter != mAnimations.cend())
        {
            mAnimations.erase(findIter);
        }
    }
}

///------------------------------------------------------------------------------------------------

void AnimationManager::StopAllAnimationsPlayingForSceneObject(const strutils::StringId& sceneObjectName)
{
    for(auto iter = mAnimations.begin(); iter != mAnimations.end();)
    {
        if (iter->mAnimation->VGetSceneObject() && iter->mAnimation->VGetSceneObject()->mName == sceneObjectName)
        {
            if (mAnimationContainerLocked)
            {
                auto tempAnimationName = strutils::StringId("TEMP");
                iter->mAnimationName = tempAnimationName;
                mAnimationNamesToRemove.emplace_back(tempAnimationName);
            }
            else
            {
                iter = mAnimations.erase(iter);
                continue;
            }
        }
        
        iter++;
    }
}

///------------------------------------------------------------------------------------------------

void AnimationManager::StopAllAnimations()
{
    if (mAnimationContainerLocked)
    {
        for (auto& animation: mAnimationsToAdd)
        {
            mAnimationNamesToRemove.push_back(animation.mAnimationName);
        }
        for (auto& animation: mAnimations)
        {
            mAnimationNamesToRemove.push_back(animation.mAnimationName);
        }
    }
    else
    {
        mAnimationsToAdd.clear();
        mAnimations.clear();
    }
}

///------------------------------------------------------------------------------------------------

void AnimationManager::Update(const float dtMillis)
{
    mAnimationContainerLocked = true;
    for(auto iter = mAnimations.begin(); iter != mAnimations.end();)
    {
        if (std::find(mAnimationNamesToRemove.cbegin(), mAnimationNamesToRemove.cend(), iter->mAnimationName) != mAnimationNamesToRemove.cend())
        {
            ++iter;
            continue;
        }
        
        auto updateTimeMillis = dtMillis * (iter->mAnimation->VGetSceneObject() && iter->mAnimation->VGetSceneObject()->mScene ? iter->mAnimation->VGetSceneObject()->mScene->GetUpdateTimeSpeedFactor() : 1.0f);
        
        if (iter->mAnimation->VUpdate(updateTimeMillis) == AnimationUpdateResult::FINISHED)
        {
            iter->mCompletionCallback();
            iter = mAnimations.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    mAnimationContainerLocked = false;
    
    for (const auto& animationName: mAnimationNamesToRemove)
    {
        auto findIter = std::find_if(mAnimations.cbegin(), mAnimations.cend(), [&](const AnimationEntry& entry) { return entry.mAnimationName == animationName; });
        if (findIter != mAnimations.cend())
        {
            mAnimations.erase(findIter);
        }
    }
    
    for (auto& animationEntry: mAnimationsToAdd)
    {
        mAnimations.emplace_back(std::move(animationEntry));
    }
    
    mAnimationsToAdd.clear();
    mAnimationNamesToRemove.clear();
}

///------------------------------------------------------------------------------------------------

bool AnimationManager::IsAnimationPlaying(const strutils::StringId& animationName) const
{
    auto findIter = std::find_if(mAnimations.cbegin(), mAnimations.cend(), [&](const AnimationEntry& entry) { return entry.mAnimationName == animationName; });
    return findIter != mAnimations.cend();
}

///------------------------------------------------------------------------------------------------

int AnimationManager::GetAnimationCountPlayingForSceneObject(const strutils::StringId& sceneObjectName)
{
    auto count = 0;
    for(auto iter = mAnimations.begin(); iter != mAnimations.end(); iter++)
    {
        if (iter->mAnimation->VGetSceneObject() && iter->mAnimation->VGetSceneObject()->mName == sceneObjectName)
        {
            count++;
        }
    }
    
    return count;
}

///------------------------------------------------------------------------------------------------

int AnimationManager::GetAnimationsPlayingCount() const
{
    return static_cast<int>(mAnimations.size());
}

///------------------------------------------------------------------------------------------------

int AnimationManager::GetAnimationCountPlayingWithName(const strutils::StringId& animationName) const
{
    return
          static_cast<int>(std::count_if(mAnimations.begin(), mAnimations.end(), [=](const AnimationEntry& animationEntry){ return animationEntry.mAnimationName == animationName; }))
    + static_cast<int>(std::count_if(mAnimationsToAdd.begin(), mAnimationsToAdd.end(), [=](const AnimationEntry& animationEntry){ return animationEntry.mAnimationName == animationName; })) - static_cast<int>(std::count(mAnimationNamesToRemove.begin(), mAnimationNamesToRemove.end(), animationName));
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
