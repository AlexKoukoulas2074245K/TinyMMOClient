///------------------------------------------------------------------------------------------------
///  BattleInitialSetupAndAnimationGameAction.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/Animations.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/sound/SoundManager.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/BattleInitialSetupAndAnimationGameAction.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>


///------------------------------------------------------------------------------------------------

const std::string BattleInitialSetupAndAnimationGameAction::CURRENT_BATTLE_SUBSCENE_PARAM = "currentBattleSubsceneParam";

///------------------------------------------------------------------------------------------------

static const std::string BATTLE_THEME_MUSIC = "battle_theme";
static const std::string MINI_BOSS_THEME_MUSIC = "mini_boss_theme";
static const std::string FINAL_BOSS_THEME_MUSIC = "final_boss_theme";
static const std::string VICTORY_THEME_MUSIC = "victory_theme";
static const std::string EMPTY_MUSIC = "empty_music";
static const std::string VICTORY_SFX = "sfx_victory";

static const strutils::StringId STORY_VICTORY_SCENE_NAME = strutils::StringId("victory_scene");
static const strutils::StringId CARD_SELECTION_REWARD_SCENE_NAME = strutils::StringId("card_selection_reward_scene");
static const strutils::StringId WHEEL_OF_FORTUNE_SCENE_NAME = strutils::StringId("wheel_of_fortune_scene");
static const strutils::StringId BOARD_SCENE_OBJECT_NAME = strutils::StringId("board");
static const strutils::StringId REPLAY_TEXT_SCENE_OBJECT_NAME = strutils::StringId("replay_text");
static const strutils::StringId TOP_PLAYER_HEALTH_CONTAINER_BASE = strutils::StringId("health_crystal_top_base");
static const strutils::StringId TOP_PLAYER_HEALTH_CONTAINER_VALUE = strutils::StringId("health_crystal_top_value");

static const glm::vec3 BOARD_TARGET_POSITION = {-0.013f, 0.003f, 0.0f };
static const glm::vec3 BOARD_TARGET_ROTATION = {0.00f, 0.000f, math::PI/2 };

static const float BOARD_ANIMATION_DURATION_SECS = 1.0f;
static const float BOARD_ITEMS_FADE_IN_DURATION_SECS = 0.5f;

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> sRequiredExtraParamNames =
{
    BattleInitialSetupAndAnimationGameAction::CURRENT_BATTLE_SUBSCENE_PARAM
};

///------------------------------------------------------------------------------------------------

void BattleInitialSetupAndAnimationGameAction::VSetNewGameState()
{
}

///------------------------------------------------------------------------------------------------

void BattleInitialSetupAndAnimationGameAction::VInitAnimation()
{
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(VICTORY_SFX);
    
    auto battleScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    auto boardSceneObject = battleScene->FindSceneObject(BOARD_SCENE_OBJECT_NAME);
    
    boardSceneObject->mPosition = game_constants::GAME_BOARD_INIT_POSITION;
    boardSceneObject->mRotation = game_constants::GAME_BOARD_INIT_ROTATION;
    
    auto currentSubSceneType = static_cast<BattleSubSceneType>(std::stoi(mExtraActionParams.at(CURRENT_BATTLE_SUBSCENE_PARAM)));
    if (currentSubSceneType == BattleSubSceneType::BATTLE)
    {
        if (DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD)
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(MINI_BOSS_THEME_MUSIC);
        }
        else if (DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD)
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(FINAL_BOSS_THEME_MUSIC);
        }
        else
        {
            CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(BATTLE_THEME_MUSIC);
        }
    }
    else
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(EMPTY_MUSIC);
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(VICTORY_SFX);
    }
    
    // Animate board to target position
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(boardSceneObject, BOARD_TARGET_POSITION, boardSceneObject->mScale, BOARD_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        mPendingAnimations--;
    });
    mPendingAnimations++;
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenRotationAnimation>(boardSceneObject, BOARD_TARGET_ROTATION, BOARD_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
    {
        mPendingAnimations--;
    });
    mPendingAnimations++;
    
    // Fade in board scene objects with a delay matching the duration of the board animation
    for (auto& sceneObject: battleScene->GetSceneObjects())
    {
        // Only fade in normally visible elements
        if (sceneObject->mInvisible || (sceneObject->mShaderFloatUniformValues.count(game_constants::CUSTOM_ALPHA_UNIFORM_NAME) && sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] <= 0.0f))
        {
            continue;
        }
        
        if (sceneObject->mName == BOARD_SCENE_OBJECT_NAME || sceneObject->mName == REPLAY_TEXT_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !DataRepository::GetInstance().GetQuickPlayData())
        {
            if (sceneObject->mName == TOP_PLAYER_HEALTH_CONTAINER_BASE || sceneObject->mName == TOP_PLAYER_HEALTH_CONTAINER_VALUE)
            {
                continue;
            }
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, BOARD_ITEMS_FADE_IN_DURATION_SECS, animation_flags::NONE, BOARD_ANIMATION_DURATION_SECS), [=]()
        {
            mPendingAnimations--;
        });
        mPendingAnimations++;
    }
}

///------------------------------------------------------------------------------------------------

ActionAnimationUpdateResult BattleInitialSetupAndAnimationGameAction::VUpdateAnimation(const float)
{
    if (mPendingAnimations == 0)
    {
        if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !DataRepository::GetInstance().GetQuickPlayData())
        {
            auto currentSubSceneType = static_cast<BattleSubSceneType>(std::stoi(mExtraActionParams.at(CURRENT_BATTLE_SUBSCENE_PARAM)));
            
            if (currentSubSceneType == BattleSubSceneType::WHEEL)
            {
                auto isTutorialMiniBoss = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD;
                auto isStoryFinalBoss = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD;
                
                if (DataRepository::GetInstance().GetCurrentStoryMapNodeType() == StoryMap::NodeType::ELITE_ENCOUNTER || DataRepository::GetInstance().GetCurrentStoryMapNodeType() == StoryMap::NodeType::BOSS_ENCOUNTER)
                {
                    if (isTutorialMiniBoss)
                    {
                        DataRepository::GetInstance().SetCurrentWheelOfFortuneType(WheelOfFortuneType::TUTORIAL_BOSS);
                    }
                    else if (isStoryFinalBoss)
                    {
                        DataRepository::GetInstance().SetCurrentWheelOfFortuneType(WheelOfFortuneType::FINAL_BOSS);
                    }
                    else
                    {
                        DataRepository::GetInstance().SetCurrentWheelOfFortuneType(WheelOfFortuneType::ELITE);
                    }
                }
                
                if (isStoryFinalBoss)
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(STORY_VICTORY_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                }
                else
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(CARD_SELECTION_REWARD_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                }
                
                events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(WHEEL_OF_FORTUNE_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
            }
            else if (currentSubSceneType == BattleSubSceneType::CARD_SELECTION)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(CARD_SELECTION_REWARD_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
            }
            else if (currentSubSceneType == BattleSubSceneType::STORY_VICTORY)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(STORY_VICTORY_SCENE_NAME, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
            }
        }
    }
    
    return mPendingAnimations == 0 ? ActionAnimationUpdateResult::FINISHED : ActionAnimationUpdateResult::ONGOING;
}

///------------------------------------------------------------------------------------------------

bool BattleInitialSetupAndAnimationGameAction::VShouldBeSerialized() const
{
    return false;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::string>& BattleInitialSetupAndAnimationGameAction::VGetRequiredExtraParamNames() const
{
    return sRequiredExtraParamNames;
}

///------------------------------------------------------------------------------------------------
