///------------------------------------------------------------------------------------------------
///  LoadingSceneLogicManager.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 11/12/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <game/scenelogicmanagers/LoadingSceneLogicManager.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId LOADING_SCENE_NAME = strutils::StringId("loading_scene");
static const strutils::StringId LOADING_SCENE_BACKGROUND_SCENE_OBJECT_NAME = strutils::StringId("loading_background");
static const strutils::StringId LOADING_BAR_SCENE_OBJECT_NAME = strutils::StringId("loading_bar");
static const strutils::StringId LOADING_PROGRESS_TEXT_SCENE_OBJECT_NAME = strutils::StringId("loading_text");
static const strutils::StringId LOADING_TEXT_PULSE_ANIMATION_NAME = strutils::StringId("loading_text_pulse");
static const strutils::StringId LOADING_PROGRESS_UNIFORM_NAME = strutils::StringId("loading_progress");

static const float LOADING_PROGRESS_TEXT_PULSE_SCALE_FACTOR = 1.05f;
static const float LOADING_PROGRESS_TEXT_INTER_PULSE_DURATION_SECS = 1.0f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    LOADING_SCENE_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& LoadingSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

void LoadingSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void LoadingSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mLoadingProgressPrefixText = "Loading Progress: ";
    mTotalLoadingJobCount = -1;
    SetLoadingProgress(0);
    
    auto loadingProgressSceneObject = scene->FindSceneObject(LOADING_PROGRESS_TEXT_SCENE_OBJECT_NAME);
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*loadingProgressSceneObject);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    loadingProgressSceneObject->mPosition.x = -textLength/2.0f;
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(scene->FindSceneObject(LOADING_PROGRESS_TEXT_SCENE_OBJECT_NAME), LOADING_PROGRESS_TEXT_PULSE_SCALE_FACTOR, LOADING_PROGRESS_TEXT_INTER_PULSE_DURATION_SECS, animation_flags::ANIMATE_CONTINUOUSLY), [](){}, LOADING_TEXT_PULSE_ANIMATION_NAME);
    
    events::EventSystem::GetInstance().RegisterForEvent<events::LoadingProgressPrefixTextOverrideEvent>(this, &LoadingSceneLogicManager::OnLoadingProgressPrefixTextOverride);
}

///------------------------------------------------------------------------------------------------

void LoadingSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    static float time = 0.0f;
    time += dtMillis/1000.0f;

    if (mTotalLoadingJobCount == -1)
    {
        mTotalLoadingJobCount = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetOustandingLoadingJobCount();
    }
    
    auto outstandingLoadingJobCount = CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetOustandingLoadingJobCount();
    
    auto loadingProgress = mTotalLoadingJobCount == 0 ? 100 : static_cast<int>((math::Max(mTotalLoadingJobCount, outstandingLoadingJobCount) - outstandingLoadingJobCount)/static_cast<float>(mTotalLoadingJobCount) * 100.0f);
    auto loadingProgressFrac = loadingProgress/100.0f;
    
    auto loadingBarSceneObject = scene->FindSceneObject(LOADING_BAR_SCENE_OBJECT_NAME);
    loadingBarSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    loadingBarSceneObject->mShaderFloatUniformValues[LOADING_PROGRESS_UNIFORM_NAME] = loadingProgressFrac;
    SetLoadingProgress(loadingProgress);
}

///------------------------------------------------------------------------------------------------

void LoadingSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene>)
{
    SetLoadingProgress(100);
    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(LOADING_TEXT_PULSE_ANIMATION_NAME);
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> LoadingSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void LoadingSceneLogicManager::SetLoadingProgress(const int progressPercent)
{
    auto loadingBarSceneObject = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::LOADING_SCENE)->FindSceneObject(LOADING_BAR_SCENE_OBJECT_NAME);
    loadingBarSceneObject->mShaderFloatUniformValues[LOADING_PROGRESS_UNIFORM_NAME] = progressPercent/100.0f;
    
    auto loadingScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(LOADING_SCENE_NAME);
    auto loadingProgressSceneObject = loadingScene->FindSceneObject(LOADING_PROGRESS_TEXT_SCENE_OBJECT_NAME);
    std::get<scene::TextSceneObjectData>(loadingProgressSceneObject->mSceneObjectTypeData).mText = mLoadingProgressPrefixText + std::to_string(progressPercent) + "%";
}

///------------------------------------------------------------------------------------------------

void LoadingSceneLogicManager::OnLoadingProgressPrefixTextOverride(const events::LoadingProgressPrefixTextOverrideEvent& event)
{
    mLoadingProgressPrefixText = event.mLoadingProgressPrefixTextOverride;
    SetLoadingProgress(0);
    auto loadingScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(LOADING_SCENE_NAME);
    auto loadingProgressSceneObject = loadingScene->FindSceneObject(LOADING_PROGRESS_TEXT_SCENE_OBJECT_NAME);
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*loadingProgressSceneObject);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    loadingProgressSceneObject->mPosition.x = -textLength/2.0f;
}

///------------------------------------------------------------------------------------------------
