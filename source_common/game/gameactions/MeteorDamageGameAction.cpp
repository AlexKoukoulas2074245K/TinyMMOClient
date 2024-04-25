///------------------------------------------------------------------------------------------------
///  MeteorDamageGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 22/02/2024
///------------------------------------------------------------------------------------------------

#include <game/AchievementManager.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameOverResurrectionCheckGameAction.h>
#include <game/gameactions/CardDestructionGameAction.h>
#include <game/GameConstants.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/MeteorDamageGameAction.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneManager.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string MeteorDamageGameAction::METEOR_DAMAGE_PARAM = "meteorDamage";

static const strutils::StringId GAME_OVER_CHECK_GAME_ACTION_NAME = strutils::StringId("GameOverResurrectionCheckGameAction");
static const strutils::StringId METEOR_SCENE_OBJECT_NAME = strutils::StringId("meteor");
static const strutils::StringId METEOR_IMPACT_PARTICLE_NAME = strutils::StringId("meteor_impact");

static const std::string EXPLOSION_SFX = "sfx_explosion";
static const std::string METEOR_MESH_FILE_NAME = "meteor.obj";
static const std::string METEOR_TEXTURE_FILE_NAME = "meteor_model.png";

static const glm::vec3 METEOR_INIT_POSITION = {-0.3, 0.15f, 20.0f};
static const glm::vec3 METEOR_INIT_SCALE = {0.05f, 0.05f, 0.05f};
static const glm::vec3 METEOR_END_SCALE = {0.02f, 0.02f, 0.02f};

static const float CAMERA_SHAKE_DURATION = 1.5f;
static const float CAMERA_SHAKE_STRENGTH = 0.035f;
static const float METEOR_TRAVEL_DURATION_SECS = 1.0f;
static const float METEOR_FADE_OUT_DURATION_SECS = 0.2f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =

{
    MeteorDamageGameAction::METEOR_DAMAGE_PARAM
};

///------------------------------------------------------------------------------------------------

void MeteorDamageGameAction::VSetNewGameState()
{
    assert(mExtraActionParams.count(METEOR_DAMAGE_PARAM) == 1);
    
    auto& inactivePlayerState = mBoardState->GetInactivePlayerState();
    auto meteorDamage = std::stoi(mExtraActionParams.at(METEOR_DAMAGE_PARAM));
    
    mPendingDamage = meteorDamage;
    mAmountOfArmorDamaged = 0;
    mAmountOfHealthDamaged = 0;
    
    if (meteorDamage > 0)
    {
        if (inactivePlayerState.mPlayerCurrentArmor > 0)
        {
            int startingArmorValue = inactivePlayerState.mPlayerCurrentArmor;
            inactivePlayerState.mPlayerCurrentArmor = math::Max(0, inactivePlayerState.mPlayerCurrentArmor - meteorDamage);
            meteorDamage = math::Max(0, meteorDamage - startingArmorValue);
            mAmountOfArmorDamaged = math::Min(startingArmorValue, mPendingDamage);
        }
        
        if (meteorDamage > 0 && inactivePlayerState.mPlayerCurrentArmor <= 0)
        {
            inactivePlayerState.mPlayerHealth -= meteorDamage;
            mAmountOfHealthDamaged = meteorDamage;
        }
        
        if (inactivePlayerState.mPlayerHealth <= 0)
        {
            inactivePlayerState.mPlayerHealth = 0;
            mGameActionEngine->AddGameAction(GAME_OVER_CHECK_GAME_ACTION_NAME,
            {
                { GameOverResurrectionCheckGameAction::VICTORIOUS_PLAYER_INDEX_PARAM, std::to_string(mBoardState->GetActivePlayerIndex())}
            });
        }
    }
}

///------------------------------------------------------------------------------------------------

void MeteorDamageGameAction::VInitAnimation()
{
    mFinished = false;
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(EXPLOSION_SFX);
    
    // Create Meteor Model
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto meteorSceneObject = sceneManager.FindScene(game_constants::BATTLE_SCENE)->CreateSceneObject(METEOR_SCENE_OBJECT_NAME);
    meteorSceneObject->mMeshResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + METEOR_MESH_FILE_NAME);
    meteorSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + METEOR_TEXTURE_FILE_NAME);
    meteorSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    meteorSceneObject->mPosition = METEOR_INIT_POSITION;
    meteorSceneObject->mScale = METEOR_INIT_SCALE;
    
    // Calculate target meteor position based on active player
    auto targetPosition = glm::vec3(0.0f, mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::IN_GAME_BOT_PLAYER_BOARD_CARD_Y : game_constants::IN_GAME_TOP_PLAYER_BOARD_CARD_Y, meteorSceneObject->mPosition.z);
    
    // Initiate Meteor movement & scale animation
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(meteorSceneObject, targetPosition, METEOR_END_SCALE, METEOR_TRAVEL_DURATION_SECS), [=]()
    {
    
        // Fade Meteor Out
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(meteorSceneObject, 0.0f, METEOR_FADE_OUT_DURATION_SECS), []()
        {
            CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->RemoveSceneObject(METEOR_SCENE_OBJECT_NAME);
        });
        
        // Play SFX, Particle & Camera Shake
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EXPLOSION_SFX);
        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(CAMERA_SHAKE_DURATION, CAMERA_SHAKE_STRENGTH);
        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
        (
            METEOR_IMPACT_PARTICLE_NAME,
            targetPosition,
            *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)
        );
        
        // Time delay the actual armor/health reduction animatinos
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(CAMERA_SHAKE_DURATION), [=]()
        {
            if (mPendingDamage != 0)
            {
                if (mAmountOfArmorDamaged > 0)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, mBoardState->GetInactivePlayerState().mPlayerCurrentArmor);
                    
                    if (mAmountOfHealthDamaged > 0)
                    {
                        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(game_constants::PER_ARMOR_DROPPED_DELAY_ANIMATION_DURATION_SECS * mAmountOfArmorDamaged), [&]()
                        {
                            mFinished = true;
                            events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
                        });
                    }
                    else
                    {
                        mFinished = true;
                    }
                }
                else
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
                    mFinished = true;
                }
            }
        });
    });
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult MeteorDamageGameAction::VUpdateAnimation(const float dtMillis)
{
    if (mFinished)
    {
        if (mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX)
        {
            if (mPendingDamage >= 10)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEAL_10_DAMAGE);
            }
            
            if (mPendingDamage >= 20)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEAL_20_DAMAGE);
            }
        }
        
        return ActionAnimationUpdateResult::FINISHED;
    }
    
    return ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool MeteorDamageGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& MeteorDamageGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
