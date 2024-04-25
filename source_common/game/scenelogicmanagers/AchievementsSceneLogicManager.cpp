///------------------------------------------------------------------------------------------------
///  AchievementsSceneLogicManager.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 19/03/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/SceneObjectUtils.h>
#include <game/AchievementManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/CardTooltipController.h>
#include <game/CardUtils.h>
#include <game/GameSceneTransitionManager.h>
#include <game/GameSymbolicGlyphNames.h>
#include <game/GuiObjectManager.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/AchievementsSceneLogicManager.h>
#include <game/ProductRepository.h>

///------------------------------------------------------------------------------------------------

static const std::string ACHIEVEMENT_ENTRY_SHADER = "achievement_container_entry.vs";
static const std::string ACHIEVEMENT_TEXT_ENTRY_SHADER = "achievement_text_container_entry.vs";
static const std::string ACHIEVEMENT_FRAME_TEXTURE_FILE_NAME = "achievement_frame.png";

static const strutils::StringId BACK_BUTTON_NAME = strutils::StringId("back_button");
static const strutils::StringId ACHIEVEMENT_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("achievement_container");
static const strutils::StringId ACHIEVEMENT_UNLOCKED_UNIFORM_NAME = strutils::StringId("achievement_unlocked");

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 BACK_BUTTON_POSITION = {0.0f, -0.2f, 23.2f};
static const glm::vec3 ACHIEVEMENT_ENTRY_SCALE = glm::vec3(0.2512f/3.0f, 0.2512f/3.0f, 3.0f);
static const glm::vec3 ACHIEVEMENT_TOOLTIP_POSITION_OFFSET = {0.0f, 0.1f, 1.0f};
static const glm::vec3 ACHIEVEMENT_TOOLTIP_BASE_SCALE = {0.3f, 0.274f, 1/10.0f};
static const glm::vec3 ACHIEVEMENT_CONTAINER_ITEM_ENTRY_SCALE = {0.193f, 0.2f, 2.0f};
static const glm::vec3 ACHIEVEMENT_TEXT_SCALE = {0.00025f, 0.00025f, 0.00025f};
static const glm::vec3 ACHIEVEMENT_FRAME_OFFSET = {0.0f, 0.0f, 0.1f};
static const glm::vec3 ACHIEVEMENT_PORTRAIT_OFFSET = {0.0f, 0.0f, 0.05f};
static const glm::vec3 ACHIEVEMENT_BOUNTY_TEXT_OFFSET = {-0.054f, -0.061f, 0.1f};
static const glm::vec3 ACHIEVEMENT_NAME_OFFSET = {0.01f, 0.065f, 0.1f};

static const glm::vec2 ACHIEVEMENT_ENTRY_CUTOFF_VALUES = {-0.185f, 0.183f};
static const glm::vec2 ACHIEVEMENT_CONTAINER_CUTOFF_VALUES = {0.076, 0.093f};

static const math::Rectangle ACHIEVEMENT_CONTAINER_BOUNDS = {{-0.305f, -0.250f}, {0.305f, 0.182f}};

static const float ITEMS_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.05f;
static const float BACK_BUTTON_SNAP_TO_EDGE_FACTOR = 950000.0f;
static const float ITEM_ENTRY_Z = 23.2f;

static const int ACHIEVEMENT_CONTAINER_MIN_ENTRIES_TO_ANIMATE = 7;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::ACHIEVEMENTS_SCENE
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& AchievementsSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

AchievementsSceneLogicManager::AchievementsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

AchievementsSceneLogicManager::~AchievementsSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mScene = scene;

    mAchievementsContainer = nullptr;
    mSelectedAchievementIndex = -1;
    mToolTipIndex = -1;
    mToolTipPointeePosY = 0.0f;
    mToolTipPointeePosX = 0.0f;
    mLightRayPositionX = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
    mAnimatedButtons.clear();
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        BACK_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Back",
        BACK_BUTTON_NAME,
        [=]()
        {
            events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            mTransitioning = true;
        },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        BACK_BUTTON_SNAP_TO_EDGE_FACTOR
    ));
    mAnimatedButtons.back()->GetSceneObject()->mInvisible = false;
    mAnimatedButtons.back()->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    CreateAchievementEntriesAndContainer();
    
    // Staggered Item Presentation
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            continue;
        }

        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, ITEMS_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=](){});
    }
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mLightRayPositionX, game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f, animation_flags::NONE, 2.0f), [](){}, game_constants::GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME);
    events::EventSystem::GetInstance().RegisterForEvent<events::WindowResizeEvent>(this, &AchievementsSceneLogicManager::OnWindowResize);
    mTransitioning = false;
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    if (mTransitioning)
    {
        return;
    }
    
    UpdateAchievementContainer(dtMillis);
    
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
    
    if (mAchievementTooltipController)
    {
        mAchievementTooltipController->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    DestroyAchievementTooltip();
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, ITEMS_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            scene->RemoveSceneObject(sceneObject->mName);
        });
    }
    
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    
    animationManager.StopAnimation(game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    animationManager.StopAnimation(game_constants::GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(previousScene->GetUpdateTimeSpeedFactor(), 1.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> AchievementsSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::UpdateAchievementContainer(const float dtMillis)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    for (auto i = 0; i < mAchievementsContainer->GetItems().size(); ++i)
    {
        for (auto& sceneObject: mAchievementsContainer->GetItems()[i].mSceneObjects)
        {
            sceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time + i;
            sceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = mLightRayPositionX;
        }
    }
    
    if (mAchievementsContainer)
    {
        const auto& unlockedAchievements = DataRepository::GetInstance().GetUnlockedAchievements();
        const auto& achievementContainerUpdateResult = mAchievementsContainer->Update(dtMillis);
        
        if (achievementContainerUpdateResult.mInteractionType == InteractionType::INTERACTED_WITH_ELEMENTS)
        {
            if (mToolTipIndex != achievementContainerUpdateResult.mInteractedElementIndex)
            {
                mToolTipIndex = achievementContainerUpdateResult.mInteractedElementIndex;
                auto interactedElementEntry = mAchievementsContainer->GetItems()[achievementContainerUpdateResult.mInteractedElementIndex];
                
                DestroyAchievementTooltip();
                
                mToolTipPointeePosY = interactedElementEntry.mSceneObjects.front()->mPosition.y;
                mToolTipPointeePosX = interactedElementEntry.mSceneObjects.front()->mPosition.x;
                
                auto achievementDescription = AchievementManager::GetInstance().GetAchievementDefinitions().at(interactedElementEntry.mAchievementName).mAchievementDescription;
                
                if (std::find(unlockedAchievements.cbegin(), unlockedAchievements.cend(), interactedElementEntry.mAchievementName) != unlockedAchievements.cend())
                {
                    CreateAchievementTooltip(interactedElementEntry.mSceneObjects.front()->mPosition, achievementDescription);
                }
            }
        }
        else if (achievementContainerUpdateResult.mInteractionType == InteractionType::INTERACTED_WITH_CONTAINER_AREA)
        {
            DestroyAchievementTooltip();
        }
        
        if (mToolTipIndex != -1)
        {
            auto interactedElementEntry = mAchievementsContainer->GetItems()[mToolTipIndex];
            if (math::Abs(interactedElementEntry.mSceneObjects.front()->mPosition.y - mToolTipPointeePosY) > 0.01f)
            {
                mToolTipIndex = -1;
                DestroyAchievementTooltip();
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::OnWindowResize(const events::WindowResizeEvent&)
{
    mScene->RecalculatePositionOfEdgeSnappingSceneObjects();
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::CreateAchievementEntriesAndContainer()
{
    // Clean up existing container
    bool containerExists = mAchievementsContainer != nullptr;
    if (containerExists)
    {
        for (const auto& containerItem: mAchievementsContainer->GetItems())
        {
            for (const auto& sceneObject: containerItem.mSceneObjects)
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
                mScene->RemoveSceneObject(sceneObject->mName);
            }
        }
        
        mAchievementsContainer = nullptr;
    }
    
    // Create achievement entries
    mAchievementsContainer = std::make_unique<SwipeableContainer<AchievementEntry>>
    (
        ContainerType::VERTICAL_MATRIX,
        ACHIEVEMENT_CONTAINER_ITEM_ENTRY_SCALE,
        ACHIEVEMENT_CONTAINER_BOUNDS,
        ACHIEVEMENT_CONTAINER_CUTOFF_VALUES,
        ACHIEVEMENT_CONTAINER_SCENE_OBJECT_NAME,
        ITEM_ENTRY_Z,
        *mScene,
        ACHIEVEMENT_CONTAINER_MIN_ENTRIES_TO_ANIMATE
    );
    
    const auto& unlockedAchievements = DataRepository::GetInstance().GetUnlockedAchievements();
    const auto& achievementEntries = AchievementManager::GetInstance().GetAchievementDefinitions();
    
    std::vector<AchievementDefinition> sortedAchievementDefinitions;
    for (const auto& achievementDefinitionEntry: achievementEntries)
    {
        sortedAchievementDefinitions.push_back(achievementDefinitionEntry.second);
    }
    std::sort(sortedAchievementDefinitions.begin(), sortedAchievementDefinitions.end(), [](const AchievementDefinition& lhs, const AchievementDefinition& rhs)
    {
        return lhs.mAchievementBountyReward < rhs.mAchievementBountyReward;
    });
    
    for (const auto& achievementDefinition: sortedAchievementDefinitions)
    {
        bool hasUnlockedAchievement = std::find(unlockedAchievements.cbegin(), unlockedAchievements.cend(), achievementDefinition.mAchievementName) != unlockedAchievements.cend();
        
        // Achievement Frame
        auto achievementFrameSceneObject = mScene->CreateSceneObject();
        achievementFrameSceneObject->mPosition += ACHIEVEMENT_FRAME_OFFSET;
        achievementFrameSceneObject->mScale = ACHIEVEMENT_ENTRY_SCALE;
        achievementFrameSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + ACHIEVEMENT_FRAME_TEXTURE_FILE_NAME);
        achievementFrameSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ACHIEVEMENT_ENTRY_SHADER);
        achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        achievementFrameSceneObject->mShaderBoolUniformValues[ACHIEVEMENT_UNLOCKED_UNIFORM_NAME] = hasUnlockedAchievement;
        achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.s;
        achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.t;
        achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = mLightRayPositionX;
        achievementFrameSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        // Achievement Portrait
        auto achievementPortraitSceneObject = mScene->CreateSceneObject();
        achievementPortraitSceneObject->mPosition += ACHIEVEMENT_PORTRAIT_OFFSET;
        achievementPortraitSceneObject->mScale = ACHIEVEMENT_ENTRY_SCALE * 0.8f;
        achievementPortraitSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + achievementDefinition.mAchievementPortraitTextureFileName);
        achievementPortraitSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ACHIEVEMENT_ENTRY_SHADER);
        achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        achievementPortraitSceneObject->mShaderBoolUniformValues[ACHIEVEMENT_UNLOCKED_UNIFORM_NAME] = hasUnlockedAchievement;
        achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.s;
        achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.t;
        achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = mLightRayPositionX;
        achievementPortraitSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        // Achievement Title
        auto achievementUnlockedAchievementTitleTextSceneObject = mScene->CreateSceneObject();
        achievementUnlockedAchievementTitleTextSceneObject->mScale = ACHIEVEMENT_TEXT_SCALE;
        achievementUnlockedAchievementTitleTextSceneObject->mPosition += ACHIEVEMENT_NAME_OFFSET;
        achievementUnlockedAchievementTitleTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ACHIEVEMENT_TEXT_ENTRY_SHADER);
        achievementUnlockedAchievementTitleTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.s;
        achievementUnlockedAchievementTitleTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.t;
        achievementUnlockedAchievementTitleTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        scene::TextSceneObjectData achievementTitleTextData;
        achievementTitleTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        achievementTitleTextData.mText = achievementDefinition.mAchievementTitle;
        
        achievementUnlockedAchievementTitleTextSceneObject->mSceneObjectTypeData = std::move(achievementTitleTextData);
        
        auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*achievementUnlockedAchievementTitleTextSceneObject);
        auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
        achievementUnlockedAchievementTitleTextSceneObject->mPosition.x -= textLength/2.0f;
        
        // Achievement Bounty Text Title
        auto achievementBountyTextSceneObject = mScene->CreateSceneObject();
        achievementBountyTextSceneObject->mScale = ACHIEVEMENT_TEXT_SCALE;
        achievementBountyTextSceneObject->mPosition += ACHIEVEMENT_BOUNTY_TEXT_OFFSET;
        achievementBountyTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        achievementBountyTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ACHIEVEMENT_TEXT_ENTRY_SHADER);
        achievementBountyTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.s;
        achievementBountyTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = ACHIEVEMENT_ENTRY_CUTOFF_VALUES.t;
        achievementBountyTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        
        scene::TextSceneObjectData achievementBountyTextData;
        achievementBountyTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        achievementBountyTextData.mText = "Bounty " + std::to_string(achievementDefinition.mAchievementBountyReward) + symbolic_glyph_names::SYMBOLIC_NAMES.at(symbolic_glyph_names::COIN);
        
        achievementBountyTextSceneObject->mSceneObjectTypeData = std::move(achievementBountyTextData);
        
        
        AchievementEntry achievementEntry;
        achievementEntry.mAchievementName = achievementDefinition.mAchievementName;
        achievementEntry.mSceneObjects.push_back(achievementFrameSceneObject);
        achievementEntry.mSceneObjects.push_back(achievementPortraitSceneObject);
        achievementEntry.mSceneObjects.push_back(achievementUnlockedAchievementTitleTextSceneObject);
        achievementEntry.mSceneObjects.push_back(achievementBountyTextSceneObject);
        
        mAchievementsContainer->AddItem(std::move(achievementEntry), EntryAdditionStrategy::ADD_ON_THE_BACK);
    }
    
    // If container doesn't exist the staggered fade in will happen automatically at the end of VInitScene
    if (containerExists)
    {
        // Staggered Item Presentation
        size_t sceneObjectIndex = 0;
        for (const auto& containerItems: mAchievementsContainer->GetItems())
        {
            for (auto& sceneObject: containerItems.mSceneObjects)
            {
                sceneObject->mInvisible = false;
                sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, ITEMS_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=](){});
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::CreateAchievementTooltip(const glm::vec3& achievementOriginPostion, const std::string& tooltipText)
{
    bool shouldBeHorFlipped = achievementOriginPostion.x > 0.0f;
    bool shouldBeVerFlipped = achievementOriginPostion.y > 0.0f;
    
    auto achievementTooltipText = tooltipText;
    for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
    {
        strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), achievementTooltipText);
    }
    
    mAchievementTooltipController = std::make_unique<CardTooltipController>
    (
        achievementOriginPostion + ACHIEVEMENT_TOOLTIP_POSITION_OFFSET,
        ACHIEVEMENT_TOOLTIP_BASE_SCALE,
        achievementTooltipText,
        false,
        shouldBeHorFlipped,
        shouldBeVerFlipped,
        *mScene
    );
}

///------------------------------------------------------------------------------------------------

void AchievementsSceneLogicManager::DestroyAchievementTooltip()
{
    if (mAchievementTooltipController)
    {
        for (auto sceneObject: mAchievementTooltipController->GetSceneObjects())
        {
            mScene->RemoveSceneObject(sceneObject->mName);
        }
    }
    
    mAchievementTooltipController = nullptr;
}

///------------------------------------------------------------------------------------------------
