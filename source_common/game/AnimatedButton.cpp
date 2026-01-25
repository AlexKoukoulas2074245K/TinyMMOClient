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

static const float INTERACTION_ANIMATION_DURATION = 0.085f;
static const float INTERACTION_ANIMATION_SCALE_FACTOR = 0.6f;
static const float INNER_BUTTON_OBJECT_Z_OFFSET = 0.01f;
static const float MIN_DYNAMIC_TEXTURE_HEIGHT_MULTIPLIER = 1.4f;
static const float DYNAMIC_TEXTURE_HEIGHT_MULTIPLIER = 4.0f;
static const float DYNAMIC_TEXTURE_X_OFFSET_MULTIPLIER = 1.0f/2.5f;
static const float DYNAMIC_TEXTURE_X_OFFSET_POWER = 1.015f;
static const float DYNAMIC_TEXTURE_Y_OFFSET_MULTIPLIER = 1.0f/1.8f;
static const float DYNAMIC_TEXT_INIT_SCALE_GUESS_MULTIPLIER = 1.0f/700.0f;
static const float DYNAMIC_TEXT_POSITION_X_MULTIPLIER = 1.0f/2.3f;
static const float DYNAMIC_TEXT_POSITION_Y_MULTIPLIER = 1.0f/1.8f;
static const float DYNAMIC_TEXT_WIDTH_THRESHOLD_MULTIPLIER = 0.9f;

static const strutils::StringId BUTTON_PULSING_IN_ANIMATION_NAME = strutils::StringId("pulsing_in_animation");
static const strutils::StringId BUTTON_PULSING_OUT_ANIMATION_NAME = strutils::StringId("pulsing_out_animation");
static const strutils::StringId BUTTON_CLICK_ANIMATION_NAME = strutils::StringId("click_animation");
static const std::string BASE_BUTTON_SCENE_OBJECT_NAME_POSTFIX = "_base";
static const std::string INNER_BUTTON_SCENE_OBJECT_NAME_POSTFIX = "_inner";


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
    mSceneObjects.push_back(scene.CreateSceneObject(strutils::StringId(buttonName.GetString() + BASE_BUTTON_SCENE_OBJECT_NAME_POSTFIX)));
    
    mSceneObjects.back()->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + textureFilename);
    mSceneObjects.back()->mPosition = position;
    mSceneObjects.back()->mScale = scale;
    mSceneObjects.back()->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObjects.back()->mSnapToEdgeScaleOffsetFactor = mSceneObjects.back()->mScale.x * snapToEdgeScaleOffsetFactor;
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
    mSceneObjects.push_back(scene.CreateSceneObject(strutils::StringId(buttonName.GetString() + BASE_BUTTON_SCENE_OBJECT_NAME_POSTFIX)));
    
    scene::TextSceneObjectData textData;
    textData.mFontName = fontName;
    textData.mText = text;
    
    mSceneObjects.back()->mSceneObjectTypeData = std::move(textData);
    mSceneObjects.back()->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
    mSceneObjects.back()->mPosition = position;
    mSceneObjects.back()->mScale = scale;
    mSceneObjects.back()->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObjects.back()->mSnapToEdgeScaleOffsetFactor = mSceneObjects.back()->mScale.x * snapToEdgeScaleOffsetFactor;
}

///------------------------------------------------------------------------------------------------

AnimatedButton::AnimatedButton
(
    const glm::vec3& position,
    const glm::vec3& textScale,
    const float textureAspectRatio,
    const std::string& textureFilename,
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
    mSceneObjects.push_back(scene.CreateSceneObject(strutils::StringId(buttonName.GetString() + BASE_BUTTON_SCENE_OBJECT_NAME_POSTFIX)));
    
    mSceneObjects.back()->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + textureFilename);
    mSceneObjects.back()->mPosition = position;
    mSceneObjects.back()->mScale = textScale;
    mSceneObjects.back()->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObjects.back()->mSnapToEdgeScaleOffsetFactor = mSceneObjects.back()->mScale.x * snapToEdgeScaleOffsetFactor;
    
    mSceneObjects.push_back(scene.CreateSceneObject(strutils::StringId(buttonName.GetString() + INNER_BUTTON_SCENE_OBJECT_NAME_POSTFIX)));
    
    scene::TextSceneObjectData textData;
    textData.mFontName = fontName;
    textData.mText = text;
    
    mSceneObjects.back()->mSceneObjectTypeData = std::move(textData);
    mSceneObjects.back()->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
    mSceneObjects.back()->mPosition = position;
    mSceneObjects.back()->mPosition.z += INNER_BUTTON_OBJECT_Z_OFFSET;
    mSceneObjects.back()->mScale = textScale;
    mSceneObjects.back()->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObjects.back()->mSnapToEdgeScaleOffsetFactor = mSceneObjects.back()->mScale.x * snapToEdgeScaleOffsetFactor;
    
    auto textSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mSceneObjects.back());
    auto textSceneObjectWidth = textSceneObjectRect.topRight.x - textSceneObjectRect.bottomLeft.x;
    auto textSceneObjectHeight = textSceneObjectRect.topRight.y - textSceneObjectRect.bottomLeft.y;
    
    mSceneObjects.front()->mPosition.x += std::powf(textSceneObjectWidth * DYNAMIC_TEXTURE_X_OFFSET_MULTIPLIER, DYNAMIC_TEXTURE_X_OFFSET_POWER);
    mSceneObjects.front()->mPosition.y -= textSceneObjectHeight * DYNAMIC_TEXTURE_Y_OFFSET_MULTIPLIER;
    mSceneObjects.front()->mScale.x = math::Max(textSceneObjectHeight * DYNAMIC_TEXTURE_HEIGHT_MULTIPLIER, (textSceneObjectWidth + textSceneObjectHeight) * MIN_DYNAMIC_TEXTURE_HEIGHT_MULTIPLIER);
    mSceneObjects.front()->mScale.y = mSceneObjects.front()->mScale.x / textureAspectRatio;
}

///------------------------------------------------------------------------------------------------

AnimatedButton::AnimatedButton
(
    const glm::vec3& texturePosition,
    const glm::vec3& textureScale,
    const std::string& textureFilename,
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
    mSceneObjects.push_back(scene.CreateSceneObject(strutils::StringId(buttonName.GetString() + BASE_BUTTON_SCENE_OBJECT_NAME_POSTFIX)));
    mSceneObjects.back()->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + textureFilename);
    mSceneObjects.back()->mPosition = texturePosition;
    mSceneObjects.back()->mScale = textureScale;
    mSceneObjects.back()->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObjects.back()->mSnapToEdgeScaleOffsetFactor = mSceneObjects.back()->mScale.x * snapToEdgeScaleOffsetFactor;
    
    auto textureSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mSceneObjects.back());
    auto textureSceneObjectWidth = textureSceneObjectRect.topRight.x - textureSceneObjectRect.bottomLeft.x;
    
    mSceneObjects.push_back(scene.CreateSceneObject(strutils::StringId(buttonName.GetString() + INNER_BUTTON_SCENE_OBJECT_NAME_POSTFIX)));
    
    scene::TextSceneObjectData textData;
    textData.mFontName = fontName;
    textData.mText = text;
    
    mSceneObjects.back()->mSceneObjectTypeData = std::move(textData);
    mSceneObjects.back()->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
    mSceneObjects.back()->mPosition = texturePosition;
    mSceneObjects.back()->mPosition.z += INNER_BUTTON_OBJECT_Z_OFFSET;
    
    auto computedTextScale = textureScale.x * DYNAMIC_TEXT_INIT_SCALE_GUESS_MULTIPLIER;
    mSceneObjects.back()->mScale = glm::vec3(computedTextScale);
    
    auto textSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mSceneObjects.back());
    auto textSceneObjectWidth = textSceneObjectRect.topRight.x - textSceneObjectRect.bottomLeft.x;
    auto textSceneObjectHeight = textSceneObjectRect.topRight.y - textSceneObjectRect.bottomLeft.y;
    
    while (textSceneObjectWidth > textureSceneObjectWidth * DYNAMIC_TEXT_WIDTH_THRESHOLD_MULTIPLIER)
    {
        computedTextScale *= 0.9f;
        mSceneObjects.back()->mScale = glm::vec3(computedTextScale);
        
        textSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mSceneObjects.back());
        textSceneObjectWidth = textSceneObjectRect.topRight.x - textSceneObjectRect.bottomLeft.x;
        textSceneObjectHeight = textSceneObjectRect.topRight.y - textSceneObjectRect.bottomLeft.y;
    }
    
    mSceneObjects.back()->mPosition.y += textSceneObjectHeight * DYNAMIC_TEXT_POSITION_Y_MULTIPLIER;
    mSceneObjects.back()->mPosition.x -= textSceneObjectWidth * DYNAMIC_TEXT_POSITION_X_MULTIPLIER;
    
    mSceneObjects.back()->mSnapToEdgeBehavior = snapToEdgeBehavior;
    mSceneObjects.back()->mSnapToEdgeScaleOffsetFactor = mSceneObjects.back()->mScale.x * snapToEdgeScaleOffsetFactor;
}

///------------------------------------------------------------------------------------------------

AnimatedButton::~AnimatedButton()
{
    for (auto& sceneObject: mSceneObjects)
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
    }
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(BUTTON_CLICK_ANIMATION_NAME);
}

///------------------------------------------------------------------------------------------------

ButtonUpdateInteractionResult AnimatedButton::Update(const float)
{
    ButtonUpdateInteractionResult interactionResult = ButtonUpdateInteractionResult::NOT_CLICKED;
    
    const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(mScene.GetCamera().GetViewMatrix(), mScene.GetCamera().GetProjMatrix());
    
    auto baseSceneObject = mSceneObjects.front();
    auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*baseSceneObject);
    bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
    
    if (!baseSceneObject->mInvisible && cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) && !mAnimating)
    {
        interactionResult = ButtonUpdateInteractionResult::CLICKED;
        mAnimating = true;
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        
        auto initScale = mSceneObjects.front()->mScale;
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(mSceneObjects, mSceneObjects.front()->mPosition, initScale * INTERACTION_ANIMATION_SCALE_FACTOR, INTERACTION_ANIMATION_DURATION), [this, initScale]()
        {
            mOnPressCallback();
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(mSceneObjects, mSceneObjects.front()->mPosition, initScale, INTERACTION_ANIMATION_DURATION, animation_flags::NONE), [this](){ mAnimating = false; }, BUTTON_PULSING_OUT_ANIMATION_NAME);
        }, BUTTON_PULSING_IN_ANIMATION_NAME);
    }
    
    return interactionResult;
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<scene::SceneObject>> AnimatedButton::GetSceneObjects() { return mSceneObjects; }

///------------------------------------------------------------------------------------------------
