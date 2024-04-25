///------------------------------------------------------------------------------------------------
///  AchievementManager.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 18/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Logging.h>
#include <game/AchievementManager.h>
#include <game/AnimatedButton.h>
#include <game/DataRepository.h>
#include <game/GameSymbolicGlyphNames.h>
#include <game/GuiObjectManager.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

static const strutils::StringId ACHIEVEMENT_UNLOCKED_BASE_SCENE_OBJECT_NAME = strutils::StringId("achievement_unlocked_base");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_TITLE_TEXT_SCENE_OBJECT_NAME = strutils::StringId("achievement_unlocked_title_text");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_ACHIEVEMENT_TITLE_TEXT_SCENE_OBJECT_NAME = strutils::StringId("achievement_unlocked_achievement_title_text");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_ACHIEVEMENT_FRAME_SCENE_OBJECT_NAME = strutils::StringId("achievement_unlocked_frame");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_PORTRAIT_SCENE_OBJECT_NAME = strutils::StringId("achievement_unlocked_portrait");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_BOUNTY_TEXT_SCENE_OBJECT_NAME = strutils::StringId("achievement_unlocked_bounty_text");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_CONTINUE_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("continue_button");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_UNIFORM_NAME = strutils::StringId("achievement_unlocked");
static const strutils::StringId ACHIEVEMENT_PORTRAIT_LIGHT_RAY_ANIMATION = strutils::StringId("portrait_light_ray_animation");
static const strutils::StringId ACHIEVEMENT_FRAME_LIGHT_RAY_ANIMATION = strutils::StringId("frame_light_ray_animation");
static const strutils::StringId ACHIEVEMENT_DESCRIPTION_TEXT_SCENE_OBJECT_NAMES[] =
{
    strutils::StringId("achievement_unlocked_description_text_0"),
    strutils::StringId("achievement_unlocked_description_text_1"),
    strutils::StringId("achievement_unlocked_description_text_2"),
    strutils::StringId("achievement_unlocked_description_text_3")
};

static const std::string ACHIEVEMENT_BASE_TEXTURE_FILE_NAME = "achievement_unlocked.png";
static const std::string ACHIEVEMENT_FRAME_TEXTURE_FILE_NAME = "achievement_frame.png";
static const std::string ACHIEVEMENT_PORTRAIT_SHADER_FILE_NAME = "achievement_portrait.vs";
static const std::string FIREWORKS_SFX = "sfx_fireworks";
static const std::string VICTORY_SFX = "sfx_victory";

static const glm::vec3 ACHIEVEMENT_BASE_INIT_POSITION = {-0.016f, 0.4f, 23.5f};
static const glm::vec3 ACHIEVEMENT_BASE_END_POSITION = {-0.016f, 0.115f, 23.5f};
static const glm::vec3 ACHIEVEMENT_TEXT_SCALE = {0.0003f, 0.0003f, 0.0003f};
static const glm::vec3 ACHIEVEMENT_DESCRIPTION_TEXT_SCALE = {0.0003f, 0.0003f, 0.0003f};
static const glm::vec3 ACHIEVEMENT_BASE_SCALE = {0.6f, 0.5f, 0.4f};
static const glm::vec3 ACHIEVEMENT_FRAME_SCALE = {0.1f, 0.1f, 0.1f};
static const glm::vec3 ACHIEVEMENT_PORTRAIT_SCALE = {0.08f, 0.08f, 0.08f};
static const glm::vec3 ACHIEVEMENT_FRAME_OFFSET = {-0.166f, -0.02f, 0.1f};
static const glm::vec3 ACHIEVEMENT_PORTRAIT_OFFSET = {-0.166f, -0.02f, 0.05f};
static const glm::vec3 ACHIEVEMENT_BOUNTY_TEXT_OFFSET = {-0.232f, -0.081f, 0.1f};
static const glm::vec3 ACHIEVEMENT_CONTINUE_BUTTON_OFFSET = {0.14f, -0.081f, 0.1f};
static const glm::vec3 ACHIEVEMENT_BOUNTY_SPAWN_OFFSET = {-0.015f, -0.02f, 0.0f};
static const glm::vec3 ACHIEVEMENT_TITLE_TEXT_OFFSET = {-0.118f, 0.094f, 0.1f};
static const glm::vec3 ACHIEVEMENT_UNLOCKED_TITLE_OFFSET = {-0.232f, 0.055f, 0.1f};
static const glm::vec3 ACHIEVEMENT_TEXT_OFFSETS[] =
{
    {  0.0f, 0.055, 0.1f },
    {  0.0f, 0.021f, 0.1f },
    {  0.0f, -0.013f, 0.1f },
    {  0.0f, -0.047f, 0.1f }
};

static const float ACHIEVEMENT_SWIPE_IN_OUT_DURATION_SECS = 1.0f;

///------------------------------------------------------------------------------------------------

AchievementManager& AchievementManager::GetInstance()
{
    static AchievementManager instance;
    return instance;
}

///------------------------------------------------------------------------------------------------

AchievementManager::AchievementManager()
{
    events::EventSystem::GetInstance().RegisterForEvent<events::AchievementUnlockedTriggerEvent>(this, &AchievementManager::OnAchievementUnlockedTrigger);
}

///------------------------------------------------------------------------------------------------

AchievementManager::~AchievementManager()
{
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, AchievementDefinition, strutils::StringIdHasher>& AchievementManager::GetAchievementDefinitions() const
{
    return mAchievementDefinitions;
}

///------------------------------------------------------------------------------------------------

bool AchievementManager::HasAnyActiveAchievements() const
{
    return !mActiveAchievements.empty();
}

///------------------------------------------------------------------------------------------------

bool AchievementManager::IsAchievementActive(const strutils::StringId& achievementName) const
{
    return std::find_if(mActiveAchievements.cbegin(), mActiveAchievements.cend(), [&](const events::AchievementUnlockedTriggerEvent& event){ return event.mAchievementName == achievementName; }) != mActiveAchievements.cend();
}

///------------------------------------------------------------------------------------------------

void AchievementManager::LoadAchievementDefinitions()
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto achievementsDefinitionJsonResourceId = systemsEngine.GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + "achievement_definitions.json", resources::DONT_RELOAD);
    const auto achievementsJson =  nlohmann::json::parse(systemsEngine.GetResourceLoadingService().GetResource<resources::DataFileResource>(achievementsDefinitionJsonResourceId).GetContents());
    
    for (const auto& achievementDefinitionObject: achievementsJson["achievement_definitions"])
    {
        strutils::StringId achievementName = strutils::StringId(achievementDefinitionObject["name"].get<std::string>());
        std::string achievementTitle = achievementDefinitionObject["title"].get<std::string>();
        std::string achievementDescription = achievementDefinitionObject["description"].get<std::string>();
        std::string achievementTextureFileName = achievementDefinitionObject["texture"].get<std::string>();
        long long bounty = achievementDefinitionObject["bounty"].get<long long>();
        
        mAchievementDefinitions.emplace(std::make_pair(achievementName, AchievementDefinition(achievementName, achievementTitle, achievementDescription, achievementTextureFileName, bounty)));
    }
}

///------------------------------------------------------------------------------------------------

void AchievementManager::Update(const float dtMillis, std::shared_ptr<GuiObjectManager> activeGuiObjectManager)
{
    if (!mActiveAchievements.empty())
    {
        // Achievement active but not created yet. Create it.
        if (mAchievementSceneObjects.empty())
        {
            CreateAchievement();
        }
        // Achievement active and created. Update it
        else
        {
            UpdateActiveAchievement(dtMillis, activeGuiObjectManager);
        }
    }
}

///------------------------------------------------------------------------------------------------

void AchievementManager::CreateAchievement()
{
    mLastGuiObjectManager = nullptr;
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(VICTORY_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(FIREWORKS_SFX);
    
    const auto& achievementDefinition = mAchievementDefinitions.at(mActiveAchievements.front().mAchievementName);
    
    // Add achievement to be surfaced to perm. unlocked achievements
    auto unlockedAchievements = DataRepository::GetInstance().GetUnlockedAchievements();
    unlockedAchievements.push_back(mActiveAchievements.front().mAchievementName);
    DataRepository::GetInstance().SetUnlockedAchievements(unlockedAchievements);
    DataRepository::GetInstance().FlushStateToFile();

    // Create custom scene
    auto unlockedAchievementScene = CoreSystemsEngine::GetInstance().GetSceneManager().CreateScene(game_constants::ACHIEVEMENT_UNLOCKED_SCENE);
    unlockedAchievementScene->SetLoaded(true);
    
    // Achievement Base
    auto achievementBaseSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_UNLOCKED_BASE_SCENE_OBJECT_NAME);
    achievementBaseSceneObject->mPosition = ACHIEVEMENT_BASE_INIT_POSITION;
    achievementBaseSceneObject->mScale = ACHIEVEMENT_BASE_SCALE;
    achievementBaseSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + ACHIEVEMENT_BASE_TEXTURE_FILE_NAME);
    achievementBaseSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    mAchievementSceneObjects.push_back(achievementBaseSceneObject);
    
    // Achievement Unlocked Text Title
    auto achievementUnlockedTitleTextSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_UNLOCKED_TITLE_TEXT_SCENE_OBJECT_NAME);
    achievementUnlockedTitleTextSceneObject->mScale = ACHIEVEMENT_TEXT_SCALE;
    achievementUnlockedTitleTextSceneObject->mPosition = achievementBaseSceneObject->mPosition;
    achievementUnlockedTitleTextSceneObject->mPosition += ACHIEVEMENT_TITLE_TEXT_OFFSET;
    achievementUnlockedTitleTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    
    scene::TextSceneObjectData achievementUnlockedTitletextData;
    achievementUnlockedTitletextData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
    achievementUnlockedTitletextData.mText = "Achievement Unlocked!";
    
    achievementUnlockedTitleTextSceneObject->mSceneObjectTypeData = std::move(achievementUnlockedTitletextData);
    mAchievementSceneObjects.push_back(achievementUnlockedTitleTextSceneObject);
    
    // Achievement Title
    auto achievementUnlockedAchievementTitleTextSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_UNLOCKED_ACHIEVEMENT_TITLE_TEXT_SCENE_OBJECT_NAME);
    achievementUnlockedAchievementTitleTextSceneObject->mScale = ACHIEVEMENT_TEXT_SCALE;
    achievementUnlockedAchievementTitleTextSceneObject->mPosition = achievementBaseSceneObject->mPosition;
    achievementUnlockedAchievementTitleTextSceneObject->mPosition += ACHIEVEMENT_UNLOCKED_TITLE_OFFSET;
    achievementUnlockedAchievementTitleTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    
    scene::TextSceneObjectData achievementTitleTextData;
    achievementTitleTextData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
    achievementTitleTextData.mText = achievementDefinition.mAchievementTitle;
    
    achievementUnlockedAchievementTitleTextSceneObject->mSceneObjectTypeData = std::move(achievementTitleTextData);
    mAchievementSceneObjects.push_back(achievementUnlockedAchievementTitleTextSceneObject);
    
    // Achievement Frame
    auto achievementFrameSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_UNLOCKED_ACHIEVEMENT_FRAME_SCENE_OBJECT_NAME);
    achievementFrameSceneObject->mPosition = achievementBaseSceneObject->mPosition;
    achievementFrameSceneObject->mPosition += ACHIEVEMENT_FRAME_OFFSET;
    achievementFrameSceneObject->mScale = ACHIEVEMENT_FRAME_SCALE;
    achievementFrameSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + ACHIEVEMENT_FRAME_TEXTURE_FILE_NAME);
    achievementFrameSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ACHIEVEMENT_PORTRAIT_SHADER_FILE_NAME);
    achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    achievementFrameSceneObject->mShaderBoolUniformValues[ACHIEVEMENT_UNLOCKED_UNIFORM_NAME] = true;
    mAchievementSceneObjects.push_back(achievementFrameSceneObject);
    
    // Achievement Portrait
    auto achievementPortraitSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_UNLOCKED_PORTRAIT_SCENE_OBJECT_NAME);
    achievementPortraitSceneObject->mPosition = achievementBaseSceneObject->mPosition;
    achievementPortraitSceneObject->mPosition += ACHIEVEMENT_PORTRAIT_OFFSET;
    achievementPortraitSceneObject->mScale = ACHIEVEMENT_PORTRAIT_SCALE;
    achievementPortraitSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + achievementDefinition.mAchievementPortraitTextureFileName);
    achievementPortraitSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ACHIEVEMENT_PORTRAIT_SHADER_FILE_NAME);
    achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    achievementPortraitSceneObject->mShaderBoolUniformValues[ACHIEVEMENT_UNLOCKED_UNIFORM_NAME] = true;
    mAchievementSceneObjects.push_back(achievementPortraitSceneObject);
    
    // Achievement Bounty Text Title
    auto achievementBountyTextSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_UNLOCKED_BOUNTY_TEXT_SCENE_OBJECT_NAME);
    achievementBountyTextSceneObject->mScale = ACHIEVEMENT_TEXT_SCALE;
    achievementBountyTextSceneObject->mPosition = achievementBaseSceneObject->mPosition;
    achievementBountyTextSceneObject->mPosition += ACHIEVEMENT_BOUNTY_TEXT_OFFSET;
    achievementBountyTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    
    scene::TextSceneObjectData achievementBountyTextData;
    achievementBountyTextData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
    achievementBountyTextData.mText = "Bounty " + std::to_string(achievementDefinition.mAchievementBountyReward) + symbolic_glyph_names::SYMBOLIC_NAMES.at(symbolic_glyph_names::COIN);
    
    achievementBountyTextSceneObject->mSceneObjectTypeData = std::move(achievementBountyTextData);
    mAchievementSceneObjects.push_back(achievementBountyTextSceneObject);
    
    // Achievement Description
    auto achievementDescriptionRows = strutils::StringSplit(achievementDefinition.mAchievementDescription, '$');
    for (auto i = 0U; i < achievementDescriptionRows.size(); ++i)
    {
        auto achievementTextSceneObject = unlockedAchievementScene->CreateSceneObject(ACHIEVEMENT_DESCRIPTION_TEXT_SCENE_OBJECT_NAMES[i]);
        achievementTextSceneObject->mScale = ACHIEVEMENT_DESCRIPTION_TEXT_SCALE;
        achievementTextSceneObject->mPosition = achievementBaseSceneObject->mPosition;
        achievementTextSceneObject->mPosition += ACHIEVEMENT_TEXT_OFFSETS[i];
        achievementTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        
        scene::TextSceneObjectData textData;
        textData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
        textData.mText = achievementDescriptionRows[i];
        
        for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
        {
            strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), textData.mText);
        }
        
        achievementTextSceneObject->mSceneObjectTypeData = std::move(textData);
        mAchievementSceneObjects.push_back(achievementTextSceneObject);
    }
    
    mContinueButton = std::make_unique<AnimatedButton>
    (
        achievementBaseSceneObject->mPosition + ACHIEVEMENT_CONTINUE_BUTTON_OFFSET,
        ACHIEVEMENT_TEXT_SCALE,
        game_constants::DEFAULT_FONT_BLACK_NAME,
        "Continue",
        ACHIEVEMENT_UNLOCKED_CONTINUE_BUTTON_SCENE_OBJECT_NAME,
        [=]()
        {
            if (mLastGuiObjectManager)
            {
                mLastGuiObjectManager->StopRewardAnimation();
                mLastGuiObjectManager->ResetDisplayedCurrencyCoins();
            }
            
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            animationManager.StartAnimation(std::make_unique<rendering::TimeDelayAnimation>(ACHIEVEMENT_SWIPE_IN_OUT_DURATION_SECS), [=]()
            {
                DestroyAchievement();
            });
            SwipeOutAchievement();
        },
        *unlockedAchievementScene
    );
    mAchievementSceneObjects.push_back(mContinueButton->GetSceneObject());
    
    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(FIREWORKS_SFX);
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(mAchievementSceneObjects, ACHIEVEMENT_BASE_END_POSITION, mAchievementSceneObjects.front()->mScale, ACHIEVEMENT_SWIPE_IN_OUT_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=]()
    {
        CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(VICTORY_SFX);
        const auto& achievementDefinition = mAchievementDefinitions.at(mActiveAchievements.front().mAchievementName);
        
        auto goldCoinSourcePosition = achievementFrameSceneObject->mPosition + ACHIEVEMENT_BOUNTY_SPAWN_OFFSET;
        if (CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE))
        {
            goldCoinSourcePosition.x /= 2.0f;
            goldCoinSourcePosition.y /= 2.0f;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(ACHIEVEMENT_FRAME_LIGHT_RAY_ANIMATION);
        CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(ACHIEVEMENT_PORTRAIT_LIGHT_RAY_ANIMATION);
        
        auto achievementFrameSceneObject = unlockedAchievementScene->FindSceneObject(ACHIEVEMENT_UNLOCKED_ACHIEVEMENT_FRAME_SCENE_OBJECT_NAME);
        auto achievementPortraitSceneObject = unlockedAchievementScene->FindSceneObject(ACHIEVEMENT_UNLOCKED_PORTRAIT_SCENE_OBJECT_NAME);
        
        achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
        achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME], game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f), [](){}, ACHIEVEMENT_FRAME_LIGHT_RAY_ANIMATION);
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME], game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f), [](){}, ACHIEVEMENT_PORTRAIT_LIGHT_RAY_ANIMATION);
        
        events::EventSystem::GetInstance().DispatchEvent<events::CoinRewardEvent>(static_cast<int>(achievementDefinition.mAchievementBountyReward), goldCoinSourcePosition);
        DataRepository::GetInstance().FlushStateToFile();
    });
}

///------------------------------------------------------------------------------------------------

void AchievementManager::SwipeOutAchievement()
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(mAchievementSceneObjects, ACHIEVEMENT_BASE_INIT_POSITION, mAchievementSceneObjects.front()->mScale, ACHIEVEMENT_SWIPE_IN_OUT_DURATION_SECS, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=]()
    {
        for (auto sceneObject: mAchievementSceneObjects)
        {
            sceneObject->mInvisible = true;
        }
    });
}

///------------------------------------------------------------------------------------------------

void AchievementManager::DestroyAchievement()
{
    CoreSystemsEngine::GetInstance().GetSceneManager().RemoveScene(game_constants::ACHIEVEMENT_UNLOCKED_SCENE);
    mActiveAchievements.erase(mActiveAchievements.begin());
    mAchievementSceneObjects.clear();
    mContinueButton = nullptr;
    mLastGuiObjectManager = nullptr;
}

///------------------------------------------------------------------------------------------------

void AchievementManager::UpdateActiveAchievement(const float dtMillis, std::shared_ptr<GuiObjectManager> activeGuiObjectManager)
{
    if (!mActiveAchievements.empty())
    {
        if (activeGuiObjectManager)
        {
            mLastGuiObjectManager = activeGuiObjectManager;
            activeGuiObjectManager->Update(dtMillis, false);
        }
        
        // Continue button interaction
        mContinueButton->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void AchievementManager::OnAchievementUnlockedTrigger(const events::AchievementUnlockedTriggerEvent& event)
{
    // Achievement unlocked already
    const auto& unlockedAchievements = DataRepository::GetInstance().GetUnlockedAchievements();
    if (std::find(unlockedAchievements.cbegin(), unlockedAchievements.cend(), event.mAchievementName) != unlockedAchievements.cend())
    {
        return;
    }
    
    // Achievement already queued up
    if (IsAchievementActive(event.mAchievementName))
    {
        return;
    }
    
    // Achievement definition not found
    if (mAchievementDefinitions.count(event.mAchievementName) == 0)
    {
        logging::Log(logging::LogType::ERROR, "Tried to surface unknown achievement %s", event.mAchievementName.GetString().c_str());
        assert(false);
        return;
    }
    
    mActiveAchievements.push_back(event);
}

///------------------------------------------------------------------------------------------------
