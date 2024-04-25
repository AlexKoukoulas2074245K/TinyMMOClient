///------------------------------------------------------------------------------------------------
///  DemonPunchGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 21/02/2024
///------------------------------------------------------------------------------------------------

#include <game/AchievementManager.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/gameactions/DemonPunchGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameOverResurrectionCheckGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>

///------------------------------------------------------------------------------------------------

const std::string DemonPunchGameAction::DEMON_PUNCH_DAMAGE_PARAM = "demonPunchDamage";

static const std::string EXPLOSION_SFX = "sfx_explosion";
static const std::string DEMON_PUNCH_ICON_SHADER_FILE_NAME = "demon_punch.vs";
static const std::string DEMON_PUNCH_ICON_TEXTURE_FILE_NAME = "demon_punch.png";
static const std::string DEMON_PUNCH_ICON_EFFECT_TEXTURE_FILE_NAME = "trap_mask.png";

static const strutils::StringId DEMON_PUNCH_PARTICLE_NAME = strutils::StringId("card_play");
static const strutils::StringId GAME_OVER_CHECK_GAME_ACTION_NAME = strutils::StringId("GameOverResurrectionCheckGameAction");
static const strutils::StringId DEMON_PUNCH_ICON_SCENE_OBJECT_NAME = strutils::StringId("demon_punch_icon");

static const glm::vec3 DEMON_PUNCH_ICON_INIT_SCALE = {0.001f, 0.001f, 0.001f};
static const glm::vec3 DEMON_PUNCH_ICON_END_SCALE = {0.3f, 0.3f, 0.3f};

static const float ANIMATION_STEP_DURATION = 2.0f;
static const float ANIMATION_MAX_ALPHA = 0.6f;
static const float DEMON_PUNCH_ICON_Z = 20.0f;
static const float SHAKE_DURATION = 1.0f;
static const float SHAKE_STRENGTH = 0.03f;
static const float SMOKE_Z_OFFSET = -0.09f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    DemonPunchGameAction::DEMON_PUNCH_DAMAGE_PARAM
};

///------------------------------------------------------------------------------------------------

void DemonPunchGameAction::VSetNewGameState()
{
    auto& inactivePlayerState = mBoardState->GetInactivePlayerState();
    
    assert(mExtraActionParams.count(DEMON_PUNCH_DAMAGE_PARAM) == 1);
    
    auto demonPunchDamage = std::stoi(mExtraActionParams.at(DEMON_PUNCH_DAMAGE_PARAM));
    
    mPendingDamage = demonPunchDamage;
    mAmountOfArmorDamaged = 0;
    mAmountOfHealthDamaged = 0;
    
    if (demonPunchDamage > 0)
    {
        if (inactivePlayerState.mPlayerCurrentArmor > 0)
        {
            int startingArmorValue = inactivePlayerState.mPlayerCurrentArmor;
            inactivePlayerState.mPlayerCurrentArmor = math::Max(0, inactivePlayerState.mPlayerCurrentArmor - demonPunchDamage);
            demonPunchDamage = math::Max(0, demonPunchDamage - startingArmorValue);
            mAmountOfArmorDamaged = math::Min(startingArmorValue, mPendingDamage);
        }
        
        if (demonPunchDamage > 0 && inactivePlayerState.mPlayerCurrentArmor <= 0)
        {
            inactivePlayerState.mPlayerHealth -= demonPunchDamage;
            mAmountOfHealthDamaged = demonPunchDamage;
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

void DemonPunchGameAction::VInitAnimation()
{
    mAnimationState = ActionState::ANIMATION_GROWING;
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);

    auto demonPunchIconSceneObject = scene->CreateSceneObject(DEMON_PUNCH_ICON_SCENE_OBJECT_NAME);
    demonPunchIconSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = ANIMATION_MAX_ALPHA;
    demonPunchIconSceneObject->mPosition.z = DEMON_PUNCH_ICON_Z;
    demonPunchIconSceneObject->mScale = DEMON_PUNCH_ICON_INIT_SCALE;
    demonPunchIconSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + DEMON_PUNCH_ICON_SHADER_FILE_NAME);
    demonPunchIconSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DEMON_PUNCH_ICON_TEXTURE_FILE_NAME);
    demonPunchIconSceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DEMON_PUNCH_ICON_EFFECT_TEXTURE_FILE_NAME);
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(EXPLOSION_SFX);
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(demonPunchIconSceneObject, demonPunchIconSceneObject->mPosition, DEMON_PUNCH_ICON_END_SCALE, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(demonPunchIconSceneObject, 0.0f, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(SHAKE_DURATION), [=]()
        {
            if (mPendingDamage != 0)
            {
                auto targetPosition =
                    mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ?
                    game_constants::HEALTH_CRYSTAL_BOT_POSITION :
                    game_constants::HEALTH_CRYSTAL_TOP_POSITION;
                targetPosition.z += SMOKE_Z_OFFSET;
                
                CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
                (
                    DEMON_PUNCH_PARTICLE_NAME,
                    targetPosition,
                    *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)
                );
                
                if (mAmountOfArmorDamaged > 0)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::ArmorChangeChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX, mBoardState->GetInactivePlayerState().mPlayerCurrentArmor);
                    
                    if (mAmountOfHealthDamaged > 0)
                    {
                        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(game_constants::PER_ARMOR_DROPPED_DELAY_ANIMATION_DURATION_SECS * mAmountOfArmorDamaged), [&]()
                        {
                            mAnimationState = ActionState::FINISHED;
                            events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
                        });
                    }
                    else
                    {
                        mAnimationState = ActionState::FINISHED;
                    }
                }
                else
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX);
                    mAnimationState = ActionState::FINISHED;
                }
            }
        });
        
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EXPLOSION_SFX);
        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(SHAKE_DURATION, SHAKE_STRENGTH);
        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->RemoveSceneObject(DEMON_PUNCH_ICON_SCENE_OBJECT_NAME);
    });
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult DemonPunchGameAction::VUpdateAnimation(const float)
{
    if (mAnimationState == ActionState::FINISHED)
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

bool DemonPunchGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& DemonPunchGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
