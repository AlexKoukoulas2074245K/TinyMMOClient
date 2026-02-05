///------------------------------------------------------------------------------------------------
///  CastBarController.cpp
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 03/02/2026
///------------------------------------------------------------------------------------------------

#include <game/CastBarController.h>
#include <game/ui/FillableBar.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/utils/StringUtils.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId FILL_CAST_BAR_ANIMATION_NAME = strutils::StringId("fill_castbar");
static const strutils::StringId CAST_BAR_BASE_NAME = strutils::StringId("cast_bar");
static const strutils::StringId CAST_BAR_MID_TEXT_NAME = strutils::StringId("cast_bar_text");

static const float CAST_BAR_SHOW_HIDE_DURATION_SECS = 0.1f;
static const float CAST_BAR_CANCEL_HIDE_DURATION_SECS = 0.2f;

///------------------------------------------------------------------------------------------------

CastBarController::CastBarController(std::shared_ptr<scene::Scene> scene)
    : mScene(scene)
{
    // Create all cast bar elements...
    mCastBar = std::make_unique<FillableBar>(glm::vec3(0.0f, -0.2f, 25.0f), glm::vec3(0.25f), CAST_BAR_BASE_NAME, scene, glm::vec4(1.0f, 0.66f, 0.0f, 0.9f), 0.0f);
    mCastBar->AddTextElement("Attacking", glm::vec3(0.0f, 0.021f, 0.1f), glm::vec3(0.0001f), CAST_BAR_MID_TEXT_NAME);
    
    // Hide them at start
    for (auto sceneObject: mCastBar->GetSceneObjects())
    {
        sceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    }
}

///------------------------------------------------------------------------------------------------

CastBarController::~CastBarController(){}

///------------------------------------------------------------------------------------------------

void CastBarController::ShowCastBar(const float revealSecs)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    for (auto sceneObject: mCastBar->GetSceneObjects())
    {
        animationManager.StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, revealSecs), [](){});
    }
}

///------------------------------------------------------------------------------------------------

void CastBarController::HideCastBar(const float hideSecs)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    for (auto sceneObject: mCastBar->GetSceneObjects())
    {
        animationManager.StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, hideSecs), [this]()
        {
            mCastBar->SetFillProgress(0.0f);
        });
    }
}

///------------------------------------------------------------------------------------------------

void CastBarController::BeginCast(const float duration, std::function<void()> onCompleteCallback)
{
    mOnCompleteCallback = onCompleteCallback;
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    for (auto sceneObject: mCastBar->GetSceneObjects())
    {
        animationManager.StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
    }

    mCastBar->SetFillProgress(0.0f);
    mCastBar->SetColorFactor(glm::vec4(1.0f, 0.66f, 0.0f, 0.9f));
    std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(CAST_BAR_MID_TEXT_NAME)->mSceneObjectTypeData).mText = "Attacking";
    ShowCastBar(CAST_BAR_SHOW_HIDE_DURATION_SECS);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mCastBar->GetFillProgress(), 1.0f, duration), [this]()
    {
        assert(mOnCompleteCallback);
        mOnCompleteCallback();
        HideCastBar(CAST_BAR_SHOW_HIDE_DURATION_SECS);
    }, FILL_CAST_BAR_ANIMATION_NAME);
}

///------------------------------------------------------------------------------------------------

void CastBarController::CancelCast()
{
    mOnCompleteCallback = nullptr;
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    mCastBar->SetColorFactor(glm::vec4(1.0f, 0.0f, 0.0f, 0.9f));
    std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(CAST_BAR_MID_TEXT_NAME)->mSceneObjectTypeData).mText = "Cancelled";
    animationManager.StopAnimation(FILL_CAST_BAR_ANIMATION_NAME);
    HideCastBar(CAST_BAR_CANCEL_HIDE_DURATION_SECS);
}

///------------------------------------------------------------------------------------------------
