///------------------------------------------------------------------------------------------------
///  VictorySceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/02/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/VictorySceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId VICTORY_SCENE_NAME = strutils::StringId("victory_scene");
static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId BACK_TO_MAIN_MENU_BUTTON_NAME = strutils::StringId("back_to_main_menu_button");
static const strutils::StringId VICTORY_TEXT_SCENE_OBJECT_NAME = strutils::StringId("victory_text");

static const strutils::StringId VICTORY_INTRO_TEXT_TOP_NAME = strutils::StringId("victory_intro_text_top");
static const strutils::StringId VICTORY_INTRO_TEXT_MID_NAME = strutils::StringId("victory_intro_text_mid");
static const strutils::StringId VICTORY_INTRO_TEXT_BOT_NAME = strutils::StringId("victory_intro_text_bot");
static const strutils::StringId VICTORY_RESULTS_TEXT_TOP_NAME = strutils::StringId("victory_results_text_top");
static const strutils::StringId VICTORY_RESULTS_TEXT_MID_NAME = strutils::StringId("victory_results_text_mid");
static const strutils::StringId VICTORY_RESULTS_TEXT_BOT_NAME = strutils::StringId("victory_results_text_bot");

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.081f, -0.131f, 23.1f};
static const glm::vec3 BACK_TO_MAIN_MENU_BUTTON_POSITION = {-0.142f, -0.083f, 23.1f};
static const glm::vec3 VICTORY_INTRO_TEXT_TOP_POSITION = {-0.25f, 0.07f, 23.1f};
static const glm::vec3 VICTORY_INTRO_TEXT_MID_POSITION = {-0.292f, 0.019f, 23.1f};
static const glm::vec3 VICTORY_INTRO_TEXT_BOT_POSITION = {-0.302f, -0.031f, 23.1f};
static const glm::vec3 VICTORY_RESULTS_TEXT_TOP_POSITION = {-0.123f, 0.109f, 23.1f};
static const glm::vec3 VICTORY_RESULTS_TEXT_MID_POSITION = {-0.191f, 0.058f, 23.1f};
static const glm::vec3 VICTORY_RESULTS_TEXT_BOT_POSITION = {-0.191f, 0.007f, 23.1f};
static const glm::vec3 VICTORY_RESULTS_COINS_DIFFERENCE_POSITIVE_COLOR = {0.0f, 0.7f, 0.0f};
static const glm::vec3 VICTORY_RESULTS_COINS_DIFFERENCE_NEGATIVE_COLOR = {0.8f, 0.0f, 0.0f};
static const glm::vec3 VICTORY_RESULTS_COINS_DIFFERENCE_NEUTRAL_COLOR = {1.0f, 1.0f, 1.0f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;
static const float INITIAL_SURFACING_DELAY_SECS = 1.0f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    VICTORY_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    VICTORY_TEXT_SCENE_OBJECT_NAME,
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& VictorySceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

VictorySceneLogicManager::VictorySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

VictorySceneLogicManager::~VictorySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void VictorySceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void VictorySceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::STORY_VICTORY);
    DataRepository::GetInstance().FlushStateToFile();
    
    scene->FindSceneObject(VICTORY_TEXT_SCENE_OBJECT_NAME)->mInvisible = true;
    
    mActiveSubScene = SubSceneType::NONE;
    mTransitioningToSubScene = false;
    mInitialSurfacingHappened = false;
    mInitialSurfacingDelaySecs = INITIAL_SURFACING_DELAY_SECS;
}

///------------------------------------------------------------------------------------------------

void VictorySceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    if (mTransitioningToSubScene)
    {
        return;
    }
    
    if (!mInitialSurfacingHappened)
    {
        mInitialSurfacingDelaySecs -= dtMillis/1000.0f;
        if (mInitialSurfacingDelaySecs <= 0.0f)
        {
            scene->FindSceneObject(VICTORY_TEXT_SCENE_OBJECT_NAME)->mInvisible = false;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(VICTORY_TEXT_SCENE_OBJECT_NAME), 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=](){});
            
            InitSubScene(SubSceneType::INTRO, scene);
            mInitialSurfacingHappened = true;
        }
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void VictorySceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    for (auto sceneObject: scene->GetSceneObjects())
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
        });
    }
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> VictorySceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void VictorySceneLogicManager::InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    if (mActiveSubScene == subSceneType)
    {
        return;
    }
    
    mActiveSubScene = subSceneType;
    
    scene->RemoveAllSceneObjectsButTheOnesNamed(STATIC_SCENE_ELEMENTS);
    mAnimatedButtons.clear();
    
    switch (subSceneType)
    {
        case SubSceneType::INTRO:
        {
            scene::TextSceneObjectData textDataVictoryIntroTop;
            textDataVictoryIntroTop.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataVictoryIntroTop.mText = "Congratulations on your victory!";
            auto textVictoryIntroTopSceneObject = scene->CreateSceneObject(VICTORY_INTRO_TEXT_TOP_NAME);
            textVictoryIntroTopSceneObject->mSceneObjectTypeData = std::move(textDataVictoryIntroTop);
            textVictoryIntroTopSceneObject->mPosition = VICTORY_INTRO_TEXT_TOP_POSITION;
            textVictoryIntroTopSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataVictoryIntroMid;
            textDataVictoryIntroMid.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataVictoryIntroMid.mText = "For an extra challenge on your next story";
            auto textVictoryIntroMidSceneObject = scene->CreateSceneObject(VICTORY_INTRO_TEXT_MID_NAME);
            textVictoryIntroMidSceneObject->mSceneObjectTypeData = std::move(textDataVictoryIntroMid);
            textVictoryIntroMidSceneObject->mPosition = VICTORY_INTRO_TEXT_MID_POSITION;
            textVictoryIntroMidSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataVictoryIntroBot;
            textDataVictoryIntroBot.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            if (DataRepository::GetInstance().GetCurrentStoryMutationLevel() > 0)
            {
                textDataVictoryIntroBot.mText = "attempt you can try adding more Mutations!";
            }
            else
            {
                textDataVictoryIntroBot.mText = "attempt you can try out adding Mutations!";
            }
            
            auto textVictoryIntroBotSceneObject = scene->CreateSceneObject(VICTORY_INTRO_TEXT_BOT_NAME);
            textVictoryIntroBotSceneObject->mSceneObjectTypeData = std::move(textDataVictoryIntroBot);
            textVictoryIntroBotSceneObject->mPosition = VICTORY_INTRO_TEXT_BOT_POSITION;
            textVictoryIntroBotSceneObject->mScale = BUTTON_SCALE;
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                CONTINUE_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Continue",
                CONTINUE_BUTTON_NAME,
                [=]() { TransitionToSubScene(SubSceneType::RESULTS, scene); },
                *scene
            ));
        } break;
            
        case SubSceneType::RESULTS:
        {
            scene::TextSceneObjectData textDataVictoryResultsTop;
            textDataVictoryResultsTop.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataVictoryResultsTop.mText = "Mutation level: " + std::to_string(DataRepository::GetInstance().GetCurrentStoryMutationLevel());
            auto textVictoryResultsTopSceneObject = scene->CreateSceneObject(VICTORY_RESULTS_TEXT_TOP_NAME);
            textVictoryResultsTopSceneObject->mSceneObjectTypeData = std::move(textDataVictoryResultsTop);
            textVictoryResultsTopSceneObject->mPosition = VICTORY_RESULTS_TEXT_TOP_POSITION;
            textVictoryResultsTopSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataVictoryResultsMid;
            textDataVictoryResultsMid.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            auto timePreformatted = strutils::GetHoursMinutesSecondsStringFromSeconds(DataRepository::GetInstance().GetCurrentStorySecondsPlayed());
            auto timeComponents = strutils::StringSplit(timePreformatted, ':');
            textDataVictoryResultsMid.mText = "Time played: " + timeComponents[0] + "h " + timeComponents[1] + "m " + timeComponents[2] + "s";
            
            auto textVictoryResultMidSceneObject = scene->CreateSceneObject(VICTORY_RESULTS_TEXT_MID_NAME);
            textVictoryResultMidSceneObject->mSceneObjectTypeData = std::move(textDataVictoryResultsMid);
            textVictoryResultMidSceneObject->mPosition = VICTORY_RESULTS_TEXT_MID_POSITION;
            textVictoryResultMidSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataVictoryResultsBot;
            textDataVictoryResultsBot.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataVictoryResultsBot.mText = "Gold Coin Difference: ";
            
            auto coinDifference = DataRepository::GetInstance().CurrencyCoins().GetValue() - DataRepository::GetInstance().GetStoryStartingGold();
            auto coinDifferenceString = std::to_string(coinDifference);
            
            auto textVictoryResultsBotSceneObject = scene->CreateSceneObject(VICTORY_RESULTS_TEXT_BOT_NAME);
            textVictoryResultsBotSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::BASIC_CUSTOM_COLOR_SHADER_FILE_NAME);
            
            textVictoryResultsBotSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = VICTORY_RESULTS_COINS_DIFFERENCE_NEUTRAL_COLOR;
            if (coinDifference != 0 && coinDifference > 0)
            {
                coinDifferenceString = "+" + coinDifferenceString;
                textVictoryResultsBotSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = VICTORY_RESULTS_COINS_DIFFERENCE_POSITIVE_COLOR;
            }
            else if (coinDifference != 0 && coinDifference < 0)
            {
                textVictoryResultsBotSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = VICTORY_RESULTS_COINS_DIFFERENCE_NEGATIVE_COLOR;
            }
            textDataVictoryResultsBot.mText += coinDifferenceString;
            textVictoryResultsBotSceneObject->mSceneObjectTypeData = std::move(textDataVictoryResultsBot);
            textVictoryResultsBotSceneObject->mPosition = VICTORY_RESULTS_TEXT_BOT_POSITION;
            textVictoryResultsBotSceneObject->mScale = BUTTON_SCALE;
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                BACK_TO_MAIN_MENU_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Back to Main Menu",
                BACK_TO_MAIN_MENU_BUTTON_NAME,
                [=]()
                {
                    auto currentMutationLevel = DataRepository::GetInstance().GetCurrentStoryMutationLevel();
                    auto currentMutationLevelVictories = DataRepository::GetInstance().GetMutationLevelVictories(currentMutationLevel);
                    DataRepository::GetInstance().SetMutationLevelVictories(currentMutationLevel, currentMutationLevelVictories + 1);
                    
                    auto currentMutationLevelBestTime = DataRepository::GetInstance().GetMutationLevelBestTime(currentMutationLevel);
                    DataRepository::GetInstance().SetMutationLevelBestTime(currentMutationLevel, math::Min(DataRepository::GetInstance().GetCurrentStorySecondsPlayed(), currentMutationLevelBestTime));
                    
                    DataRepository::GetInstance().ResetStoryData();
                    
                    if (DataRepository::GetInstance().GetGamesFinishedCount() == 0)
                    {
                        DataRepository::GetInstance().AddPendingCardPack(CardPackType::NORMAL);
                    }
                    
                    DataRepository::GetInstance().SetGamesFinishedCount(DataRepository::GetInstance().GetGamesFinishedCount() + 1);
                    
                    DataRepository::GetInstance().FlushStateToFile();
                    
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::MAIN_MENU_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
                },
                *scene
            ));
        } break;
            
        default: break;
    }
    
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        sceneObject->mInvisible = false;
        
        if (!STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
            mTransitioningToSubScene = false;
        });
    }
}

///------------------------------------------------------------------------------------------------

void VictorySceneLogicManager::TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    mTransitioningToSubScene = true;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            InitSubScene(subSceneType, scene);
        });
    }
}

///------------------------------------------------------------------------------------------------
