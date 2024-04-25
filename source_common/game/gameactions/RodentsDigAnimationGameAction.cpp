///------------------------------------------------------------------------------------------------
///  RodentsDigAnimationGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/11/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/sound/SoundManager.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/RodentsDigAnimationGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>

///------------------------------------------------------------------------------------------------

const std::string RodentsDigAnimationGameAction::CARD_INDEX_PARAM = "cardIndex";
const std::string RodentsDigAnimationGameAction::PLAYER_INDEX_PARAM = "playerIndex";

static const strutils::StringId SHOVEL_SCENE_OBEJECT_NAME = strutils::StringId("dig_shovel");
static const strutils::StringId DIRT_PARTICLE_NAME = strutils::StringId("dirt");

static const std::string DIGGING_SFX = "sfx_digging";
static const std::string SHOVEL_TEXTURE_FILE_NAME = "shovel.png";
static const int ANIMATION_STEP_COUNT = 3;
static const float TARGET_ANIMATION_DURATION = 2.0f;
static const float SHOVEL_Y_STEP = -0.015f;
static const float DIRT_Y_OFFSET = -0.04f;
static const float SHOVEL_SHOWHIDE_ANIMATION_DURATION_SECS = 0.5f;
static const float SHOVEL_ROTATION_RIGHT_ANIMATION_DURATION_SECS = 0.4f;
static const float SHOVEL_ROTATION_LEFT_ANIMATION_DURATION_SECS = 0.1f;
static const float SHOVEL_Y_MOVEMENT_ANIMATION_DURATION_SECS = 0.3f;

static const glm::vec3 SHOVEL_OFFSET = {-0.009f, 0.065f, 0.1f};
static const glm::vec3 SHOVEL_SCALE = glm::vec3(0.075f);
static const glm::vec2 SHOVEL_MIN_MAX_ROTATIONS = {-0.250f, 0.350f};

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    RodentsDigAnimationGameAction::CARD_INDEX_PARAM,
    RodentsDigAnimationGameAction::PLAYER_INDEX_PARAM
};

///------------------------------------------------------------------------------------------------

void RodentsDigAnimationGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void RodentsDigAnimationGameAction::VInitAnimation()
{
    mStepsFinished = 0;
    mSecsAccum = 0.0f;
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    systemsEngine.GetSoundManager().PreloadSfx(DIGGING_SFX);
    
    auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
    auto playerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
    
    auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(playerIndex).at(cardIndex);
    
    auto shovelSceneObject = scene->CreateSceneObject(SHOVEL_SCENE_OBEJECT_NAME);
    shovelSceneObject->mPosition = cardSoWrapper->mSceneObject->mPosition + SHOVEL_OFFSET;
    shovelSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    shovelSceneObject->mScale = SHOVEL_SCALE;
    shovelSceneObject->mRotation.z = SHOVEL_MIN_MAX_ROTATIONS.s;
    shovelSceneObject->mTextureResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SHOVEL_TEXTURE_FILE_NAME);
    
    systemsEngine.GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(SHOVEL_SCENE_OBEJECT_NAME), 1.0f, SHOVEL_SHOWHIDE_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    
    CreateAnimations();
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult RodentsDigAnimationGameAction::VUpdateAnimation(const float dtMillis)
{
    mSecsAccum += dtMillis/1000.0f;
    
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
    auto playerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
    
    auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(playerIndex).at(cardIndex);
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::DORMANT_CARD_VALUE_UNIFORM_NAME] = math::Lerp(0.0f, 1.0f, mSecsAccum/TARGET_ANIMATION_DURATION);
    
    return mStepsFinished == ANIMATION_STEP_COUNT ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool RodentsDigAnimationGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& RodentsDigAnimationGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------

void RodentsDigAnimationGameAction::CreateAnimations()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto scene = systemsEngine.GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    auto shovelSceneObject = scene->FindSceneObject(SHOVEL_SCENE_OBEJECT_NAME);
    
    auto cardIndex = std::stoi(mExtraActionParams.at(CARD_INDEX_PARAM));
    auto playerIndex = std::stoi(mExtraActionParams.at(PLAYER_INDEX_PARAM));
    
    auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers().at(playerIndex).at(cardIndex);
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto targetPosition = shovelSceneObject->mPosition;
    targetPosition.y += SHOVEL_Y_STEP;
    
    glm::vec3 targetRotation = shovelSceneObject->mRotation;
    targetRotation.z = SHOVEL_MIN_MAX_ROTATIONS.s;
    animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(shovelSceneObject, targetRotation, SHOVEL_ROTATION_LEFT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::BounceFunction, math::TweeningMode::EASE_IN), [=](){});
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(shovelSceneObject, targetPosition, shovelSceneObject->mScale, SHOVEL_Y_MOVEMENT_ANIMATION_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Z_COMPONENT, 0.0f, math::BounceFunction, math::TweeningMode::EASE_IN), [=]()
    {
        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
        (
            DIRT_PARTICLE_NAME,
            glm::vec3(targetPosition.x, targetPosition.y + DIRT_Y_OFFSET, targetPosition.z),
            *scene
        );
        
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(DIGGING_SFX);
        
        glm::vec3 targetRotation = shovelSceneObject->mRotation;
        targetRotation.z = SHOVEL_MIN_MAX_ROTATIONS.t;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(shovelSceneObject, targetRotation, SHOVEL_ROTATION_RIGHT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::BounceFunction, math::TweeningMode::EASE_IN), [=]()
        {
            if (++mStepsFinished < ANIMATION_STEP_COUNT)
            {
                CreateAnimations();
            }
            else
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(SHOVEL_SCENE_OBEJECT_NAME), 0.0f, SHOVEL_SHOWHIDE_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
                {
                    scene->RemoveSceneObject(SHOVEL_SCENE_OBEJECT_NAME);
                });
            }
        });
    });
}

///------------------------------------------------------------------------------------------------
