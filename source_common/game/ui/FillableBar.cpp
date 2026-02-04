///------------------------------------------------------------------------------------------------
///  FillableBar.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 03/02/2026
///------------------------------------------------------------------------------------------------

#include <game/ui/FillableBar.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObjectUtils.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId FILL_PROGRESS_UNIFORM_NAME = strutils::StringId("fill_progress");
static const strutils::StringId COLOR_FACTOR_UNIFORM_NAME = strutils::StringId("color_factor");

static const std::string FILLABLE_BAR_SHADER_FILE = "fillable_bar.vs";
static const std::string FILLABLE_BAR_FRAME_TEXTURE_FILE = "game/ui/fillable_bar_frame.png";
static const std::string FILLABLE_BAR_PROGRESS_TEXTURE_FILE = "game/ui/fillable_bar_progress.png";

///------------------------------------------------------------------------------------------------

FillableBar::FillableBar
(
    const glm::vec3& position,
    const glm::vec3& scale,
    const strutils::StringId& name,
    std::shared_ptr<scene::Scene> scene,
    const glm::vec4 colorFactor /* glm::vec4(0.0f) */,
    const float fillProgress /* 0.0f */
)
    : mScene(scene)
{
    mSceneObjects.push_back(scene->CreateSceneObject(name));
    mSceneObjects.back()->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + FILLABLE_BAR_SHADER_FILE);
    mSceneObjects.back()->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + FILLABLE_BAR_FRAME_TEXTURE_FILE);
    mSceneObjects.back()->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + FILLABLE_BAR_PROGRESS_TEXTURE_FILE);
    mSceneObjects.back()->mPosition = position;
    mSceneObjects.back()->mScale = scale;
    mSceneObjects.back()->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    mSceneObjects.back()->mShaderFloatUniformValues[FILL_PROGRESS_UNIFORM_NAME] = fillProgress;
    mSceneObjects.back()->mShaderVec4UniformValues[COLOR_FACTOR_UNIFORM_NAME] = colorFactor;
}

///------------------------------------------------------------------------------------------------

FillableBar::~FillableBar()
{
    for (auto& sceneObject: mSceneObjects)
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
    }
}

///------------------------------------------------------------------------------------------------

void FillableBar::AddTextElement(const std::string& text, const glm::vec3& offset, const glm::vec3& scale, const strutils::StringId& name)
{
    scene::TextSceneObjectData textData;
    textData.mFontName = game_constants::DEFAULT_FONT_NAME;
    textData.mText = text;
    
    auto textSceneObject = mScene->CreateSceneObject(name);
    textSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::DEFAULT_FONT_SHADER_NAME);
    textSceneObject->mSceneObjectTypeData = std::move(textData);
    textSceneObject->mPosition = mSceneObjects.front()->mPosition + offset;
    textSceneObject->mScale = scale;
    textSceneObject->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    mSceneObjects.push_back(textSceneObject);
}

///------------------------------------------------------------------------------------------------

void FillableBar::SetFillProgress(const float fillProgress)
{
    mSceneObjects.back()->mShaderFloatUniformValues[FILL_PROGRESS_UNIFORM_NAME] = fillProgress;
}

///------------------------------------------------------------------------------------------------

std::vector<std::shared_ptr<scene::SceneObject>>& FillableBar::GetSceneObjects()
{
    return mSceneObjects;
}

///------------------------------------------------------------------------------------------------
