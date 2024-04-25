///------------------------------------------------------------------------------------------------
///  WheelOfFortuneController.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 08/01/2024                                                       
///------------------------------------------------------------------------------------------------

#include <engine/utils/Logging.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/sound/SoundManager.h>
#include <game/ProductRepository.h>
#include <game/WheelOfFortuneController.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId WHEEL_BASE_SCENE_OBJECT_NAME = strutils::StringId("wheel_base");
static const strutils::StringId WHEEL_POINTER_SCENE_OBJECT_NAME = strutils::StringId("wheel_pointer");
static const strutils::StringId WHEEL_CENTER_SCENE_OBJECT_NAME = strutils::StringId("wheel_center");
static const strutils::StringId NORMAL_PACK_PRODUCT_NAME = strutils::StringId("normal_card_pack");
static const strutils::StringId GOLDEN_PACK_PRODUCT_NAME = strutils::StringId("golden_card_pack");

static const std::string WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX = "wheel_item_";
static const std::string WHEEL_BASE_TEXTURE_FILE_NAME = "wheel_of_fortune.png";
static const std::string WHEEL_POINTER_TEXTURE_FILE_NAME = "wheel_of_fortune_pointer.png";
static const std::string WHEEL_CENTER_TEXTURE_FILE_NAME = "wheel_of_fortune_center.png";
static const std::string GOLDEN_CARD_PACK_SHADER_FILE_NAME = "card_pack_golden.vs";
static const std::string GOLDEN_CARD_PACK_TEXTURE_FILE_NAME = "card_pack_golden.png";
static const std::string NORMAL_CARD_PACK_SHADER_FILE_NAME = "basic.vs";
static const std::string NORMAL_CARD_PACK_TEXTURE_FILE_NAME = "card_pack_normal.png";
static const std::string CARD_PACK_REWARD_MESH_FILE_NAME = "card_pack_wheel_item.obj";
static const std::string WHEEL_REWARD_SELECTED_SFX = "sfx_wheel_reward_selected";

static const glm::vec3 WHEEL_BASE_POSITION = {-0.05f, -0.05f, 23.1f};
static const glm::vec3 WHEEL_COMPONENTS_POSITION = {-0.05f, -0.05f, 23.2f};
static const glm::vec3 WHEEL_BASE_SCALE = {0.35f, 0.35f, 0.35f};
static const glm::vec3 CARD_PACK_PRODUCT_WHEEL_ITEM_SCALE = {1/250.0f, 1/250.0f, 1/250.0f};

static const glm::vec2 WHEEL_ROTATION_MULTIPLIER_RANDOM_RANGE = {800.0f, 1200.0f};
static const float WHEEL_SPIN_ROTATION_DAMPING = 0.98f;
static const float WHEEL_MIN_ROTATION_SPEED = 0.0001f;
static const float WHEEL_INITIAL_SLOW_ROTATION_SPEED = 0.0002f;
static const float WHEEL_SPEED_DELTA_MILLIS = 16.6666f;
static const float WHEEL_ROTATION_TO_SELECTED_TARGET_ANIMATION_DURATION_SECS = 1.0f;

///------------------------------------------------------------------------------------------------

WheelOfFortuneController::WheelOfFortuneController(scene::Scene& scene, const std::vector<strutils::StringId>& productNames, std::function<void(const int, const std::shared_ptr<scene::SceneObject>)> onItemSelectedCallback)
    : mScene(scene)
    , mItems(productNames)
    , mOnItemSelectedCallback(onItemSelectedCallback)
    , mWheelRotationSpeed(0.0f)
    , mWheelRotation(0.0f)
{
    auto wheelBaseSceneObject = mScene.CreateSceneObject(strutils::StringId(WHEEL_BASE_SCENE_OBJECT_NAME));
    wheelBaseSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + WHEEL_BASE_TEXTURE_FILE_NAME);
    wheelBaseSceneObject->mPosition = WHEEL_BASE_POSITION;
    wheelBaseSceneObject->mScale = WHEEL_BASE_SCALE;
    wheelBaseSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    auto wheelPointerSceneObject = mScene.CreateSceneObject(strutils::StringId(WHEEL_POINTER_SCENE_OBJECT_NAME));
    wheelPointerSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + WHEEL_POINTER_TEXTURE_FILE_NAME);
    wheelPointerSceneObject->mPosition = WHEEL_COMPONENTS_POSITION;
    wheelPointerSceneObject->mScale = WHEEL_BASE_SCALE;
    wheelPointerSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    auto wheelCenterSceneObject = mScene.CreateSceneObject(strutils::StringId(WHEEL_CENTER_SCENE_OBJECT_NAME));
    wheelCenterSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + WHEEL_CENTER_TEXTURE_FILE_NAME);
    wheelCenterSceneObject->mPosition = WHEEL_COMPONENTS_POSITION;
    wheelCenterSceneObject->mScale = WHEEL_BASE_SCALE;
    wheelCenterSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
                                                    
    for (auto i = 0U; i < mItems.size(); ++i)
    {
        auto productName = strutils::StringId(mItems[i]);
        if (productName == NORMAL_PACK_PRODUCT_NAME || productName == GOLDEN_PACK_PRODUCT_NAME)
        {
            auto wheelItemSceneObject = mScene.CreateSceneObject(strutils::StringId(WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
            wheelItemSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + std::get<std::string>(ProductRepository::GetInstance().GetProductDefinition(mItems[i]).mProductTexturePathOrCardId));
            wheelItemSceneObject->mMeshResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + CARD_PACK_REWARD_MESH_FILE_NAME);
            wheelItemSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + (productName == NORMAL_PACK_PRODUCT_NAME ? NORMAL_CARD_PACK_SHADER_FILE_NAME : GOLDEN_CARD_PACK_SHADER_FILE_NAME));
            wheelItemSceneObject->mPosition = WHEEL_COMPONENTS_POSITION;
            wheelItemSceneObject->mScale = CARD_PACK_PRODUCT_WHEEL_ITEM_SCALE;
            wheelItemSceneObject->mRotation.z -= i * math::PI/6;
            wheelItemSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        }
        // Generic wheel reward
        else
        {
            auto wheelItemSceneObject = mScene.CreateSceneObject(strutils::StringId(WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
            wheelItemSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + std::get<std::string>(ProductRepository::GetInstance().GetProductDefinition(mItems[i]).mProductTexturePathOrCardId));
            wheelItemSceneObject->mPosition = WHEEL_COMPONENTS_POSITION;
            wheelItemSceneObject->mScale = WHEEL_BASE_SCALE;
            wheelItemSceneObject->mRotation.z -= i * math::PI/6;
            wheelItemSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        }
    }
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(WHEEL_REWARD_SELECTED_SFX);
    
    mState = WheelState::INITIAL_SLOW_ROTATION;
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneController::Spin()
{
    mWheelRotationSpeed = WHEEL_INITIAL_SLOW_ROTATION_SPEED * math::ControlledRandomFloat(WHEEL_ROTATION_MULTIPLIER_RANDOM_RANGE.s, WHEEL_ROTATION_MULTIPLIER_RANDOM_RANGE.t);
    mState = WheelState::SPINNING;
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneController::Update(const float dtMillis)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    switch (mState)
    {
        case WheelState::INITIAL_SLOW_ROTATION:
        {
            mWheelRotationSpeed = 0.0f;
            
        } break;
            
        case WheelState::SPINNING:
        {
            mWheelRotationSpeed = mWheelRotationSpeed * WHEEL_SPIN_ROTATION_DAMPING;
            if (mWheelRotationSpeed < WHEEL_MIN_ROTATION_SPEED)
            {
                mWheelRotationSpeed = 0.0f;
                
                // Calculate pointee index
                auto sliceIndexFloat = (mWheelRotation + math::PI/12)/(-math::PI/6);
                auto itemIndex = static_cast<int>(sliceIndexFloat < 0.0f ? 0 : (mItems.size() - 1) - static_cast<int>(sliceIndexFloat));
                
                // Calculate rotation to target
                auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
                auto targetRotation = -mScene.FindSceneObject(strutils::StringId(WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX + std::to_string(itemIndex)))->mRotation.z;
                if (targetRotation > math::PI)
                {
                    targetRotation -= 2.0f * math::PI;
                }
                
                CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(WHEEL_REWARD_SELECTED_SFX);
                
                // Tween wheel to rotation target
                animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>
                (
                    mWheelRotation,
                    mWheelRotation + targetRotation,
                    WHEEL_ROTATION_TO_SELECTED_TARGET_ANIMATION_DURATION_SECS,
                    animation_flags::NONE, 0.0f,
                    math::ElasticFunction,
                    math::TweeningMode::EASE_IN), [=]()
                {
                    mOnItemSelectedCallback(itemIndex, mScene.FindSceneObject(strutils::StringId(WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX + std::to_string(itemIndex))));
                    mState = WheelState::FINISHED;
                });
                
                mState = WheelState::ROTATING_TO_SELECTED_ITEM;
            }
        } break;
            
        default: break;
    }
            
    mWheelRotation -= mWheelRotationSpeed * WHEEL_SPEED_DELTA_MILLIS;
    if (mWheelRotation < -math::PI * 2.0f)
    {
        mWheelRotation += math::PI * 2.0f;
    }
    
    ApplyRotationToItems();
    
    
    for (auto i = 0U; i < mItems.size(); ++i)
    {
        mScene.FindSceneObject(strutils::StringId(WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)))->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    }
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<scene::SceneObject>> WheelOfFortuneController::GetSceneObjects() const
{
    return mSceneObjects;
}

///------------------------------------------------------------------------------------------------

void WheelOfFortuneController::ApplyRotationToItems()
{
    mScene.FindSceneObject(WHEEL_BASE_SCENE_OBJECT_NAME)->mRotation.z = mWheelRotation;
    
    for (auto i = 0U; i < mItems.size(); ++i)
    {
        mScene.FindSceneObject(strutils::StringId(WHEEL_ITEM_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)))->mRotation.z = -(i * math::PI/6) + mWheelRotation;
    }
}

///------------------------------------------------------------------------------------------------
