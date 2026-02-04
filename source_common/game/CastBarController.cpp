///------------------------------------------------------------------------------------------------
///  CastBarController.cpp
///  TinyMMOClient
///                                                                                                
///  Created by Alex  Koukoulas on 03/02/2026
///------------------------------------------------------------------------------------------------

#include <game/CastBarController.h>
#include <game/ui/FillableBar.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/utils/StringUtils.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/SceneObject.h>

///------------------------------------------------------------------------------------------------

CastBarController::CastBarController(std::shared_ptr<scene::Scene> scene)
    : mScene(scene)
{
    // Create all cast bar elements...
    mCastBar = std::make_unique<FillableBar>(glm::vec3(0.0f, -0.2f, 25.0f), glm::vec3(0.25f), strutils::StringId("cast_bar"), scene, glm::vec4(1.0f, 0.0f, 0.0f, 0.9f), 0.5f);
    mCastBar->AddTextElement("Attacking", glm::vec3(0.0f, 0.021f, 0.1f), glm::vec3(0.0001f), strutils::StringId("cast_bar_text"));
    
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
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, hideSecs), [](){});
    }
}

///------------------------------------------------------------------------------------------------
