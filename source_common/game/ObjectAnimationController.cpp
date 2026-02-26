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
static const float PLAYER_ANIMATION_TIME_CONSTANT = 0.000492f;
static const float ATTACK_FRAME_ANIMATION_TIME_SECS = 0.05f;
static const float NPC_FRAME_ANIMATION_TIME_SECS = 0.15f;

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

static inline int GetAnimationRowFromFacingDirection(const network::FacingDirection direction)
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

static inline bool ShouldFlipAnimation(const network::FacingDirection direction)
{
    return direction == network::FacingDirection::NORTH_WEST ||
           direction == network::FacingDirection::SOUTH_WEST ||
           direction == network::FacingDirection::WEST;
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

void ObjectAnimationController::OnNPCAttack(const strutils::StringId& npcNameId)
{
    if (mObjectAnimationInfoMap.contains(npcNameId))
    {
        mObjectAnimationInfoMap.at(npcNameId).mFrameIndex = 0;
    }
}

///------------------------------------------------------------------------------------------------

const ObjectAnimationController::ObjectAnimationInfo& ObjectAnimationController::UpdateObjectAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const network::ObjectType objectType, const network::ObjectState objectState, const network::FacingDirection facingDirection, const glm::vec3& velocity, const float dtMillis)
{
    if (!mObjectAnimationInfoMap.count(sceneObject->mName))
    {
        mObjectAnimationInfoMap[sceneObject->mName] = {};
    }
    
    if (objectType == network::ObjectType::PLAYER || objectType == network::ObjectType::NPC)
    {
        UpdateCharacterAnimation(sceneObject, objectType, objectState, facingDirection, velocity, dtMillis);
    }
    else if (objectType == network::ObjectType::ATTACK)
    {
        UpdateAttackAnimation(sceneObject, facingDirection, dtMillis);
    }
    
    sceneObject->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].first.x;
    sceneObject->mShaderFloatUniformValues[MIN_V_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].first.y;
    sceneObject->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].second.x;
    sceneObject->mShaderFloatUniformValues[MAX_V_UNIFORM_NAME] = ANIMATION_UV_MAP[mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow][mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex].second.y;
    
    if (mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation)
    {
        std::swap(sceneObject->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME], sceneObject->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME]);
    }
    
    return mObjectAnimationInfoMap[sceneObject->mName];
}

///------------------------------------------------------------------------------------------------

void ObjectAnimationController::UpdateCharacterAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const network::ObjectType objectType, const network::ObjectState objectState, const network::FacingDirection facingDirection, const glm::vec3 &velocity, const float dtMillis)
{
    if (mObjectAnimationInfoMap[sceneObject->mName].mObjectState != objectState)
    {
        // Animation Change
        switch (objectState)
        {
            case network::ObjectState::IDLE:
            case network::ObjectState::RUNNING:
            {
                switch (objectType)
                {
                    case network::ObjectType::PLAYER:
                    {
                        sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/anims/player_running/core.png");
                    } break;
                        
                    case network::ObjectType::NPC:
                    {
                        sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/anims/rat_running/core.png");
                    } break;
                        
                    default: assert(false);
                }
                
            } break;
                
            case network::ObjectState::BEGIN_MELEE:
            case network::ObjectState::MELEE_ATTACK:
            {
                mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = 0;
                switch (objectType)
                {
                    case network::ObjectType::PLAYER:
                    {
                        sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/anims/player_melee_attack/core.png");
                    } break;
                        
                    case network::ObjectType::NPC:
                    {
                        sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/anims/rat_melee_attack/core.png");
                    } break;
                        
                    default: assert(false);
                }

            } break;
            case network::ObjectState::CASTING:
            {
                switch (objectType)
                {
                    case network::ObjectType::PLAYER:
                    {
                        sceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "game/anims/player_casting/core.png");
                    } break;
                    default: assert(false);
                }
            } break;
        }
    }
    
    mObjectAnimationInfoMap[sceneObject->mName].mObjectState = objectState;
    mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = facingDirection;
    
    auto shouldProgressAnimation = objectState != network::ObjectState::BEGIN_MELEE;
    if (!shouldProgressAnimation)
    {
        mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = 0;
        mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = GetAnimationRowFromFacingDirection(facingDirection);
        mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = ShouldFlipAnimation(facingDirection);
    }
    else
    {
        if ((objectState == network::ObjectState::IDLE || objectState == network::ObjectState::RUNNING) && glm::length(velocity) <= 0.0f)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = 1;
        }
        else
        {
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum += dtMillis/1000.0f;
            
            if (objectState == network::ObjectState::MELEE_ATTACK)
            {
                float targetAnimationDuration = objectType == network::ObjectType::NPC ? NPC_FRAME_ANIMATION_TIME_SECS : ATTACK_FRAME_ANIMATION_TIME_SECS;
                if (mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum > targetAnimationDuration)
                {
                    mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum -= targetAnimationDuration;
                    mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex++;
                    if (mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex > 2)
                    {
                        mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = 2;
                        mObjectAnimationInfoMap[sceneObject->mName].mAnimationFinished = true;
                    }
                }
            }
            else
            {
                float targetAnimationTime = PLAYER_ANIMATION_TIME_CONSTANT/glm::length(velocity);
                if (mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum > targetAnimationTime)
                {
                    mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum -= targetAnimationTime;
                    mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = (mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex + 1) % 3;
                }
            }
            
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = GetAnimationRowFromFacingDirection(facingDirection);
            mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = ShouldFlipAnimation(facingDirection);
        }
    }
}

///------------------------------------------------------------------------------------------------

void ObjectAnimationController::UpdateAttackAnimation(std::shared_ptr<scene::SceneObject> sceneObject, const network::FacingDirection facingDirection, const float dtMillis)
{
    mObjectAnimationInfoMap[sceneObject->mName].mFacingDirection = facingDirection;
    mObjectAnimationInfoMap[sceneObject->mName].mAnimationRow = GetAnimationRowFromFacingDirection(facingDirection);
    mObjectAnimationInfoMap[sceneObject->mName].mFlippedAnimation = ShouldFlipAnimation(facingDirection);
    
    mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum += dtMillis/1000.0f;
    
    if (mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum > ATTACK_FRAME_ANIMATION_TIME_SECS)
    {
        mObjectAnimationInfoMap[sceneObject->mName].mAnimationTimeAccum -= ATTACK_FRAME_ANIMATION_TIME_SECS;
        mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex++;
        if (mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex > 2)
        {
            mObjectAnimationInfoMap[sceneObject->mName].mFrameIndex = 2;
            mObjectAnimationInfoMap[sceneObject->mName].mAnimationFinished = true;
        }
    }
}

///------------------------------------------------------------------------------------------------
