///------------------------------------------------------------------------------------------------
///  PaylineView.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 20/03/2025
///------------------------------------------------------------------------------------------------

#include <game/PaylineView.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObjectUtils.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId HOR_REVEAL_THRESHOLD_UNIFORM_NAME = strutils::StringId("hor_reveal_threshold");

static const glm::vec3 PAYLINE_POSITION = glm::vec3(0.0f, 0.0f, 2.0f);
static const glm::vec3 PAYLINE_SCALE = glm::vec3(0.5f * 1.28f, 0.5f, 1.0f);
static const std::unordered_map<slots::PaylineType, std::string> PAYLINE_NAMES =
{
    { slots::PaylineType::PAYLINE_1, "payline_1"},
    { slots::PaylineType::PAYLINE_2, "payline_2"},
    { slots::PaylineType::PAYLINE_3, "payline_3"},
    { slots::PaylineType::PAYLINE_4, "payline_4"},
    { slots::PaylineType::PAYLINE_5, "payline_5"},
    { slots::PaylineType::PAYLINE_6, "payline_6"},
    { slots::PaylineType::PAYLINE_7, "payline_7"},
    { slots::PaylineType::PAYLINE_8, "payline_8"},
    { slots::PaylineType::PAYLINE_9, "payline_9"},
    { slots::PaylineType::PAYLINE_10, "payline_10"},
    { slots::PaylineType::PAYLINE_11, "payline_11"},
    { slots::PaylineType::PAYLINE_12, "payline_12"},
    { slots::PaylineType::PAYLINE_13, "payline_13"},
    { slots::PaylineType::PAYLINE_14, "payline_14"},
    { slots::PaylineType::PAYLINE_15, "payline_15"}
};

///------------------------------------------------------------------------------------------------

const std::string& PaylineView::GetPaylineName(const slots::PaylineType payline)
{
    return PAYLINE_NAMES.at(payline);
}

///------------------------------------------------------------------------------------------------

PaylineView::PaylineView(scene::Scene& scene, const slots::PaylineType payline)
    : mScene(scene)
    , mPayline(payline)
{
    mSceneObject = scene.CreateSceneObject(strutils::StringId(PAYLINE_NAMES.at(payline)));
    mSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT  + "game/paylines/" + mSceneObject->mName.GetString() + ".png");
    mSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT  + "payline.vs");
    mSceneObject->mPosition = PAYLINE_POSITION;
    mSceneObject->mPosition.z += static_cast<int>(payline) * 0.01f;
    mSceneObject->mScale = PAYLINE_SCALE;
    ResetAnimationVars();
}

///------------------------------------------------------------------------------------------------

void PaylineView::AnimatePaylineReveal(const float revealAnimationDurationSecs, const float hidingAnimationDurationSecs, const float delaySecs /* = 0.0f */)
{
    ResetAnimationVars();
    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(mSceneObject->mName);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mSceneObject->mShaderFloatUniformValues[HOR_REVEAL_THRESHOLD_UNIFORM_NAME], 1.0f, revealAnimationDurationSecs, animation_flags::NONE, delaySecs), [this, hidingAnimationDurationSecs]()
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mSceneObject, 0.0f, hidingAnimationDurationSecs), [this]()
        {
            ResetAnimationVars();
        });
    });
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<scene::SceneObject> PaylineView::GetSceneObject() { return mSceneObject; }

///------------------------------------------------------------------------------------------------

void PaylineView::ResetAnimationVars()
{
    mSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    mSceneObject->mShaderFloatUniformValues[HOR_REVEAL_THRESHOLD_UNIFORM_NAME] = 0.0f;
}

///------------------------------------------------------------------------------------------------
