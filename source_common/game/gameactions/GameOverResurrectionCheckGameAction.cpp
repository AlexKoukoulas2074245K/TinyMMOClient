///------------------------------------------------------------------------------------------------
///  GameOverResurrectionCheckGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 29/02/2024
///------------------------------------------------------------------------------------------------

#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/GameOverGameAction.h>
#include <game/gameactions/GameOverResurrectionCheckGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/FileUtils.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

const std::string GameOverResurrectionCheckGameAction::VICTORIOUS_PLAYER_INDEX_PARAM = "victoriousPlayerIndex";

static const std::string VICTORY_SFX = "sfx_victory";
static const std::string EVIL_LAUGH_SFX = "sfx_laugh";
static const std::string GUARDIAN_ANGEL_ICON_SHADER_FILE_NAME = "rare_item.vs";
static const std::string GUARDIAN_ANGEL_ICON_TEXTURE_FILE_NAME = "rare_item_rewards/guardian_angel.png";
static const std::string FINAL_BOSS_RESURRECTION_SHADER_FILE_NAME = "demon_punch.vs";
static const std::string FINAL_BOSS_RESURRECTION_EFFECT_TEXTURE_FILE_NAME = "trap_mask.png";

static const strutils::StringId GAME_OVER_GAME_ACTION_NAME = strutils::StringId("GameOverGameAction");
static const strutils::StringId GAME_OVER_CHECK_GAME_ACTION_NAME = strutils::StringId("GameOverResurrectionCheckGameAction");
static const strutils::StringId GUARDIAN_ANGEL_ICON_SCENE_OBJECT_NAME = strutils::StringId("guardian_angel_icon");

static const glm::vec3 GUARDIAN_ANGEL_ICON_INIT_SCALE = {0.001f, 0.001f, 0.001f};
static const glm::vec3 GUARDIAN_ANGEL_ICON_END_SCALE = {0.4f, 0.4f, 0.4f};

static const float ANIMATION_STEP_DURATION = 2.0f;
static const float ANIMATION_MAX_ALPHA = 0.6f;
static const float GUARDIAN_ANGEL_ICON_Z = 20.0f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    GameOverResurrectionCheckGameAction::VICTORIOUS_PLAYER_INDEX_PARAM
};

///------------------------------------------------------------------------------------------------

void GameOverResurrectionCheckGameAction::VSetNewGameState()
{
    assert(mExtraActionParams.count(VICTORIOUS_PLAYER_INDEX_PARAM) != 0);
    
    mUsedUpResurrection = false;
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !DataRepository::GetInstance().GetQuickPlayData())
    {
        if (std::stoi(mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM)) == game_constants::REMOTE_PLAYER_INDEX &&
            mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mHasResurrectionActive)
        {
            mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mHasResurrectionActive = false;
            mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth = DataRepository::GetInstance().GetStoryMaxHealth()/2;
            mUsedUpResurrection = true;
        }
        else if (std::stoi(mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM)) == game_constants::LOCAL_PLAYER_INDEX &&
                 mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasResurrectionActive)
         {
             mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasResurrectionActive = false;
             mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerHealth = DataRepository::GetInstance().GetNextBattleTopPlayerHealth()/2;
             mUsedUpResurrection = true;
         }
    }
    
    if (!mUsedUpResurrection)
    {
        mGameActionEngine->AddGameAction(GAME_OVER_GAME_ACTION_NAME, { {GameOverGameAction::VICTORIOUS_PLAYER_INDEX_PARAM, mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM)} });
    }
}

///------------------------------------------------------------------------------------------------

void GameOverResurrectionCheckGameAction::VInitAnimation()
{
    if (mUsedUpResurrection)
    {
        mAnimationState = AnimationState::ANIMATING_ARTIFACT;
        CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(VICTORY_SFX);
        CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(EVIL_LAUGH_SFX);
        
        bool localPlayerResurrecting = std::stoi(mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM)) == game_constants::REMOTE_PLAYER_INDEX;

        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(localPlayerResurrecting ? VICTORY_SFX : EVIL_LAUGH_SFX);
        
        auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        auto scene = sceneManager.FindScene(game_constants::BATTLE_SCENE);

        auto guardianAngelIconSceneObject = scene->CreateSceneObject(GUARDIAN_ANGEL_ICON_SCENE_OBJECT_NAME);
        guardianAngelIconSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = ANIMATION_MAX_ALPHA;
        guardianAngelIconSceneObject->mPosition.z = GUARDIAN_ANGEL_ICON_Z;
        guardianAngelIconSceneObject->mScale = GUARDIAN_ANGEL_ICON_INIT_SCALE;
        
        if (localPlayerResurrecting)
        {
            guardianAngelIconSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + GUARDIAN_ANGEL_ICON_SHADER_FILE_NAME);
            guardianAngelIconSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + GUARDIAN_ANGEL_ICON_TEXTURE_FILE_NAME);
        }
        else
        {
            // "Localize" dynamically created hero card texture. This path could have come from an iPhone app.
            auto heroCardTextureFileName = fileutils::GetFileName(DataRepository::GetInstance().GetNextStoryOpponentTexturePath());
            guardianAngelIconSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + "story_cards/" + heroCardTextureFileName);
            guardianAngelIconSceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + FINAL_BOSS_RESURRECTION_EFFECT_TEXTURE_FILE_NAME);
            guardianAngelIconSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + FINAL_BOSS_RESURRECTION_SHADER_FILE_NAME);
        }
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(guardianAngelIconSceneObject, guardianAngelIconSceneObject->mPosition, GUARDIAN_ANGEL_ICON_END_SCALE, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(guardianAngelIconSceneObject, 0.0f, ANIMATION_STEP_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
        {
            mAnimationState = AnimationState::FINISHED;
            CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->RemoveSceneObject(GUARDIAN_ANGEL_ICON_SCENE_OBJECT_NAME);
            events::EventSystem::GetInstance().DispatchEvent<events::HealthChangeAnimationTriggerEvent>(std::stoi(mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM)) == game_constants::LOCAL_PLAYER_INDEX);
        });
    }
    else
    {
        mAnimationState = AnimationState::FINISHED;
    }
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult GameOverResurrectionCheckGameAction::VUpdateAnimation(const float)
{
    return mAnimationState == AnimationState::FINISHED ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool GameOverResurrectionCheckGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& GameOverResurrectionCheckGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
