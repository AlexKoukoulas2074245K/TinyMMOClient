///------------------------------------------------------------------------------------------------
///  ObjectAnimationController.cpp
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/12/2025                                                       
///------------------------------------------------------------------------------------------------

#include <engine/scene/SceneObject.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/utils/Logging.h>
#include <game/ObjectAnimationController.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

static const float UV_X_STEP = 0.3333f; // Assuming 3 columns
static const float UV_Y_STEP = 0.2f;    // Assuming 5 rows
static const float ANIMATION_TIME_CONSTANT = 0.000192f;

///------------------------------------------------------------------------------------------------

static const std::pair<glm::vec2, glm::vec2> ANIMATION_UV_MAP[5][3] =
{
    {{ {0.0f, UV_Y_STEP * 4.0f}, {UV_X_STEP, UV_Y_STEP * 5.0f} }, { {UV_X_STEP, UV_Y_STEP * 4.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 5.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 4.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 5.0f} }},
    {{ {0.0f, UV_Y_STEP * 3.0f}, {UV_X_STEP, UV_Y_STEP * 4.0f} }, { {UV_X_STEP, UV_Y_STEP * 3.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 4.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 3.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 4.0f} }},
    {{ {0.0f, UV_Y_STEP * 2.0f}, {UV_X_STEP, UV_Y_STEP * 3.0f} }, { {UV_X_STEP, UV_Y_STEP * 2.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 3.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 2.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 3.0f} }},
    {{ {0.0f, UV_Y_STEP * 1.0f}, {UV_X_STEP, UV_Y_STEP * 2.0f} }, { {UV_X_STEP, UV_Y_STEP * 1.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 2.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 1.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 2.0f} }},
    {{ {0.0f, 0.0f}, {UV_X_STEP, UV_Y_STEP} }, { {UV_X_STEP, 0.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP} }, { {UV_X_STEP * 2.0f, 0.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP } }}
};

///------------------------------------------------------------------------------------------------

ObjectAnimationController::ObjectAnimationController()
{
    events::EventSystem::GetInstance().RegisterForEvent<events::ObjectDestroyedEvent>(this, &ObjectAnimationController::OnObjectDestroyedEvent);
}

///------------------------------------------------------------------------------------------------

void ObjectAnimationController::OnObjectDestroyedEvent(const events::ObjectDestroyedEvent& objectDestroyedEvent)
{
    mObjectAnimationInfoMap.erase(objectDestroyedEvent.mSceneObjectName);
}

///------------------------------------------------------------------------------------------------

const ObjectAnimationController::ObjectAnimationInfo& ObjectAnimationController::UpdateObjectAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const glm::vec3& velocity, const float dtMillis, std::optional<network::FacingDirection> facingDirection)
{
    if (!mObjectAnimationInfoMap.count(sceneObject->mName))
    {
        mObjectAnimationInfoMap[sceneObject->mName] = {};
    }
    
    if (glm::length(velocity) <= 0.0f)
    {
        mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = 1;
    }
    else
    {
        mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum += dtMillis/1000.0f;
        
        float targetAnimationTime = ANIMATION_TIME_CONSTANT/glm::length(velocity);
        if (mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum > targetAnimationTime)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum -= targetAnimationTime;
            mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = (mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex + 1) % 3;
        }

        // NE
        if (velocity.x > 0.0f && velocity.y > 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 3;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = false;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::NORTH_EAST;
        }
        // SE
        else if (velocity.x > 0.0f && velocity.y < 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 1;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = false;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::SOUTH_EAST;
        }
        // SW
        else if (velocity.x < 0.0f && velocity.y < 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 1;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = true;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::SOUTH_WEST;
        }
        // NW
        else if (velocity.x < 0.0f && velocity.y > 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 3;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = true;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::NORTH_WEST;
        }
        // W
        else if (velocity.y > 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 4;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = false;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::NORTH;
        }
        // S
        else if (velocity.y < 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 0;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = false;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::SOUTH;
        }
        // W
        else if (velocity.x < 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 2;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = true;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::WEST;
        }
        // E
        else if (velocity.x > 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = 2;
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = false;
            mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = network::FacingDirection::EAST;
        }
    }
    
    if (facingDirection.has_value())
    {
        auto facingDirectionToAnimationRow = [](const network::FacingDirection direction)
        {
            switch (direction)
            {
                case network::FacingDirection::NORTH_WEST:
                case network::FacingDirection::NORTH_EAST:
                    return 3;
                
                case network::FacingDirection::SOUTH_WEST:
                case network::FacingDirection::SOUTH_EAST:
                    return 1;
                
                case network::FacingDirection::NORTH:
                    return 4;

                case network::FacingDirection::SOUTH:
                    return 0;
                    
                case network::FacingDirection::EAST:
                case network::FacingDirection::WEST:
                    return 2;
            }
        };
        auto shouldFlipAnimation = [](const network::FacingDirection direction)
        {
            return direction == network::FacingDirection::NORTH_WEST ||
                   direction == network::FacingDirection::SOUTH_WEST ||
                   direction == network::FacingDirection::WEST;
        };
        
        mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = facingDirectionToAnimationRow(*facingDirection);
        mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = shouldFlipAnimation(*facingDirection);
    }
    
    sceneObject->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].first.x;
    sceneObject->mShaderFloatUniformValues[MIN_V_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].first.y;
    sceneObject->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].second.x;
    sceneObject->mShaderFloatUniformValues[MAX_V_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].second.y;
    
    if (mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation)
    {
        sceneObject->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] *= -1.0f;
        sceneObject->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] *= -1.0f;
    }
    
    return mObjectAnimationInfoMap[sceneObject->mName];
}
