///------------------------------------------------------------------------------------------------
///  ScatterAnimationFlow.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 01/04/2025
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/utils/Logging.h>
#include <game/ScatterAnimationFlow.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId SCATTER_OVERLAY_SO_NAME = strutils::StringId("overlay");
static const strutils::StringId SCATTER_GRANDMA_SO_NAME = strutils::StringId("scatter_grandma");
static const strutils::StringId SCATTER_SELECTED_SYMBOL_NAME = strutils::StringId("scatter_selected_combo_symbol");

static const std::string SCATTER_THINKING_BUBBLE_NAME_PREFIX = "thinking_bubble_";
static const std::string SCATTER_OVERLAY_TEXTURE_PATH = "game/overlay.png";
static const std::string SCATTER_GRANDMA_TEXTURE_PATH = "game/food_slot_images/scatter_grandma.png";
static const std::string SCATTER_MASK_TEXTURE_PATH = "game/food_slot_images/scatter_selected_symbol_mask.png";
static const std::string SCATTER_MASK_SHADER_PATH = "scatter_selected_symbol.vs";
static const std::string SCATTER_GRANDMA_THINKING_BUBBLE_TEXTURE_PATH = "game/grandma_thinking_bubble.png";

static const glm::vec3 SCATTER_OVERLAY_SO_POSITION = glm::vec3(0.0f, 0.0f, 4.9f);
static const glm::vec3 SCATTER_GRANDMA_SO_POSITION = glm::vec3(-0.252f, -0.077f, 5.0f);
static const glm::vec3 SCATTER_GRANDMA_THINKING_BUBBLE_INIT_POSITION = glm::vec3(-0.047f, -0.0047f, 5.0f);
static const glm::vec3 SCATTER_SELECTED_SYMBOL_POSITION = glm::vec3(0.252f, 0.077f, 5.0f);

static const glm::vec3 SCATTER_ANIMATION_THINKING_BUBBLE_MAX_SCALE = glm::vec3(0.05f, 0.05f, 1.0f);
static const glm::vec3 SCATTER_ANIMATION_COMPONENT_MAX_SCALE = glm::vec3(0.092f * 4.0f, 0.06624f * 4.0f, 1.0f);
static const glm::vec3 SCATTER_ANIMATION_OVERLAY_SCALE = glm::vec3(100.0f, 100.0f, 1.0f);
static const glm::vec3 SCATTER_ANIMATION_COMPONENT_MIN_SCALE = glm::vec3(0.001f, 0.001f, 1.0f);

static const float SCATTER_THINKING_BUBBLE_ANIMATION_DURATION = 0.4f;
static const float SCATTER_COMPONENT_ANIMATION_DURATION = 1.0f;
static const float SCATTER_ANIMATION_OVERLAY_MAX_ALPHA = 0.8f;
static const float SCATTER_THINKING_BUBBLE_HOR_DISTANCE = 0.05f;
static const float SCATTER_THINKING_BUBBLE_VER_DISTANCE = 0.035f;

///------------------------------------------------------------------------------------------------

float StartScatterAnimationFlow(scene::Scene& scene, const std::string& selectedSymbolPath)
{
    float totalAnimationDelay = 0.0f;
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    std::vector<std::shared_ptr<scene::SceneObject>> animationSceneObjects;
    
    auto overlaySo = scene.CreateSceneObject(SCATTER_OVERLAY_SO_NAME);
    overlaySo->mScale = SCATTER_ANIMATION_OVERLAY_SCALE;
    overlaySo->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    overlaySo->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_OVERLAY_TEXTURE_PATH);
    overlaySo->mPosition = SCATTER_OVERLAY_SO_POSITION;
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(overlaySo, SCATTER_ANIMATION_OVERLAY_MAX_ALPHA, SCATTER_COMPONENT_ANIMATION_DURATION), [](){});
    totalAnimationDelay += SCATTER_COMPONENT_ANIMATION_DURATION;
    animationSceneObjects.push_back(overlaySo);
    
    auto grandmaSo = scene.CreateSceneObject(SCATTER_GRANDMA_SO_NAME);
    grandmaSo->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_GRANDMA_TEXTURE_PATH);
    grandmaSo->mEffectTextureResourceIds[0] =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_MASK_TEXTURE_PATH);
    grandmaSo->mShaderResourceId =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SCATTER_MASK_SHADER_PATH);
    grandmaSo->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    grandmaSo->mPosition = SCATTER_GRANDMA_SO_POSITION;
    grandmaSo->mScale = SCATTER_ANIMATION_COMPONENT_MIN_SCALE;
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(grandmaSo, 1.0f, SCATTER_COMPONENT_ANIMATION_DURATION, animation_flags::NONE, totalAnimationDelay), [](){});
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(grandmaSo, grandmaSo->mPosition, SCATTER_ANIMATION_COMPONENT_MAX_SCALE, SCATTER_COMPONENT_ANIMATION_DURATION, animation_flags::NONE, totalAnimationDelay, math::ElasticFunction, math::TweeningMode::EASE_IN), [](){});
    totalAnimationDelay += SCATTER_COMPONENT_ANIMATION_DURATION;
    animationSceneObjects.push_back(grandmaSo);

    for (int i = 0; i < 3; ++i)
    {
        auto thinkingBubbleSo = scene.CreateSceneObject(strutils::StringId(SCATTER_THINKING_BUBBLE_NAME_PREFIX + std::to_string(i)));
        thinkingBubbleSo->mScale = SCATTER_ANIMATION_COMPONENT_MIN_SCALE;
        thinkingBubbleSo->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        thinkingBubbleSo->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_GRANDMA_THINKING_BUBBLE_TEXTURE_PATH);
        thinkingBubbleSo->mPosition = SCATTER_GRANDMA_THINKING_BUBBLE_INIT_POSITION;
        thinkingBubbleSo->mPosition.x += i * SCATTER_THINKING_BUBBLE_HOR_DISTANCE;
        thinkingBubbleSo->mPosition.y += i * SCATTER_THINKING_BUBBLE_VER_DISTANCE;
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(thinkingBubbleSo, 1.0f, SCATTER_COMPONENT_ANIMATION_DURATION, animation_flags::NONE, SCATTER_COMPONENT_ANIMATION_DURATION), [](){});
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(thinkingBubbleSo, thinkingBubbleSo->mPosition, SCATTER_ANIMATION_THINKING_BUBBLE_MAX_SCALE, SCATTER_THINKING_BUBBLE_ANIMATION_DURATION, animation_flags::NONE, totalAnimationDelay, math::ElasticFunction, math::TweeningMode::EASE_IN), [](){});
        
        totalAnimationDelay += SCATTER_THINKING_BUBBLE_ANIMATION_DURATION;
        animationSceneObjects.push_back(thinkingBubbleSo);
    }
    
    auto selectedSymbolSo = scene.CreateSceneObject(SCATTER_SELECTED_SYMBOL_NAME);
    selectedSymbolSo->mShaderFloatUniformValues[CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    selectedSymbolSo->mPosition = SCATTER_SELECTED_SYMBOL_POSITION;
    selectedSymbolSo->mScale = SCATTER_ANIMATION_COMPONENT_MIN_SCALE;
    selectedSymbolSo->mTextureResourceId =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(selectedSymbolPath);
    selectedSymbolSo->mEffectTextureResourceIds[0] =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SCATTER_MASK_TEXTURE_PATH);
    selectedSymbolSo->mShaderResourceId =  CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SCATTER_MASK_SHADER_PATH);
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(selectedSymbolSo, 1.0f, SCATTER_COMPONENT_ANIMATION_DURATION, animation_flags::NONE, totalAnimationDelay), [](){});
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(selectedSymbolSo, selectedSymbolSo->mPosition, SCATTER_ANIMATION_COMPONENT_MAX_SCALE, SCATTER_COMPONENT_ANIMATION_DURATION, animation_flags::NONE, totalAnimationDelay, math::ElasticFunction, math::TweeningMode::EASE_IN), [](){});
    totalAnimationDelay += SCATTER_COMPONENT_ANIMATION_DURATION;
    animationSceneObjects.push_back(selectedSymbolSo);
    
    // animation padding
    totalAnimationDelay += SCATTER_COMPONENT_ANIMATION_DURATION/2.0f;

    for (auto sceneObject: animationSceneObjects)
    {
        auto* scenePtr = &scene;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(totalAnimationDelay), [sceneObject, scenePtr]()
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SCATTER_COMPONENT_ANIMATION_DURATION), [sceneObject, scenePtr](){ scenePtr->RemoveSceneObject(sceneObject->mName); });
            
        });
    }
    totalAnimationDelay += SCATTER_COMPONENT_ANIMATION_DURATION;
    
    return totalAnimationDelay;
}

///------------------------------------------------------------------------------------------------
