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

static const std::pair<glm::vec2, glm::vec2> ANIMATION_UV_MAP[5][3] =
{
    {{ {0.0f, UV_Y_STEP * 4.0f}, {UV_X_STEP, UV_Y_STEP * 5.0f} }, { {UV_X_STEP, UV_Y_STEP * 4.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 5.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 4.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 5.0f} }},
    {{ {0.0f, UV_Y_STEP * 3.0f}, {UV_X_STEP, UV_Y_STEP * 4.0f} }, { {UV_X_STEP, UV_Y_STEP * 3.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 4.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 3.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 4.0f} }},
    {{ {0.0f, UV_Y_STEP * 2.0f}, {UV_X_STEP, UV_Y_STEP * 3.0f} }, { {UV_X_STEP, UV_Y_STEP * 2.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 3.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 2.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 3.0f} }},
    {{ {0.0f, UV_Y_STEP * 1.0f}, {UV_X_STEP, UV_Y_STEP * 2.0f} }, { {UV_X_STEP, UV_Y_STEP * 1.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP * 2.0f} }, { {UV_X_STEP * 2.0f, UV_Y_STEP * 1.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP * 2.0f} }},
    {{ {0.0f, 0.0f}, {UV_X_STEP, UV_Y_STEP} }, { {UV_X_STEP, 0.0f}, {UV_X_STEP * 2.0f, UV_Y_STEP} }, { {UV_X_STEP * 2.0f, 0.0f}, { UV_X_STEP * 3.0f, UV_Y_STEP } }}
};

void PlayerAnimationController::UpdatePlayerAnimation(std::shared_ptr<scene::SceneObject> player, const float playerSpeed, const glm::vec3& velocity, const float dtMillis)
{
    static int animationIndex = 0;
    static float accum = 0.0f;
    
    static int rowIndex = 0;
    static bool flip = false;
    
    if (glm::length(velocity) <= 0.0f)
    {
        animationIndex = 1;
    }
    else
    {
        accum += dtMillis/1000.0f;
        
        float targetAnimationTime = ANIMATION_TIME_CONSTANT/playerSpeed;
        if (accum > targetAnimationTime)
        {
            accum -= targetAnimationTime;
            animationIndex = (animationIndex + 1) % 3;
        }

        // NE
        if (velocity.x > 0.0f && velocity.y > 0.0f)
        {
            rowIndex = 3;
            flip = false;
        }
        // SE
        else if (velocity.x > 0.0f && velocity.y < 0.0f)
        {
            rowIndex = 1;
            flip = false;
        }
        // SW
        else if (velocity.x < 0.0f && velocity.y < 0.0f)
        {
            rowIndex = 1;
            flip = true;
        }
        // NW
        else if (velocity.x < 0.0f && velocity.y > 0.0f)
        {
            rowIndex = 3;
            flip = true;
        }
        // W
        else if (velocity.y > 0.0f)
        {
            rowIndex = 4;
            flip = false;
        }
        // S
        else if (velocity.y < 0.0f)
        {
            rowIndex = 0;
            flip = false;
        }
        // W
        else if (velocity.x < 0.0f)
        {
            rowIndex = 2;
            flip = true;
        }
        // E
        else if (velocity.x > 0.0f)
        {
            rowIndex = 2;
            flip = false;
        }
    }
    
    player->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] = ANIMATION_UV_MAP[rowIndex][animationIndex].first.x;
    player->mShaderFloatUniformValues[MIN_V_UNIFORM_NAME] = ANIMATION_UV_MAP[rowIndex][animationIndex].first.y;
    player->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] = ANIMATION_UV_MAP[rowIndex][animationIndex].second.x;
    player->mShaderFloatUniformValues[MAX_V_UNIFORM_NAME] = ANIMATION_UV_MAP[rowIndex][animationIndex].second.y;
    
    if (flip)
    {
        player->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] *= -1.0f;
        player->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] *= -1.0f;
    }
}
