///------------------------------------------------------------------------------------------------
///  AnimatedButton.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/11/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/AnimatedButton.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

static const float INTERACTION_ANIMATION_DURATION = 0.1f;
static const float INTERACTION_ANIMATION_SCALE_FACTOR = 0.5f;
static const strutils::StringId BUTTON_PULSING_ANIMATION_NAME = strutils::StringId("pulsing_animation");
static const strutils::StringId BUTTON_CLICK_ANIMATION_NAME = strutils::StringId("click_animation");

///------------------------------------------------------------------------------------------------

AnimatedButton::AnimatedButton
(
    const glm::vec3& position,
    const glm::vec3& scale,
    const std::string& textureFilename,
    const strutils::StringId& buttonName,
    std::function<void()> onPressCallback,
    scene::Scene& scene,
    scene::SnapToEdgeBehavior snapToEdgeBehavior /* = scene::SnapToEdgeBehavior::NONE */,
    const float snapToEdgeScaleOffsetFactor /* = 1.0f */
)
    : mScene(scene)
    , mOnPressCallback(onPressCallback)
    , mAnimating(false)
{
    mSceneObject = scene.CreateSceneObject(buttonName);
    
    mSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + textureFilename);
    mSceneObject->mPosition = position;
    mSceneObject->mScale = scale;
    mSceneObject->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObject->mSnapToEdgeScaleOffsetFactor = mSceneObject->mScale.x * snapToEdgeScaleOffsetFactor;
}

///------------------------------------------------------------------------------------------------

AnimatedButton::AnimatedButton
(
    const glm::vec3& position,
    const glm::vec3& scale,
    const strutils::StringId& fontName,
    const std::string& text,
    const strutils::StringId& buttonName,
    std::function<void()> onPressCallback,
    scene::Scene& scene,
    scene::SnapToEdgeBehavior snapToEdgeBehavior /* = scene::SnapToEdgeBehavior::NONE */,
    const float snapToEdgeScaleOffsetFactor /* = 1.0f */
)
    : mScene(scene)
    , mOnPressCallback(onPressCallback)
    , mAnimating(false)
{
    mSceneObject = scene.CreateSceneObject(buttonName);
    
    scene::TextSceneObjectData textData;
    textData.mFontName = fontName;
    textData.mText = text;
    
    mSceneObject->mSceneObjectTypeData = std::move(textData);
    mSceneObject->mPosition = position;
    mSceneObject->mScale = scale;
    mSceneObject->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObject->mSnapToEdgeScaleOffsetFactor = mSceneObject->mScale.x * snapToEdgeScaleOffsetFactor;
}

///------------------------------------------------------------------------------------------------

AnimatedButton::~AnimatedButton()
{
    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(BUTTON_PULSING_ANIMATION_NAME);
    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(BUTTON_CLICK_ANIMATION_NAME);
}

///------------------------------------------------------------------------------------------------

ButtonUpdateInteractionResult AnimatedButton::Update(const float)
{
    ButtonUpdateInteractionResult interactionResult = ButtonUpdateInteractionResult::NOT_CLICKED;
    
    const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(mScene.GetCamera().GetViewMatrix(), mScene.GetCamera().GetProjMatrix());
    
    auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mSceneObject);
    bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
    
    if (!mSceneObject->mInvisible && cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) && !mAnimating)
    {
        interactionResult = ButtonUpdateInteractionResult::CLICKED;
        mAnimating = true;
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        auto originalScale = mSceneObject->mScale;
        animationManager.StartAnimation(std::make_unique<rendering::PulseAnimation>(mSceneObject, INTERACTION_ANIMATION_SCALE_FACTOR, INTERACTION_ANIMATION_DURATION), [=]()
        {
            mSceneObject->mScale = originalScale;
            mAnimating = false;
        }, BUTTON_PULSING_ANIMATION_NAME);
        
        // Dummy animation to invoke callback mid-way pulse animation
        animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(mSceneObject, mSceneObject->mRotation, INTERACTION_ANIMATION_DURATION/2, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [=](){ mOnPressCallback(); }, BUTTON_CLICK_ANIMATION_NAME);
    }
    
    return interactionResult;
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<scene::SceneObject> AnimatedButton::GetSceneObject() { return mSceneObject; }

///------------------------------------------------------------------------------------------------
