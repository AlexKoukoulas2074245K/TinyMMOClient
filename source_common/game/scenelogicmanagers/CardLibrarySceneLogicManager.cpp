///------------------------------------------------------------------------------------------------
///  CardLibrarySceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 13/01/2024
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/ParticleManager.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <game/AchievementManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/CardTooltipController.h>
#include <game/CardUtils.h>
#include <game/GameSceneTransitionManager.h>
#include <game/GuiObjectManager.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/CardLibrarySceneLogicManager.h>
#include <game/TutorialManager.h>

///------------------------------------------------------------------------------------------------

static const std::string CARD_ENTRY_SHADER = "card_library_entry.vs";
static const std::string TITLE_STORY_CARDS = "Story Card Deck";
static const std::string TITLE_BROWSING_FOR_DELETION = "Select Card To Delete";
static const std::string TITLE_CARD_LIBRARY = "Card Library";
static const std::string DISSOLVE_SHADER_FILE_NAME = "card_dissolve.vs";
static const std::string DISSOLVE_TEXTURE_FILE_NAME = "dissolve.png";
static const std::string GOLDEN_CHECKBOX_FILLED_TEXTURE_FILE_NAME = "golden_checkbox_filled.png";
static const std::string CHECKBOX_EMPTY_TEXTURE_FILE_NAME = "checkbox_empty.png";
static const std::string CHECKBOX_FILLED_TEXTURE_FILE_NAME = "checkbox_filled.png";
static const std::string CARD_FAMILY_FILTER_ICON_SHADER_FILE_NAME = "card_family_stamp.vs";
static const std::string CARD_FAMILY_FILTER_ICON_MASK_TEXTURE_FILE_NAME = "trap_mask.png";
static const std::string NEW_CARD_INDICATOR_SHADER_FILE_NAME = "new_indicator.vs";
static const std::string GOLDEN_CARDS_COLLECTED_TEXT_SHADER_FILE_NAME = "basic_custom_color.vs";
static const std::string FAMILY_STAMP_MASK_TEXTURE_FILE_NAME = "trap_mask.png";
static const std::string CARD_FAMILY_STAMP_SHADER_FILE_NAME = "card_family_stamp_library_entry.vs";

static const strutils::StringId BACK_BUTTON_NAME = strutils::StringId("back_button");
static const strutils::StringId FILTERS_TEXT_SCENE_OBJECT_NAME = strutils::StringId("card_library_filters_text");
static const strutils::StringId CARD_COLLECTION_TEXT_SCENE_OBJECT_NAME = strutils::StringId("card_collection_text");
static const strutils::StringId NORMAL_CARD_COLLECTION_TEXT_SCENE_OBJECT_NAME = strutils::StringId("normal_card_collection_text");
static const strutils::StringId GOLDEN_CARD_COLLECTION_TEXT_SCENE_OBJECT_NAME = strutils::StringId("golden_card_collection_text");
static const strutils::StringId GOLDEN_CHECKBOX_TEXT_SCENE_OBJECT_NAME = strutils::StringId("golden_checkbox_text");
static const strutils::StringId GOLDEN_CHECKBOX_SCENE_OBJECT_NAME = strutils::StringId("golden_checkbox");
static const strutils::StringId STORY_CARDS_TITLE_SCENE_OBJECT_NAME = strutils::StringId("story_cards_title");
static const strutils::StringId CARD_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("card_container");
static const strutils::StringId CARD_DELETION_OVERLAY_SCENE_OBJECT_NAME = strutils::StringId("card_deletion_overlay");
static const strutils::StringId DELETE_CARD_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("delete_card_button");
static const strutils::StringId CANCEL_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("cancel_button");
static const strutils::StringId DISSOLVE_THRESHOLD_UNIFORM_NAME = strutils::StringId("dissolve_threshold");
static const strutils::StringId DISSOLVE_MAGNITUDE_UNIFORM_NAME = strutils::StringId("dissolve_magnitude");
static const strutils::StringId CARD_ORIGIN_X_UNIFORM_NAME = strutils::StringId("card_origin_x");
static const strutils::StringId CARD_ORIGIN_Y_UNIFORM_NAME = strutils::StringId("card_origin_y");
static const strutils::StringId CARD_DESELECTION_ANIMATION_NAME = strutils::StringId("card_deselection_animation");

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 CARD_COLLECTION_TEXT_SCALE = {0.0003f, 0.0003f, 0.0003f};
static const glm::vec3 DELETE_CARD_BUTTON_POSITION = {-0.225f, 0.05f, 23.9f};
static const glm::vec3 GOLDEN_CHECKBOX_TEXT_POSITION = {-0.26f, 0.05f, 23.9f};
static const glm::vec3 GOLDEN_CHECKBOX_POSITION = {-0.125f, 0.037f, 23.9f};
static const glm::vec3 BACK_BUTTON_POSITION = {0.0f, -0.2f, 23.2f};
static const glm::vec3 CANCEL_BUTTON_POSITION = {-0.231f, -0.05f, 23.9f};
static const glm::vec3 CARD_ENTRY_SCALE = glm::vec3(-0.273f, 0.2512f, 2.0f);
static const glm::vec3 CONTAINER_ITEM_ENTRY_SCALE = glm::vec3(0.124f, 0.212f, 2.0f);
static const glm::vec3 CARD_TOOLTIP_POSITION_OFFSET = {0.0f, 0.1f, 0.0f};
static const glm::vec3 CARD_TOOLTIP_BASE_SCALE = {0.3f, 0.274f, 1/10.0f};
static const glm::vec3 GOLDEN_CHECKBOX_TEXT_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 CHECKBOX_SCALE = {0.1f, 0.1f, 0.1f};
static const glm::vec3 FILTER_ICON_SCALE = {0.0769f, 0.0769f, 0.0769f};
static const glm::vec3 SELECTED_CARD_TARGET_POSITION = {0.0f, 0.0f, 26.5f};
static const glm::vec3 FILTERS_TEXT_POSITION = {0.0f, 0.176f, 23.2f};
static const glm::vec3 NEW_CARD_INDICATOR_SCALE = {0.00045f, 0.00045f, 0.00045f};
static const glm::vec3 NEW_CARD_INDICATOR_POSITION_OFFSET = {-0.036f, 0.018f, 0.1f};
static const glm::vec3 CARD_COLLECTION_TEXT_POSITION = {-0.3f, -0.216f, 23.2f};
static const glm::vec3 NORMAL_CARD_COLLECTION_TEXT_POSITION = {-0.098f, -0.216f, 23.2f};
static const glm::vec3 GOLDEN_CARD_COLLECTION_TEXT_POSITION = {0.066f, -0.216f, 23.2f};
static const glm::vec3 GOLDEN_CARDS_COLLECTED_TEXT_COLOR = {0.90f, 0.81f, 0.21f};

static const glm::vec2 CARD_ENTRY_CUTOFF_VALUES = {-0.193f, 0.173f};
static const glm::vec2 CARD_CONTAINER_CUTOFF_VALUES = {-0.085f, 0.065f};
static const glm::vec2 CARD_DISSOLVE_EFFECT_MAG_RANGE = {3.0f, 6.0f};

static const math::Rectangle CARD_CONTAINER_BOUNDS = {{-0.305f, -0.205f}, {0.305f, 0.165f}};

static const float ITEMS_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.05f;
static const float BACK_BUTTON_SNAP_TO_EDGE_FACTOR = 950000.0f;
static const float CARD_ENTRY_Z = 23.2f;
static const float SELECTED_CARD_ANIMATION_DURATION_SECS = 0.35f;
static const float NEW_CARD_INDICATOR_FADE_OUT_ANIMATION_DURATION_SECS = 0.1f;
static const float SELECTED_CARD_OVERLAY_MAX_ALPHA = 0.9f;
static const float SELECTED_CARD_SCALE_FACTOR = 1.0f;
static const float CARD_DISSOLVE_SPEED = 0.0005f;
static const float MAX_CARD_DISSOLVE_VALUE = 1.2f;
static const float ANIMATED_COIN_VALUE_DURATION_SECS = 1.5f;
static const float MAX_SWIPE_DISTANCE_THRESHOLD_TO_CANCEL_CARD_SELECTION = 0.01f;
static const float FILTERS_TEXT_SNAP_TO_EDGE_SCALE_FACTOR = 415.0f;
static const float FILTER_CHECKBOX_SNAP_TO_EDGE_SCALE_FACTOR = 0.3f;
static const float FILTER_ICON_SNAP_TO_EDGE_SCALE_FACTOR = 1.6f;
static const float CARD_SELECTION_DESELECTION_BUMP_Z = 0.01f;

static constexpr int MIN_CONTAINER_ENTRIES_TO_ANIMATE = 5;
static constexpr int CARD_DELETION_SERVICE_PRICE = 100;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::CARD_LIBRARY_SCENE
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

static const std::unordered_map<strutils::StringId, glm::vec3, strutils::StringIdHasher> CARD_FAMILY_NAME_TO_FILTER_POSITION =
{
    { game_constants::DINOSAURS_FAMILY_NAME, glm::vec3(0.0f, 0.075f, 23.2f) },
    { game_constants::RODENTS_FAMILY_NAME, glm::vec3(0.0f, -0.025f, 23.2f) },
    { game_constants::INSECTS_FAMILY_NAME, glm::vec3(0.0f, -0.125f, 23.2f) }
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& CardLibrarySceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

CardLibrarySceneLogicManager::CardLibrarySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

CardLibrarySceneLogicManager::~CardLibrarySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mScene = scene;
    CardDataRepository::GetInstance().LoadCardData(true);
    mCardTooltipController = nullptr;
    mSelectedCardIndex = -1;
    mCoinAnimationValue = 0.0f;
    mAnimatingCoinValue = false;
    mHasSentTutorialTrigger = false;
    
    switch (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType())
    {
        case CardLibraryBehaviorType::STORY_CARDS:
        {
            std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(STORY_CARDS_TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = TITLE_STORY_CARDS;
        } break;
            
        case CardLibraryBehaviorType::BROWSING_FOR_DELETION:
        {
            std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(STORY_CARDS_TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = TITLE_BROWSING_FOR_DELETION;
        } break;
            
        case CardLibraryBehaviorType::CARD_LIBRARY:
        {
            std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(STORY_CARDS_TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = TITLE_CARD_LIBRARY;
        }
    }
    
    
    if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY)
    {
        // Card Library Filtering Text
        scene::TextSceneObjectData filtersTextData;
        filtersTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        filtersTextData.mText = "Filters";
        
        auto filtersTextSceneObject = scene->CreateSceneObject(FILTERS_TEXT_SCENE_OBJECT_NAME);
        filtersTextSceneObject->mSceneObjectTypeData = std::move(filtersTextData);
        filtersTextSceneObject->mPosition = FILTERS_TEXT_POSITION;
        filtersTextSceneObject->mScale = BUTTON_SCALE;
        filtersTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        filtersTextSceneObject->mInvisible = true;
        filtersTextSceneObject->mSnapToEdgeBehavior = scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE;
        filtersTextSceneObject->mSnapToEdgeScaleOffsetFactor = FILTERS_TEXT_SNAP_TO_EDGE_SCALE_FACTOR;
        
        // Card Library Filtering Checkboxes
        for (const auto& cardFamilyEntry: game_constants::CARD_FAMILY_NAMES_TO_TEXTURES)
        {
            auto filterCheckboxSceneObject = scene->CreateSceneObject(strutils::StringId(cardFamilyEntry.first.GetString() + "_filter_checkbox"));
            filterCheckboxSceneObject->mPosition = CARD_FAMILY_NAME_TO_FILTER_POSITION.at(cardFamilyEntry.first);
            filterCheckboxSceneObject->mScale = CHECKBOX_SCALE;
            filterCheckboxSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME);
            filterCheckboxSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            filterCheckboxSceneObject->mInvisible = true;
            filterCheckboxSceneObject->mSnapToEdgeBehavior = scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE;
            filterCheckboxSceneObject->mSnapToEdgeScaleOffsetFactor = FILTER_CHECKBOX_SNAP_TO_EDGE_SCALE_FACTOR;
            filterCheckboxSceneObject->mBoundingRectMultiplier /= 2.0f;
            
            auto filterIconSceneObject = scene->CreateSceneObject(strutils::StringId(cardFamilyEntry.first.GetString() + "_filter_icon"));
            filterIconSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + game_constants::CARD_FAMILY_NAMES_TO_TEXTURES.at(cardFamilyEntry.first));
            filterIconSceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CARD_FAMILY_FILTER_ICON_MASK_TEXTURE_FILE_NAME);
            filterIconSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_FAMILY_FILTER_ICON_SHADER_FILE_NAME);
            filterIconSceneObject->mScale = FILTER_ICON_SCALE;
            filterIconSceneObject->mPosition = CARD_FAMILY_NAME_TO_FILTER_POSITION.at(cardFamilyEntry.first);
            filterIconSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            filterIconSceneObject->mInvisible = true;
            filterIconSceneObject->mSnapToEdgeBehavior = scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE;
            filterIconSceneObject->mSnapToEdgeScaleOffsetFactor = FILTER_ICON_SNAP_TO_EDGE_SCALE_FACTOR;
        }
        
        // Card Collection Text
        scene::TextSceneObjectData cardCollectionTextData;
        cardCollectionTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        cardCollectionTextData.mText = "Cards Collected: ";
        
        auto cardCollectionTextSceneObject = scene->CreateSceneObject(CARD_COLLECTION_TEXT_SCENE_OBJECT_NAME);
        cardCollectionTextSceneObject->mSceneObjectTypeData = std::move(cardCollectionTextData);
        cardCollectionTextSceneObject->mPosition = CARD_COLLECTION_TEXT_POSITION;
        cardCollectionTextSceneObject->mScale = CARD_COLLECTION_TEXT_SCALE;
        cardCollectionTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        cardCollectionTextSceneObject->mInvisible = true;
        
        // Normal Card Collection Text
        auto totalCardPoolSize = DataRepository::GetInstance().GetUnlockedCardIds().size() + CardDataRepository::GetInstance().GetCardPackLockedCardRewardsPool().size();
        auto percentageCollection = static_cast<int>((DataRepository::GetInstance().GetUnlockedCardIds().size() * 100.0f)/static_cast<float>(totalCardPoolSize));
        scene::TextSceneObjectData normalCardCollectionTextData;
        normalCardCollectionTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        normalCardCollectionTextData.mText = "Normal " + std::to_string(percentageCollection) + "%";
        
        auto normalCardCollectionTextSceneObject = scene->CreateSceneObject(NORMAL_CARD_COLLECTION_TEXT_SCENE_OBJECT_NAME);
        normalCardCollectionTextSceneObject->mSceneObjectTypeData = std::move(normalCardCollectionTextData);
        normalCardCollectionTextSceneObject->mPosition = NORMAL_CARD_COLLECTION_TEXT_POSITION;
        normalCardCollectionTextSceneObject->mScale = CARD_COLLECTION_TEXT_SCALE;
        normalCardCollectionTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        normalCardCollectionTextSceneObject->mInvisible = true;
        
        if (percentageCollection == 100)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::NORMAL_COLLECTOR);
        }
        
        // Golden Card Collection Text
        auto goldenPercentageCollection = static_cast<int>((DataRepository::GetInstance().GetGoldenCardIdMap().size() * 100.0f)/static_cast<float>(totalCardPoolSize));
        scene::TextSceneObjectData goldenCardCollectionTextData;
        goldenCardCollectionTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
        goldenCardCollectionTextData.mText = "Golden " + std::to_string(goldenPercentageCollection) + "%";
        
        auto goldenCardCollectionTextSceneObject = scene->CreateSceneObject(GOLDEN_CARD_COLLECTION_TEXT_SCENE_OBJECT_NAME);
        goldenCardCollectionTextSceneObject->mSceneObjectTypeData = std::move(goldenCardCollectionTextData);
        goldenCardCollectionTextSceneObject->mPosition = GOLDEN_CARD_COLLECTION_TEXT_POSITION;
        goldenCardCollectionTextSceneObject->mScale = CARD_COLLECTION_TEXT_SCALE;
        goldenCardCollectionTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + GOLDEN_CARDS_COLLECTED_TEXT_SHADER_FILE_NAME);
        goldenCardCollectionTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        goldenCardCollectionTextSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = GOLDEN_CARDS_COLLECTED_TEXT_COLOR;
        goldenCardCollectionTextSceneObject->mInvisible = true;
        
        if (goldenPercentageCollection == 100)
        {
            events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::GOLDEN_COLLECTOR);
        }
    }
    
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
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        DELETE_CARD_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Delete",
        DELETE_CARD_BUTTON_SCENE_OBJECT_NAME,
        [=]()
        {
            DeleteCard();
        },
        *mScene
    ));
    mAnimatedButtons.back()->GetSceneObject()->mInvisible = true;
    mAnimatedButtons.back()->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        CANCEL_BUTTON_POSITION,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::BROWSING_FOR_DELETION ? "Cancel" : "Back",
        CANCEL_BUTTON_SCENE_OBJECT_NAME,
        [=]()
        {
            DeselectCard();
        },
        *mScene
    ));
    mAnimatedButtons.back()->GetSceneObject()->mInvisible = true;
    mAnimatedButtons.back()->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    CreateCardEntriesAndContainer();
    
    // Golden card behavior Checkbox
    auto goldenCheckboxSceneObject = scene->CreateSceneObject(GOLDEN_CHECKBOX_SCENE_OBJECT_NAME);
    goldenCheckboxSceneObject->mPosition = GOLDEN_CHECKBOX_POSITION;
    goldenCheckboxSceneObject->mScale = CHECKBOX_SCALE;
    goldenCheckboxSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
    goldenCheckboxSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    goldenCheckboxSceneObject->mInvisible = true;
    goldenCheckboxSceneObject->mBoundingRectMultiplier /= 2.0f;
    
    scene::TextSceneObjectData goldenCheckboxTextData;
    goldenCheckboxTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
    goldenCheckboxTextData.mText = "Golden";
    
    auto goldenCheckboxTextSceneObject = scene->CreateSceneObject(GOLDEN_CHECKBOX_TEXT_SCENE_OBJECT_NAME);
    goldenCheckboxTextSceneObject->mSceneObjectTypeData = std::move(goldenCheckboxTextData);
    goldenCheckboxTextSceneObject->mPosition = GOLDEN_CHECKBOX_TEXT_POSITION;
    goldenCheckboxTextSceneObject->mScale = GOLDEN_CHECKBOX_TEXT_SCALE;
    goldenCheckboxTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    goldenCheckboxTextSceneObject->mInvisible = true;
    
    // Staggered Item Presentation
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            continue;
        }
        
        if
        (
            sceneObject->mName == CARD_DELETION_OVERLAY_SCENE_OBJECT_NAME ||
            sceneObject->mName == DELETE_CARD_BUTTON_SCENE_OBJECT_NAME ||
            sceneObject->mName == CANCEL_BUTTON_SCENE_OBJECT_NAME ||
            sceneObject->mName == GOLDEN_CHECKBOX_TEXT_SCENE_OBJECT_NAME ||
            sceneObject->mName == GOLDEN_CHECKBOX_SCENE_OBJECT_NAME
        )
        {
            continue;
        }
        
        sceneObject->mInvisible = false;
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, ITEMS_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=](){});
    }
    
    events::EventSystem::GetInstance().RegisterForEvent<events::WindowResizeEvent>(this, &CardLibrarySceneLogicManager::OnWindowResize);
    mTransitioning = false;
    mSceneState = SceneState::BROWSING_CARDS;
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    if (mTransitioning)
    {
        return;
    }
    
    if (!mHasSentTutorialTrigger && DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::CARD_LIBRARY_TUTORIAL);
        mHasSentTutorialTrigger = true;
        
        // Tutorials might be disabled so we need to force this seen
        // to not make the NEW indicator sticky on the main menu.
        auto seenTutorials = DataRepository::GetInstance().GetSeenTutorials();
        seenTutorials.push_back(tutorials::CARD_LIBRARY_TUTORIAL);
        DataRepository::GetInstance().SetSeenTutorials(seenTutorials);
    }
    
    for (auto i = 0; i < mCardContainer->GetItems().size(); ++i)
    {
        for (auto& sceneObject: mCardContainer->GetItems()[i].mSceneObjects)
        {
            sceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time + i;
        }
    }
    
    if (CoreSystemsEngine::GetInstance().GetAnimationManager().IsAnimationPlaying(CARD_DESELECTION_ANIMATION_NAME))
    {
        return;
    }
    
    switch (mSceneState)
    {
        case SceneState::BROWSING_CARDS:
        {
            const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
            auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(mScene->GetCamera().GetViewMatrix(), mScene->GetCamera().GetProjMatrix());
            
            if (mCardContainer)
            {
                static int sToolTipIndex = -1;
                static float sToolTipPointeePosY = 0.0f;
                
                if (mSelectedCardIndex != -1 && !CoreSystemsEngine::GetInstance().GetInputStateManager().VButtonPressed(input::Button::MAIN_BUTTON))
                {
                    auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.front());
                    if (math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos) && glm::distance(mSelectedCardInitialPosition, mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.front()->mPosition) < MAX_SWIPE_DISTANCE_THRESHOLD_TO_CANCEL_CARD_SELECTION)
                    {
                        SelectCard();
                        return;
                    }
                }
                
                const auto& cardHistoryContainerUpdateResult = mCardContainer->Update(dtMillis);
                if (cardHistoryContainerUpdateResult.mInteractionType == InteractionType::INTERACTED_WITH_ELEMENTS)
                {
                    if (sToolTipIndex != cardHistoryContainerUpdateResult.mInteractedElementIndex)
                    {
                        sToolTipIndex = cardHistoryContainerUpdateResult.mInteractedElementIndex;
                        auto interactedElementEntry = mCardContainer->GetItems()[cardHistoryContainerUpdateResult.mInteractedElementIndex];
                        
                        switch (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType())
                        {
                            case CardLibraryBehaviorType::STORY_CARDS:
                            {
                                auto cardData = CardDataRepository::GetInstance().GetCardData(interactedElementEntry.mCardSoWrapper->mCardData.mCardId, game_constants::LOCAL_PLAYER_INDEX);
                                
                                DestroyCardTooltip();
                                
                                if (cardData.IsSpell())
                                {
                                    sToolTipPointeePosY = interactedElementEntry.mSceneObjects.front()->mPosition.y;
                                    
                                    CreateCardTooltip(interactedElementEntry.mSceneObjects.front()->mPosition, cardData.mCardEffectTooltip);
                                }
                            } break;
                            
                            case CardLibraryBehaviorType::CARD_LIBRARY:
                            case CardLibraryBehaviorType::BROWSING_FOR_DELETION:
                            {
                                mSelectedCardIndex = cardHistoryContainerUpdateResult.mInteractedElementIndex;
                                mSelectedCardInitialPosition = mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.front()->mPosition;
                            } break;
                        }
                    }
                }
                
                if (!CoreSystemsEngine::GetInstance().GetInputStateManager().VButtonPressed(input::Button::MAIN_BUTTON))
                {
                    mSelectedCardIndex = -1;
                }
                
                if (sToolTipIndex != -1)
                {
                    auto interactedElementEntry = mCardContainer->GetItems()[sToolTipIndex];
                    if (math::Abs(interactedElementEntry.mSceneObjects.front()->mPosition.y - sToolTipPointeePosY) > 0.01f)
                    {
                        sToolTipIndex = -1;
                        DestroyCardTooltip();
                    }
                }
            }
            
            for (auto& animatedButton: mAnimatedButtons)
            {
                animatedButton->Update(dtMillis);
            }
            
            if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
            {
                // Interaction with filters
                for (const auto& cardFamilyEntry: game_constants::CARD_FAMILY_NAMES_TO_TEXTURES)
                {
                    auto filterCheckboxSceneObject = mScene->FindSceneObject(strutils::StringId(cardFamilyEntry.first.GetString() + "_filter_checkbox"));
                    auto filterIconSceneObject = mScene->FindSceneObject(strutils::StringId(cardFamilyEntry.first.GetString() + "_filter_icon"));
                    
                    auto filterCheckboxSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*filterCheckboxSceneObject);
                    auto filterIconSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*filterIconSceneObject);
                    
                    if
                    (
                        math::IsPointInsideRectangle(filterCheckboxSceneObjectRect.bottomLeft, filterCheckboxSceneObjectRect.topRight, worldTouchPos) ||
                        math::IsPointInsideRectangle(filterIconSceneObjectRect.bottomLeft, filterIconSceneObjectRect.topRight, worldTouchPos)
                    )
                    {
                        ToggleFilterCheckbox(filterCheckboxSceneObject);
                    }
                }
            }
        } break;
            
        case SceneState::SELECTED_CARD_FOR_DELETION:
        {
            for (auto& animatedButton: mAnimatedButtons)
            {
                if (animatedButton->GetSceneObject()->mName == BACK_BUTTON_NAME)
                {
                    continue;
                }
                
                animatedButton->Update(dtMillis);
            }
        } break;
            
        case SceneState::SELECTED_CARD_IN_CARD_LIBRARY:
        {
            const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
            const auto& goldenCardIds = DataRepository::GetInstance().GetGoldenCardIdMap();
            auto selectedCard = mCardContainer->GetItems()[mSelectedCardIndex].mCardSoWrapper;
            
            if (goldenCardIds.count(selectedCard->mCardData.mCardId) && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
            {
                auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(mScene->GetCamera().GetViewMatrix(), mScene->GetCamera().GetProjMatrix());
            
                auto goldenCheckboxSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mScene->FindSceneObject(GOLDEN_CHECKBOX_SCENE_OBJECT_NAME));
                auto goldenCheckboxTextSceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mScene->FindSceneObject(GOLDEN_CHECKBOX_TEXT_SCENE_OBJECT_NAME));
                
                if
                (
                    math::IsPointInsideRectangle(goldenCheckboxSceneObjectRect.bottomLeft, goldenCheckboxSceneObjectRect.topRight, worldTouchPos) ||
                    math::IsPointInsideRectangle(goldenCheckboxTextSceneObjectRect.bottomLeft, goldenCheckboxTextSceneObjectRect.topRight, worldTouchPos)
                )
                {
                    ToggleGoldenCheckbox();
                }
                    
            }
            for (auto& animatedButton: mAnimatedButtons)
            {
                if (animatedButton->GetSceneObject()->mName == BACK_BUTTON_NAME)
                {
                    continue;
                }
                
                animatedButton->Update(dtMillis);
            }
        } break;
            
        case SceneState::DISSOLVING_DELETED_CARD:
        {
            auto interactedElementEntry = mCardContainer->GetItems()[mSelectedCardIndex];
            auto selectedSceneObject = interactedElementEntry.mSceneObjects.front();
            
            selectedSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] += dtMillis * CARD_DISSOLVE_SPEED;
            
            if (selectedSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] >= MAX_CARD_DISSOLVE_VALUE)
            {
                selectedSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] = MAX_CARD_DISSOLVE_VALUE;
                events::EventSystem::GetInstance().DispatchEvent<events::CardDeletionAnimationFinishedEvent>();
                events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
                mTransitioning = true;
            }
            
            if (mAnimatingCoinValue)
            {
                DataRepository::GetInstance().CurrencyCoins().SetDisplayedValue(static_cast<long long>(mCoinAnimationValue));
            }
            
            auto guiObjectManager = mGameSceneTransitionManager->GetSceneLogicManagerResponsibleForScene(mPreviousScene)->VGetGuiObjectManager();
            if (guiObjectManager)
            {
                guiObjectManager->Update(dtMillis);
            }
        }
    }
    
    
    if (mCardTooltipController)
    {
        mCardTooltipController->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    DestroyCardTooltip();
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, ITEMS_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            if (sceneObject->mName == STORY_CARDS_TITLE_SCENE_OBJECT_NAME ||
                sceneObject->mName == CARD_DELETION_OVERLAY_SCENE_OBJECT_NAME)
            {
                sceneObject->mInvisible = true;
                return;
            }
            
            scene->RemoveSceneObject(sceneObject->mName);
        });
    }
    
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    
    animationManager.StopAnimation(game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(previousScene->GetUpdateTimeSpeedFactor(), 1.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> CardLibrarySceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::OnWindowResize(const events::WindowResizeEvent&)
{
    mScene->RecalculatePositionOfEdgeSnappingSceneObjects();
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::CreateCardEntriesAndContainer()
{
    // Checbox values
    resources::ResourceId checkboxFilledTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME);

    // Clean up existing container
    bool containerExists = mCardContainer != nullptr;
    if (containerExists)
    {
        for (const auto& containerItem: mCardContainer->GetItems())
        {
            for (const auto& sceneObject: containerItem.mSceneObjects)
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
                mScene->RemoveSceneObject(sceneObject->mName);
            }
        }
        
        mCardContainer = nullptr;
    }
    
    // Card Container
    mCardContainer = std::make_unique<SwipeableContainer<CardEntry>>
    (
        ContainerType::VERTICAL_MATRIX,
        CONTAINER_ITEM_ENTRY_SCALE,
        CARD_CONTAINER_BOUNDS,
        CARD_CONTAINER_CUTOFF_VALUES,
        CARD_CONTAINER_SCENE_OBJECT_NAME,
        CARD_ENTRY_Z,
        *mScene,
        MIN_CONTAINER_ENTRIES_TO_ANIMATE
    );
    
    // Collect cards
    auto cards = DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY ? DataRepository::GetInstance().GetUnlockedCardIds() : DataRepository::GetInstance().GetCurrentStoryPlayerDeck();
    //cards = CardDataRepository::GetInstance().GetAllCardIds();
    
    // Sort cards (normal cards, spell cards, sorted by weight)
    std::sort(cards.begin(), cards.end(), [](const int& lhs, const int& rhs)
    {
        const auto& lhsCardData = CardDataRepository::GetInstance().GetCardData(lhs, game_constants::LOCAL_PLAYER_INDEX);
        const auto& rhsCardData = CardDataRepository::GetInstance().GetCardData(rhs, game_constants::LOCAL_PLAYER_INDEX);
        
        if ((lhsCardData.IsSpell() && rhsCardData.IsSpell()) || (!lhsCardData.IsSpell() && !rhsCardData.IsSpell()))
        {
            return lhsCardData.mCardWeight < rhsCardData.mCardWeight;
        }
        else
        {
            return !lhsCardData.IsSpell();
        }
    });
    
    // Filter cards
    if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY)
    {
        std::vector<std::function<bool(int)>> filterPredicates;
        
        for (const auto& cardFamilyEntry: game_constants::CARD_FAMILY_NAMES_TO_TEXTURES)
        {
            auto filterCheckboxSceneObject = mScene->FindSceneObject(strutils::StringId(cardFamilyEntry.first.GetString() + "_filter_checkbox"));
            if (filterCheckboxSceneObject->mTextureResourceId == checkboxFilledTextureResourceId)
            {
                filterPredicates.emplace_back([=](const int cardId){ return CardDataRepository::GetInstance().GetCardData(cardId, game_constants::LOCAL_PLAYER_INDEX).mCardFamily == cardFamilyEntry.first; });
            }
        }
        
        for (auto cardIter = cards.begin(); cardIter != cards.end();)
        {
            bool shouldKeepCard = false;
            for (auto& filterPredicate: filterPredicates)
            {
                if (filterPredicate(*cardIter))
                {
                    shouldKeepCard = true;
                }
            }
            
            if (!shouldKeepCard)
            {
                cardIter = cards.erase(cardIter);
            }
            else
            {
                cardIter++;
            }
        }
    }
    
    
    // Create card entries
    const auto& newCardIds = DataRepository::GetInstance().GetNewCardIds();
    for (const auto& cardId: cards)
    {
        CardData cardData = CardDataRepository::GetInstance().GetCardData(cardId, game_constants::LOCAL_PLAYER_INDEX);
        const auto& cardIdToGoldenCardEnabledMap = DataRepository::GetInstance().GetGoldenCardIdMap();
        bool isGoldenCard = cardIdToGoldenCardEnabledMap.count(cardId) && cardIdToGoldenCardEnabledMap.at(cardId);
        
        auto cardSoWrapper = card_utils::CreateCardSoWrapper(&cardData, glm::vec3(), "", CardOrientation::FRONT_FACE, isGoldenCard ? CardRarity::GOLDEN : CardRarity::NORMAL, true, false, true, {}, {}, *mScene);
        cardSoWrapper->mSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_ENTRY_SHADER);
        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.s;
        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.t;
        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        cardSoWrapper->mSceneObject->mScale = CARD_ENTRY_SCALE;
        
        CardEntry cardEntry;
        cardEntry.mCardSoWrapper = cardSoWrapper;
        cardEntry.mSceneObjects.emplace_back(cardSoWrapper->mSceneObject);
        
        // Create family stamps
        if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY)
        {
            auto familyStampSceneObject = mScene->CreateSceneObject(strutils::StringId());
            familyStampSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + game_constants::CARD_FAMILY_NAMES_TO_TEXTURES.at(cardData.mCardFamily));
            familyStampSceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + FAMILY_STAMP_MASK_TEXTURE_FILE_NAME);
            familyStampSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_FAMILY_STAMP_SHADER_FILE_NAME);
            familyStampSceneObject->mScale.x = familyStampSceneObject->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE;
            familyStampSceneObject->mPosition = cardEntry.mSceneObjects.back()->mPosition;
            familyStampSceneObject->mPosition.x -= 0.008f;
            familyStampSceneObject->mPosition.y -= 0.06f;
            familyStampSceneObject->mPosition.z += game_constants::CARD_COMPONENT_Z_OFFSET;
            familyStampSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.s;
            familyStampSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.t;
            familyStampSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            cardEntry.mSceneObjects.emplace_back(familyStampSceneObject);
        }
        
        // Create new card indicator
        if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY && std::find(newCardIds.begin(), newCardIds.end(), cardSoWrapper->mCardData.mCardId) != newCardIds.end())
        {
            auto newIndicatorSceneObject = mScene->CreateSceneObject(strutils::StringId());
            newIndicatorSceneObject->mPosition += NEW_CARD_INDICATOR_POSITION_OFFSET;
            
            scene::TextSceneObjectData textNewIndicatorData;
            textNewIndicatorData.mText = "NEW";
            textNewIndicatorData.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            newIndicatorSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + NEW_CARD_INDICATOR_SHADER_FILE_NAME);
            newIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.s;
            newIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.t;
            
            newIndicatorSceneObject->mScale = NEW_CARD_INDICATOR_SCALE;
            newIndicatorSceneObject->mSceneObjectTypeData = std::move(textNewIndicatorData);
            cardEntry.mSceneObjects.emplace_back(newIndicatorSceneObject);
        }
        
        mCardContainer->AddItem(std::move(cardEntry), EntryAdditionStrategy::ADD_ON_THE_BACK);
    }
    
    // If container doesn't exist the staggered fade in will happen automatically at the end of VInitScene
    if (containerExists)
    {
        // Staggered Item Presentation
        size_t sceneObjectIndex = 0;
        for (const auto& containerItems: mCardContainer->GetItems())
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

void CardLibrarySceneLogicManager::CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText)
{
    bool shouldBeHorFlipped = cardOriginPostion.x > 0.0f;
    bool shouldBeVerFlipped = cardOriginPostion.y > 0.0f;
    
    mCardTooltipController = std::make_unique<CardTooltipController>
    (
        cardOriginPostion + CARD_TOOLTIP_POSITION_OFFSET,
        CARD_TOOLTIP_BASE_SCALE,
        tooltipText,
        false,
        shouldBeHorFlipped,
        shouldBeVerFlipped,
        *mScene
    );
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::DestroyCardTooltip()
{
    if (mCardTooltipController)
    {
        for (auto sceneObject: mCardTooltipController->GetSceneObjects())
        {
            mScene->RemoveSceneObject(sceneObject->mName);
        }
    }
    
    mCardTooltipController = nullptr;
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::SelectCard()
{
    auto card = mCardContainer->GetItems()[mSelectedCardIndex].mCardSoWrapper;
    const auto& goldenCardIds = DataRepository::GetInstance().GetGoldenCardIdMap();
    auto newCardIds = DataRepository::GetInstance().GetNewCardIds();
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY)
    {
        auto newCardIter = std::find(newCardIds.begin(), newCardIds.end(), card->mCardData.mCardId);
        if (newCardIter != newCardIds.end())
        {
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>( mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.back(), 0.0f, NEW_CARD_INDICATOR_FADE_OUT_ANIMATION_DURATION_SECS), [=](){});
            
            newCardIds.erase(newCardIter);
            DataRepository::GetInstance().SetNewCardIds(newCardIds);
            DataRepository::GetInstance().FlushStateToFile();
        }
    }
    
    // Fade in cancel button
    auto cancelButtonSceneObject = mScene->FindSceneObject(CANCEL_BUTTON_SCENE_OBJECT_NAME);
    cancelButtonSceneObject->mInvisible = false;
    animationManager.StopAllAnimationsPlayingForSceneObject(cancelButtonSceneObject->mName);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cancelButtonSceneObject, 1.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){});
    
    // Fade in selected product overlay
    auto cardDeletionOverlaySceneObject = mScene->FindSceneObject(CARD_DELETION_OVERLAY_SCENE_OBJECT_NAME);
    cardDeletionOverlaySceneObject->mInvisible = false;
    animationManager.StopAllAnimationsPlayingForSceneObject(cardDeletionOverlaySceneObject->mName);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cardDeletionOverlaySceneObject, SELECTED_CARD_OVERLAY_MAX_ALPHA, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){});
    
    // Animate product to target position
    mSelectedCardInitialPosition = card->mSceneObject->mPosition;
    for (int i = static_cast<int>(mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.size()) - 1; i >= 0; --i)
    {
        mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects[i]->mPosition.z = mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects[i]->mPosition.z - mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects[0]->mPosition.z + cardDeletionOverlaySceneObject->mPosition.z + CARD_SELECTION_DESELECTION_BUMP_Z;
    }
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects, SELECTED_CARD_TARGET_POSITION, CARD_ENTRY_SCALE * SELECTED_CARD_SCALE_FACTOR, SELECTED_CARD_ANIMATION_DURATION_SECS), [=]()
    {
        if (card->mCardData.IsSpell())
        {
            CreateCardTooltip(SELECTED_CARD_TARGET_POSITION, card->mCardData.mCardEffectTooltip);
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(game_constants::GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME);
        card->mSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(card->mSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME], game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f), [](){}, game_constants::GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME);
    });
    
    if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::CARD_LIBRARY)
    {
        if (goldenCardIds.count(card->mCardData.mCardId))
        {
            // Fade in golden checkbox
            auto goldenCheckBoxSceneObject = mScene->FindSceneObject(GOLDEN_CHECKBOX_SCENE_OBJECT_NAME);
            goldenCheckBoxSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (goldenCardIds.at(card->mCardData.mCardId) ? GOLDEN_CHECKBOX_FILLED_TEXTURE_FILE_NAME : CHECKBOX_EMPTY_TEXTURE_FILE_NAME));
                                                                                                                                      
            goldenCheckBoxSceneObject->mInvisible = false;
            animationManager.StopAllAnimationsPlayingForSceneObject(goldenCheckBoxSceneObject->mName);
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(goldenCheckBoxSceneObject, 1.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){});
            
            // Fade in golden text
            auto goldenCheckBoxTextSceneObject = mScene->FindSceneObject(GOLDEN_CHECKBOX_TEXT_SCENE_OBJECT_NAME);
            goldenCheckBoxTextSceneObject->mInvisible = false;
            animationManager.StopAllAnimationsPlayingForSceneObject(goldenCheckBoxTextSceneObject->mName);
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(goldenCheckBoxTextSceneObject, 1.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){});
        }
        
        mSceneState = SceneState::SELECTED_CARD_IN_CARD_LIBRARY;
    }
    else if (DataRepository::GetInstance().GetCurrentCardLibraryBehaviorType() == CardLibraryBehaviorType::BROWSING_FOR_DELETION)
    {
        // Fade in delete button
        auto deleteCardButtonSceneObject = mScene->FindSceneObject(DELETE_CARD_BUTTON_SCENE_OBJECT_NAME);
        deleteCardButtonSceneObject->mInvisible = false;
        animationManager.StopAllAnimationsPlayingForSceneObject(deleteCardButtonSceneObject->mName);
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(deleteCardButtonSceneObject, 1.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){});
        
        mSceneState = SceneState::SELECTED_CARD_FOR_DELETION;
    }
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::DeleteCard()
{
    auto cardSceneObject = mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.front();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    animationManager.StopAllAnimationsPlayingForSceneObject(cardSceneObject->mName);
    
    // Fade out delete card button
    auto deleteCardButtonSceneObject = mScene->FindSceneObject(DELETE_CARD_BUTTON_SCENE_OBJECT_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(deleteCardButtonSceneObject, 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ deleteCardButtonSceneObject->mInvisible = true; });
    
    // Fade out cancel button
    auto cancelButtonSceneObject = mScene->FindSceneObject(CANCEL_BUTTON_SCENE_OBJECT_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cancelButtonSceneObject, 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ cancelButtonSceneObject->mInvisible = true; });
    
    cardSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + DISSOLVE_SHADER_FILE_NAME);
    cardSceneObject->mEffectTextureResourceIds[1] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DISSOLVE_TEXTURE_FILE_NAME);
    cardSceneObject->mShaderFloatUniformValues[DISSOLVE_THRESHOLD_UNIFORM_NAME] = 0.0f;
    cardSceneObject->mShaderFloatUniformValues[CARD_ORIGIN_X_UNIFORM_NAME] = cardSceneObject->mPosition.x;
    cardSceneObject->mShaderFloatUniformValues[CARD_ORIGIN_Y_UNIFORM_NAME] = cardSceneObject->mPosition.y;
    cardSceneObject->mShaderFloatUniformValues[DISSOLVE_MAGNITUDE_UNIFORM_NAME] = math::RandomFloat(CARD_DISSOLVE_EFFECT_MAG_RANGE.x, CARD_DISSOLVE_EFFECT_MAG_RANGE.y);
    
    auto playerDeck = DataRepository::GetInstance().GetCurrentStoryPlayerDeck();
    auto cardIdToErase = mCardContainer->GetItems()[mSelectedCardIndex].mCardSoWrapper->mCardData.mCardId;
    playerDeck.erase(std::find(playerDeck.begin(), playerDeck.end(), cardIdToErase));
    
    auto storyDeletedCards = DataRepository::GetInstance().GetStoryDeletedCardIds();
    storyDeletedCards.push_back(cardIdToErase);
    
    DataRepository::GetInstance().SetCurrentStoryPlayerDeck(playerDeck);
    DataRepository::GetInstance().SetStoryDeletedCardIds(storyDeletedCards);
    
    DataRepository::GetInstance().AddShopBoughtProductCoordinates(game_constants::CARD_DELETION_PRODUCT_COORDS);
    
    auto& storyCurrencyCoins = DataRepository::GetInstance().CurrencyCoins();
    storyCurrencyCoins.SetValue(storyCurrencyCoins.GetValue() - CARD_DELETION_SERVICE_PRICE);
    
    mCoinAnimationValue = storyCurrencyCoins.GetDisplayedValue();
    mAnimatingCoinValue = true;
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mCoinAnimationValue, static_cast<float>(storyCurrencyCoins.GetValue()), ANIMATED_COIN_VALUE_DURATION_SECS), [=](){ mAnimatingCoinValue = false; });
    
    DataRepository::GetInstance().FlushStateToFile();
    
    mSceneState = SceneState::DISSOLVING_DELETED_CARD;
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::DeselectCard()
{
    DestroyCardTooltip();
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    auto cardSceneObject = mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.front();
    animationManager.StopAllAnimationsPlayingForSceneObject(cardSceneObject->mName);
    
    // Fade out delete card button
    auto deleteCardButtonSceneObject = mScene->FindSceneObject(DELETE_CARD_BUTTON_SCENE_OBJECT_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(deleteCardButtonSceneObject, 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ deleteCardButtonSceneObject->mInvisible = true; });
    
    // Fade out golden checkbox
    auto goldenCheckboxSceneObject = mScene->FindSceneObject(GOLDEN_CHECKBOX_SCENE_OBJECT_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(goldenCheckboxSceneObject, 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ goldenCheckboxSceneObject->mInvisible = true; });
    
    // Fade out golden text checkbox
    auto goldenTextCheckboxSceneObject = mScene->FindSceneObject(GOLDEN_CHECKBOX_TEXT_SCENE_OBJECT_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(goldenTextCheckboxSceneObject, 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ goldenTextCheckboxSceneObject->mInvisible = true; });
    
    // Fade out cancel button
    auto cancelButtonSceneObject = mScene->FindSceneObject(CANCEL_BUTTON_SCENE_OBJECT_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(cancelButtonSceneObject, 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ cancelButtonSceneObject->mInvisible = true; });
    
    // Fade in selected card overlay
    auto cardDeletionOverlaySceneObject = mScene->FindSceneObject(CARD_DELETION_OVERLAY_SCENE_OBJECT_NAME);
    animationManager.StopAllAnimationsPlayingForSceneObject(cardDeletionOverlaySceneObject->mName);
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mScene->FindSceneObject(CARD_DELETION_OVERLAY_SCENE_OBJECT_NAME), 0.0f, SELECTED_CARD_ANIMATION_DURATION_SECS/2), [=](){ cardDeletionOverlaySceneObject->mInvisible = true; });
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleGroupAnimation>(mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects, mSelectedCardInitialPosition, CARD_ENTRY_SCALE, SELECTED_CARD_ANIMATION_DURATION_SECS), [=](){ mSceneState = SceneState::BROWSING_CARDS; }, CARD_DESELECTION_ANIMATION_NAME);
    
    mSelectedCardIndex = -1;
    mCardContainer->ResetSwipeData();
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::ToggleFilterCheckbox(std::shared_ptr<scene::SceneObject> filterCheckboxSceneObject)
{
    resources::ResourceId checkboxFilledTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_FILLED_TEXTURE_FILE_NAME);
    resources::ResourceId checkboxEmptyTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
    
    bool newCheckboxValue = filterCheckboxSceneObject->mTextureResourceId == checkboxFilledTextureResourceId ? false : true;
    filterCheckboxSceneObject->mTextureResourceId = newCheckboxValue ? checkboxFilledTextureResourceId : checkboxEmptyTextureResourceId;
    
    CreateCardEntriesAndContainer();
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::ToggleGoldenCheckbox()
{
    auto goldenCheckBoxSceneObject = mScene->FindSceneObject(GOLDEN_CHECKBOX_SCENE_OBJECT_NAME);

    SetGoldenCheckboxValue(goldenCheckBoxSceneObject->mTextureResourceId == CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + GOLDEN_CHECKBOX_FILLED_TEXTURE_FILE_NAME) ? false : true);
}

///------------------------------------------------------------------------------------------------

void CardLibrarySceneLogicManager::SetGoldenCheckboxValue(const bool checkboxValue)
{
    resources::ResourceId goldenCheckboxFilledTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + GOLDEN_CHECKBOX_FILLED_TEXTURE_FILE_NAME);
    resources::ResourceId goldenCheckboxEmptyTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + CHECKBOX_EMPTY_TEXTURE_FILE_NAME);
    
    auto selectedCard = mCardContainer->GetItems()[mSelectedCardIndex].mCardSoWrapper;
    auto goldenCheckBoxSceneObject = mScene->FindSceneObject(GOLDEN_CHECKBOX_SCENE_OBJECT_NAME);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StopAnimation(game_constants::GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME);
    
    goldenCheckBoxSceneObject->mTextureResourceId = checkboxValue ? goldenCheckboxFilledTextureResourceId : goldenCheckboxEmptyTextureResourceId;
    
    auto cardSoWrapper = card_utils::CreateCardSoWrapper(&selectedCard->mCardData, glm::vec3(), "", CardOrientation::FRONT_FACE, checkboxValue ? CardRarity::GOLDEN : CardRarity::NORMAL, true, false, true, {}, {}, *mScene);
    cardSoWrapper->mSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_ENTRY_SHADER);
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.s;
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.t;
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME] = game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.s;
    cardSoWrapper->mSceneObject->mScale = CARD_ENTRY_SCALE;
    
    auto familyStampSceneObject = mScene->CreateSceneObject(strutils::StringId());
    familyStampSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + game_constants::CARD_FAMILY_NAMES_TO_TEXTURES.at(selectedCard->mCardData.mCardFamily));
    familyStampSceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + FAMILY_STAMP_MASK_TEXTURE_FILE_NAME);
    familyStampSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_FAMILY_STAMP_SHADER_FILE_NAME);
    familyStampSceneObject->mScale.x = familyStampSceneObject->mScale.y = game_constants::IN_GAME_CARD_PROPERTY_ICON_SCALE;
    familyStampSceneObject->mPosition = cardSoWrapper->mSceneObject->mPosition;
    familyStampSceneObject->mPosition.x -= 0.008f;
    familyStampSceneObject->mPosition.y -= 0.06f;
    familyStampSceneObject->mPosition.z += game_constants::CARD_COMPONENT_Z_OFFSET;
    familyStampSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.s;
    familyStampSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = CARD_ENTRY_CUTOFF_VALUES.t;
    familyStampSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    
    CardEntry cardEntry;
    cardEntry.mCardSoWrapper = cardSoWrapper;
    cardEntry.mSceneObjects.emplace_back(cardSoWrapper->mSceneObject);
    cardEntry.mSceneObjects.emplace_back(familyStampSceneObject);
    mCardContainer->ReplaceItemAtIndexWithNewItem(std::move(cardEntry), mSelectedCardIndex);
    
    DataRepository::GetInstance().SetGoldenCardMapEntry(selectedCard->mCardData.mCardId, checkboxValue);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(mCardContainer->GetItems()[mSelectedCardIndex].mSceneObjects.front()->mShaderFloatUniformValues[game_constants::LIGHT_POS_X_UNIFORM_NAME], game_constants::GOLDEN_CARD_LIGHT_POS_MIN_MAX_X.t, 1.0f), [](){}, game_constants::GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME);
    
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------
