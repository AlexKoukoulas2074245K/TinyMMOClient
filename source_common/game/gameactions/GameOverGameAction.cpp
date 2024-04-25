///------------------------------------------------------------------------------------------------
///  GameOverGameAction.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 28/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <game/AchievementManager.h>
#include <game/Cards.h>
#include <game/CardUtils.h>
#include <game/gameactions/GameOverGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <game/TutorialManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/Logging.h>

///------------------------------------------------------------------------------------------------

const strutils::StringId VICTORIOUS_TEXT_SCENE_OBJECT_NAME = strutils::StringId("victorious_player_text");
const std::string GameOverGameAction::VICTORIOUS_PLAYER_INDEX_PARAM = "victoriousPlayerIndex";

static const std::string CARD_DISSOLVE_SHADER_FILE_NAME = "card_dissolve.vs";
static const std::string DISSOLVE_TEXTURE_FILE_NAME = "dissolve.png";
static const std::string VICTORY_SFX = "sfx_victory";
static const std::string STORY_VICTORY_THEME_MUSIC = "story_victory_theme";
static const std::string EMPTY_MUSIC = "empty_music";
static const std::string EXPLOSION_SFX = "sfx_explosion";

static const strutils::StringId STORY_VICTORY_SCENE_NAME = strutils::StringId("victory_scene");
static const strutils::StringId CARD_SELECTION_REWARD_SCENE_NAME = strutils::StringId("card_selection_reward_scene");
static const strutils::StringId WHEEL_OF_FORTUNE_SCENE_NAME = strutils::StringId("wheel_of_fortune_scene");
static const strutils::StringId DEFEAT_SCENE_NAME = strutils::StringId("defeat_scene");
static const strutils::StringId DISSOLVE_THRESHOLD_UNIFORM_NAME = strutils::StringId("dissolve_threshold");
static const strutils::StringId DISSOLVE_MAGNITUDE_UNIFORM_NAME = strutils::StringId("dissolve_magnitude");
static const strutils::StringId CARD_ORIGIN_X_UNIFORM_NAME = strutils::StringId("card_origin_x");
static const strutils::StringId CARD_ORIGIN_Y_UNIFORM_NAME = strutils::StringId("card_origin_y");
static const strutils::StringId HERO_CARD_DESTRUCTION_PARTICLE_NAME = strutils::StringId("hero_card_destruction");

static const float CARD_CAMERA_SHAKE_DURATION = 0.25f;
static const float CARD_CAMERA_SHAKE_STRENGTH = 0.005f;
static const float CARD_DISSOLVE_SPEED = 0.0005f;
static const float MAX_CARD_DISSOLVE_VALUE = 1.2f;
static const float EXPLOSION_DELAY_SECS = 0.8f;

static const int NORMAL_MAX_EXPLOSIONS = 5;
static const int BOSS_MAX_EXPLOSIONS = 20;

static const glm::vec2 CARD_DISSOLVE_EFFECT_MAG_RANGE = {10.0f, 18.0f};

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    GameOverGameAction::VICTORIOUS_PLAYER_INDEX_PARAM
};

///------------------------------------------------------------------------------------------------

void GameOverGameAction::VSetNewGameState()
{
    assert(mExtraActionParams.count(VICTORIOUS_PLAYER_INDEX_PARAM) != 0);
    logging::Log(logging::LogType::INFO, "%s", ("Player " + mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM) + " won!").c_str());
}

///------------------------------------------------------------------------------------------------

void GameOverGameAction::VInitAnimation()
{
    auto& scene = *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(EXPLOSION_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(VICTORY_SFX);
    
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !DataRepository::GetInstance().GetQuickPlayData())
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EMPTY_MUSIC);
        
        if (std::stoi(mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM)) == game_constants::LOCAL_PLAYER_INDEX)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::BOARD_SIDE_DEBUFF);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::KILL_NEXT);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::DEMON_KILL_NEXT);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::SPELL_KILL_NEXT);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::DUPLICATE_NEXT_INSECT);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::DIG_NO_FAIL);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::RODENT_LIFESTEAL);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::DOUBLE_POISON_ATTACKS);
            events::EventSystem::GetInstance().DispatchEvent<events::BoardSideCardEffectEndedEvent>(true, true, effects::board_modifier_masks::INSECT_VIRUS);
            
            events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::BATTLE_VICTORY_TUTORIAL);
            
            if (mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth >= DataRepository::GetInstance().StoryCurrentHealth().GetValue())
            {
                events::EventSystem::GetInstance().DispatchEvent<events::FlawlessVictoryTriggerEvent>();
            }
            
            if (DataRepository::GetInstance().GetNextStoryOpponentName() == game_constants::EMERALD_DRAGON_NAME.GetString() && mBoardState->GetTurnCounter() == 1)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::ONE_SHOT_EMERALD_DRAGON);
            }
            
            if (DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEFEAT_FINAL_BOSS_FIRST_TIME);
                
                if (mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mHasResurrectionActive)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEFEAT_FINAL_BOSS_WITH_UNUSED_RESURRECTION);
                }
                
                if (DataRepository::GetInstance().GetCurrentStoryMutationLevel() == game_constants::MAX_MUTATION_LEVEL)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::DEFEAT_FINAL_BOSS_10_MUTATIONS);
                }
            }
            
            mExplosionDelaySecs = EXPLOSION_DELAY_SECS;
            mAnimationState = AnimationState::EXPLOSIONS;
            mExplosionCounter = 0;
        }
        else
        {
            mAnimationState = AnimationState::DEFEAT;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(scene.GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
            events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(DEFEAT_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
        }
    }
    else
    {
        auto victorTextSo = scene.CreateSceneObject(VICTORIOUS_TEXT_SCENE_OBJECT_NAME);
        
        scene::TextSceneObjectData damageTextData;
        damageTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        damageTextData.mText = "Player " + mExtraActionParams.at(VICTORIOUS_PLAYER_INDEX_PARAM) + " won!";
        
        victorTextSo->mSceneObjectTypeData = std::move(damageTextData);
        victorTextSo->mScale = glm::vec3(game_constants::IN_GAME_CARD_PROPERTY_SCALE * 3);
        victorTextSo->mPosition = glm::vec3(-0.1f, 0.0f, 5.0f);
    }
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult GameOverGameAction::VUpdateAnimation(const float dtMillis)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty())
    {
        switch (mAnimationState)
        {
            case AnimationState::EXPLOSIONS:
            {
                auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[game_constants::REMOTE_PLAYER_INDEX][0];
                
                mExplosionDelaySecs -= dtMillis/1000.0f;
                if (mExplosionDelaySecs <= 0.0f)
                {
                    auto maxExplosions = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD ? BOSS_MAX_EXPLOSIONS : NORMAL_MAX_EXPLOSIONS;
                    mExplosionDelaySecs = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD ? (EXPLOSION_DELAY_SECS - (mExplosionCounter * 0.02f)) : (EXPLOSION_DELAY_SECS - (mExplosionCounter * 0.1f));
                    
                    if (mExplosionCounter++ <= maxExplosions)
                    {
                        auto particleEmitterPosition = cardSoWrapper->mSceneObject->mPosition;
                        particleEmitterPosition.x += math::RandomFloat(-0.02f, 0.01f);
                        particleEmitterPosition.y += math::RandomFloat(-0.01f, 0.01f);
                        particleEmitterPosition.z += math::RandomFloat(1.0f, 3.0f);
                        
                        CoreSystemsEngine::GetInstance().GetParticleManager().CreateParticleEmitterAtPosition
                        (
                             HERO_CARD_DESTRUCTION_PARTICLE_NAME,
                             particleEmitterPosition,
                             *CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)
                        );
                        
                        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EXPLOSION_SFX);
                        
                        CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetCamera().Shake(CARD_CAMERA_SHAKE_DURATION, CARD_CAMERA_SHAKE_STRENGTH);
                    }
                    else
                    {
                        cardSoWrapper->mSceneObject->mShaderResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_DISSOLVE_SHADER_FILE_NAME);
                        cardSoWrapper->mSceneObject->mEffectTextureResourceIds[1] = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DISSOLVE_TEXTURE_FILE_NAME);
                        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] = 0.0f;
                        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[CARD_ORIGIN_X_UNIFORM_NAME] = cardSoWrapper->mSceneObject->mPosition.x;
                        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[CARD_ORIGIN_Y_UNIFORM_NAME] = cardSoWrapper->mSceneObject->mPosition.y;
                        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_MAGNITUDE_UNIFORM_NAME] = math::RandomFloat(CARD_DISSOLVE_EFFECT_MAG_RANGE.x, CARD_DISSOLVE_EFFECT_MAG_RANGE.y);
                        cardSoWrapper->mSceneObject->mPosition.z += 1.0f;
                        mAnimationState = AnimationState::DISSOLVE;
                        
                        events::EventSystem::GetInstance().DispatchEvent<events::StoryBattleWonEvent>();
                    }
                }
            } break;
                
            case AnimationState::DISSOLVE:
            {
                auto cardSoWrapper = mBattleSceneLogicManager->GetBoardCardSoWrappers()[game_constants::REMOTE_PLAYER_INDEX][0];
                cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] = math::Min(cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] + dtMillis * CARD_DISSOLVE_SPEED, MAX_CARD_DISSOLVE_VALUE);
                
                if (cardSoWrapper->mSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] >= MAX_CARD_DISSOLVE_VALUE && !CoreSystemsEngine::GetInstance().GetAnimationManager().IsAnimationPlaying(game_constants::STAT_PARTICLE_FLYING_ANIMATION_NAME))
                {
                    auto isStoryFinalBoss = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD;
                    
                    if (isStoryFinalBoss)
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(STORY_VICTORY_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                    }
                    else
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(CARD_SELECTION_REWARD_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                    }
                    
                    
                    if (DataRepository::GetInstance().GetCurrentStoryMapNodeType() == StoryMap::NodeType::ELITE_ENCOUNTER || isStoryFinalBoss)
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(WHEEL_OF_FORTUNE_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                    }
                    
                    if (isStoryFinalBoss)
                    {
                        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(STORY_VICTORY_THEME_MUSIC);
                    }
                    else
                    {
                        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EMPTY_MUSIC);
                        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(VICTORY_SFX);
                    }
                    
                    return ActionAnimationUpdateResult::FINISHED;
                }
            } break;
                
            default: break;
        }
    }
    return mAnimationState == AnimationState::FINISHED ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool GameOverGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& GameOverGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
