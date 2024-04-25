///------------------------------------------------------------------------------------------------
///  StatsSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 08/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/DataRepository.h>
#include <game/events/EventSystem.h>
#include <game/scenelogicmanagers/StatsSceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const strutils::StringId CONTINUE_BUTTON_NAME = strutils::StringId("continue_button");
static const strutils::StringId STATS_TITLE_SCENE_OBJECT_NAME = strutils::StringId("stats_title");

static const std::string CREDITS_FILE_PATH = "credits/credits.txt";
static const std::string TEXT_ENTRY_SHADER_FILE_NAME = "text_container_entry.vs";

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.066f, -0.211f, 23.1f};
static const glm::vec3 TEXT_SCALE = glm::vec3(0.00029f, 0.00029f, 0.00029f);
static const glm::vec3 TOTAL_STATS_TEXT_SCALE = glm::vec3(0.0004f, 0.0004f, 0.0004f);
static const glm::vec3 TEXT_MUTATION_LEVEL_INIT_POSITION = {-0.3f, 0.16f, 23.2f};
static const glm::vec3 TEXT_VICTORIES_INIT_POSITION = {-0.075f, 0.16f, 23.2f};
static const glm::vec3 TEXT_BEST_TIME_INIT_POSITION = {0.1f, 0.16f, 23.2f};
static const glm::vec3 TOTAL_VICTORIES_POSITION = {0.0f, 0.12f, 23.2f};
static const glm::vec3 TOTAL_TIME_PLAYED_POSITION = {0.0f, 0.04f, 23.2f};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float TEXT_ROW_HEIGHT = 0.0325f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    strutils::StringId("stats_scene")
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& StatsSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

StatsSceneLogicManager::StatsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

StatsSceneLogicManager::~StatsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mTransitioning = false;
    mHasShownTotalStatsScreen = false;
    
    mAnimatedButtons.clear();
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        CONTINUE_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Continue",
        CONTINUE_BUTTON_NAME,
        [=]()
        {
            if (!mHasShownTotalStatsScreen)
            {
                FadeOutMutationVictoriesAndBestTimesScreen(scene);
                
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
                {
                    CreateTotalStatsScreen(scene);
                });
                
                mHasShownTotalStatsScreen = true;
            }
            else
            {
                mTransitioning = true;
                events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            }
        },
        *scene
    ));
    
    bool textElementsExist = !mTextSceneObjects.empty();
    if (textElementsExist)
    {
        for (const auto& sceneObject: mTextSceneObjects)
        {
            CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
            scene->RemoveSceneObject(sceneObject->mName);
        }
        
        mTextSceneObjects.clear();
    }
    
    CreateMutationVictoriesAndBestTimesScreen(scene);
}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    if (mTransitioning)
    {
        return;
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
            
            if (sceneObject->mName == CONTINUE_BUTTON_NAME)
            {
                scene->RemoveSceneObject(sceneObject->mName);
            }
        });
    }
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> StatsSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::FadeOutMutationVictoriesAndBestTimesScreen(std::shared_ptr<scene::Scene> scene)
{
    for (auto sceneObject: mTextSceneObjects)
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
            scene->RemoveSceneObject(sceneObject->mName);
        });
    }
}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::CreateMutationVictoriesAndBestTimesScreen(std::shared_ptr<scene::Scene> scene)
{
    auto lineCounter = 0;
    const auto& victoryCounts = DataRepository::GetInstance().GetAllMutationLevelVictoryCounts();
    const auto& bestTimes = DataRepository::GetInstance().GetAllMutationLevelBestTimes();
    
    for (int i = 0; i < victoryCounts.size(); ++i)
    {
        if (victoryCounts[i] == 0)
        {
            if (i == 0)
            {
                scene::TextSceneObjectData textData;
                textData.mFontName = game_constants::DEFAULT_FONT_NAME;
                textData.mText = "No Victories yet";
                
                auto textSceneObject = scene->CreateSceneObject(strutils::StringId("credits_text_mutation_level_" + std::to_string(lineCounter)));
                textSceneObject->mSceneObjectTypeData = std::move(textData);
                textSceneObject->mPosition = TEXT_MUTATION_LEVEL_INIT_POSITION;
                textSceneObject->mPosition.y -= i * TEXT_ROW_HEIGHT;
                textSceneObject->mScale = TEXT_SCALE;
                
                mTextSceneObjects.push_back(textSceneObject);
            }
            break;
        }
        
        // Mutation Level
        {
            scene::TextSceneObjectData textData;
            textData.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            if (i == 0)
            {
                textData.mText = "No Mutations";
            }
            else
            {
                textData.mText = "Mutation Level:  " + std::to_string(i);
            }
            
            auto textSceneObject = scene->CreateSceneObject(strutils::StringId("credits_text_mutation_level_" + std::to_string(lineCounter)));
            textSceneObject->mSceneObjectTypeData = std::move(textData);
            textSceneObject->mPosition = TEXT_MUTATION_LEVEL_INIT_POSITION;
            textSceneObject->mPosition.y -= i * TEXT_ROW_HEIGHT;
            textSceneObject->mScale = TEXT_SCALE;
            
            mTextSceneObjects.push_back(textSceneObject);
        }
        
        // Victories
        {
            scene::TextSceneObjectData textData;
            textData.mFontName = game_constants::DEFAULT_FONT_NAME;
            textData.mText = "Victories:  " + std::to_string(victoryCounts[i]);
            
            auto textSceneObject = scene->CreateSceneObject(strutils::StringId("credits_text_victories_" + std::to_string(lineCounter)));
            textSceneObject->mSceneObjectTypeData = std::move(textData);
            textSceneObject->mPosition = TEXT_VICTORIES_INIT_POSITION;
            textSceneObject->mPosition.y -= i * TEXT_ROW_HEIGHT;
            textSceneObject->mScale = TEXT_SCALE;
            
            mTextSceneObjects.push_back(textSceneObject);
        }
        
        // Best Time
        {
            auto timePreformatted = strutils::GetHoursMinutesSecondsStringFromSeconds(bestTimes[i]);
            auto timeComponents = strutils::StringSplit(timePreformatted, ':');
            
            scene::TextSceneObjectData textData;
            textData.mFontName = game_constants::DEFAULT_FONT_NAME;
            textData.mText = "Best Time:  " + timeComponents[0] + "h " + timeComponents[1] + "m " + timeComponents[2] + "s";
            
            auto textSceneObject = scene->CreateSceneObject(strutils::StringId("credits_text_best_time_" + std::to_string(lineCounter)));
            textSceneObject->mSceneObjectTypeData = std::move(textData);
            textSceneObject->mPosition = TEXT_BEST_TIME_INIT_POSITION;
            textSceneObject->mPosition.y -= i * TEXT_ROW_HEIGHT;
            textSceneObject->mScale = TEXT_SCALE;
            
            mTextSceneObjects.push_back(textSceneObject);
        }
        
        lineCounter++;
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
        });
    }
}

///------------------------------------------------------------------------------------------------

void StatsSceneLogicManager::CreateTotalStatsScreen(std::shared_ptr<scene::Scene> scene)
{
    // Total Victories
    {
        const auto& mutationVictories = DataRepository::GetInstance().GetAllMutationLevelVictoryCounts();
        auto victoriesCount = 0;
        for (const auto& mutationLevelVictoriesCount: mutationVictories)
        {
            victoriesCount += mutationLevelVictoriesCount;
        }
        
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_NAME;
        textData.mText = "Total Victories:  " + std::to_string(victoriesCount);
        
        auto textSceneObject = scene->CreateSceneObject(strutils::StringId("total_victories"));
        textSceneObject->mSceneObjectTypeData = std::move(textData);
        textSceneObject->mPosition = TOTAL_VICTORIES_POSITION;
        textSceneObject->mScale = TOTAL_STATS_TEXT_SCALE;
        
        auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*textSceneObject);
        auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
        textSceneObject->mPosition.x -= textLength/2.0f;
        
        mTextSceneObjects.push_back(textSceneObject);
    }
    
    // Total Time Played
    {
        auto timePreformatted = strutils::GetHoursMinutesSecondsStringFromSeconds(DataRepository::GetInstance().GetTotalSecondsPlayed());
        auto timeComponents = strutils::StringSplit(timePreformatted, ':');
        
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_NAME;
        textData.mText = "Total Time Played:  " + timeComponents[0] + "h " + timeComponents[1] + "m " + timeComponents[2] + "s";
        
        auto textSceneObject = scene->CreateSceneObject(strutils::StringId("total_time_played"));
        textSceneObject->mSceneObjectTypeData = std::move(textData);
        textSceneObject->mPosition = TOTAL_TIME_PLAYED_POSITION;
        textSceneObject->mScale = TOTAL_STATS_TEXT_SCALE;
        
        auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*textSceneObject);
        auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
        textSceneObject->mPosition.x -= textLength/2.0f;
        
        mTextSceneObjects.push_back(textSceneObject);
    }
    
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME || sceneObject->mName == STATS_TITLE_SCENE_OBJECT_NAME || sceneObject->mName == CONTINUE_BUTTON_NAME)
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
        });
    }
}

///------------------------------------------------------------------------------------------------
