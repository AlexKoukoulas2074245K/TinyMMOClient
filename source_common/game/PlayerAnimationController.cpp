///------------------------------------------------------------------------------------------------
///  PlayerAnimationController.cpp
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/12/2025                                                       
///------------------------------------------------------------------------------------------------

#include <engine/scene/SceneObject.h>
#include <engine/rendering/CommonUniforms.h>
#include <game/PlayerAnimationController.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

static const float UV_X_STEP = 0.3333f; // Assuming 3 columns
static const float UV_Y_STEP = 0.2f;    // Assuming 5 rows
static const float ANIMATION_TIME_CONSTANT = 0.000024f;

///------------------------------------------------------------------------------------------------

static const std::pair<glm::vec2, glm::vec2> ANIMATION_UV_MAP[5][3] =
{
    {{ {0.0f, UV_Y_STEP * 4.0f}, {UV_X_STEP, UV_Y_STEP * 5.0f} }, { {UV_X_STEP, UV_Y_STEP * 4.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 5.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 4.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 5.0f} }},
    {{ {0.0f, UV_Y_STEP * 3.0f}, {UV_X_STEP, UV_Y_STEP * 4.0f} }, { {UV_X_STEP, UV_Y_STEP * 3.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 4.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 3.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 4.0f} }},
    {{ {0.0f, UV_Y_STEP * 2.0f}, {UV_X_STEP, UV_Y_STEP * 3.0f} }, { {UV_X_STEP, UV_Y_STEP * 2.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 3.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 2.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 3.0f} }},
    {{ {0.0f, UV_Y_STEP * 1.0f}, {UV_X_STEP, UV_Y_STEP * 2.0f} }, { {UV_X_STEP, UV_Y_STEP * 1.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 2.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 1.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 2.0f} }},
    {{ {0.0f, 0.0f}, {UV_X_STEP, UV_Y_STEP} }, { {UV_X_STEP, 0.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP} }, { {UV_X_STEP * 2.0f, 0.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP } }}
};

void PlayerAnimationController::OnPlayerDisconnected(const strutils::StringId& playerNameId)
{
    mPlayerAnimationInfo.erase(playerNameId);
}

const PlayerAnimationController::PlayerAnimationInfo& PlayerAnimationController::UpdatePlayerAnimation(std::shared_ptr<scene::SceneObject> player, const float playerSpeed, const glm::vec3& velocity, const float dtMillis, const int animationIndexOverride /* = -1 */)
{
    if (!mPlayerAnimationInfo.count(player->mName))
    {
        mPlayerAnimationInfo[player->mName] = {};
    }
    
    if (glm::length(velocity) <= 0.0f)
    {
        mPlayerAnimationInfo[player->mName].mFrameIndex = 1;
    }
    else
    {
        mPlayerAnimationInfo[player->mName].mAnimationTimeAccum += dtMillis/1000.0f;
        
        float targetAnimationTime = ANIMATION_TIME_CONSTANT/playerSpeed;
        if (mPlayerAnimationInfo[player->mName].mAnimationTimeAccum > targetAnimationTime)
        {
            mPlayerAnimationInfo[player->mName].mAnimationTimeAccum -= targetAnimationTime;
            mPlayerAnimationInfo[player->mName].mFrameIndex = (mPlayerAnimationInfo[player->mName].mFrameIndex + 1) % 3;
        }

        // NE
        if (velocity.x > 0.0f && velocity.y > 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 3;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = false;
        }
        // SE
        else if (velocity.x > 0.0f && velocity.y < 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 1;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = false;
        }
        // SW
        else if (velocity.x < 0.0f && velocity.y < 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 1;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = true;
        }
        // NW
        else if (velocity.x < 0.0f && velocity.y > 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 3;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = true;
        }
        // W
        else if (velocity.y > 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 4;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = false;
        }
        // S
        else if (velocity.y < 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 0;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = false;
        }
        // W
        else if (velocity.x < 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 2;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = true;
        }
        // E
        else if (velocity.x > 0.0f)
        {
            mPlayerAnimationInfo[player->mName].mAnimationIndex = 2;
            mPlayerAnimationInfo[player->mName].mFlippedAnimation = false;
        }
    }
    
    if (animationIndexOverride >= 0)
    {
        mPlayerAnimationInfo[player->mName].mAnimationIndex = animationIndexOverride;
    }
    
    player->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] = ANIMATION_UV_MAP[mPlayerAnimationInfo[player->mName].mAnimationIndex][mPlayerAnimationInfo[player->mName].mFrameIndex].first.x;
    player->mShaderFloatUniformValues[MIN_V_UNIFORM_NAME] = ANIMATION_UV_MAP[mPlayerAnimationInfo[player->mName].mAnimationIndex][mPlayerAnimationInfo[player->mName].mFrameIndex].first.y;
    player->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] = ANIMATION_UV_MAP[mPlayerAnimationInfo[player->mName].mAnimationIndex][mPlayerAnimationInfo[player->mName].mFrameIndex].second.x;
    player->mShaderFloatUniformValues[MAX_V_UNIFORM_NAME] = ANIMATION_UV_MAP[mPlayerAnimationInfo[player->mName].mAnimationIndex][mPlayerAnimationInfo[player->mName].mFrameIndex].second.y;
    
    if (mPlayerAnimationInfo[player->mName].mFlippedAnimation)
    {
        player->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] *= -1.0f;
        player->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] *= -1.0f;
    }
    
    return mPlayerAnimationInfo[player->mName];
}
