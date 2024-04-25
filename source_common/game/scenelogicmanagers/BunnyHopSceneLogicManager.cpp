///------------------------------------------------------------------------------------------------
///  BunnyHopSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 01/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/BunnyHopSceneLogicManager.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId BUNNY_HOP_SCENE_NAME = strutils::StringId("bunny_hop_scene");
static const strutils::StringId BACKGROUND_SCENE_OBJECT_NAME = strutils::StringId("background");
static const strutils::StringId BUNNY_HOP_ICON_SCENE_OBJECT_NAME = strutils::StringId("bunny_hop_icon");

static const glm::vec3 BUNNY_ICON_INIT_POSITION = {1.0f, -1.0f, 2.0f};
static const glm::vec3 BUNNY_ICON_END_POSITION = {-1.0f, 1.0f, 2.0f};

static const float BUNNY_HOP_ANIMATION_DURATION_SECS = 5.0f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    BUNNY_HOP_SCENE_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& BunnyHopSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

void BunnyHopSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void BunnyHopSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    auto bunnyHopIconSceneObject = scene->FindSceneObject(BUNNY_HOP_ICON_SCENE_OBJECT_NAME);
    bunnyHopIconSceneObject->mPosition = BUNNY_ICON_INIT_POSITION;
    auto targetPosition = BUNNY_ICON_END_POSITION;
    
    // Eagle event hides bunny icon
    if (DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == DataRepository::GetInstance().GetPreBossMidMapNodeCoord())
    {
        bunnyHopIconSceneObject->mInvisible = true;
    }
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(bunnyHopIconSceneObject, targetPosition, bunnyHopIconSceneObject->mScale, BUNNY_HOP_ANIMATION_DURATION_SECS), []()
    {
        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::STORY_MAP_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
    });
}

///------------------------------------------------------------------------------------------------

void BunnyHopSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    static float time = 0.0f;
    time += dtMillis/1000.0f;
    
    scene->FindSceneObject(BACKGROUND_SCENE_OBJECT_NAME)->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
}

///------------------------------------------------------------------------------------------------

void BunnyHopSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene>)
{
    
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> BunnyHopSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------
