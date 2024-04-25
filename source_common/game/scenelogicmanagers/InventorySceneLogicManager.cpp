///------------------------------------------------------------------------------------------------
///  InventorySceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 27/02/2024
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
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/CardTooltipController.h>
#include <game/CardUtils.h>
#include <game/GameSceneTransitionManager.h>
#include <game/GameSymbolicGlyphNames.h>
#include <game/GuiObjectManager.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/InventorySceneLogicManager.h>
#include <game/ProductRepository.h>

///------------------------------------------------------------------------------------------------

static const std::string ARTIFACT_ITEM_ENTRY_SHADER = "artifact_container_entry.vs";
static const std::string ARTIFACT_TEXT_ITEM_ENTRY_SHADER = "artifact_text_container_entry.vs";
static const std::string MUTATION_ITEM_ENTRY_SHADER = "mutation_container_entry.vs";
static const std::string UNIQUE_ARTIFACT_ICON_TEXTURE_FILE_NAME = "single_use_stamp.png";
static const std::string UNIQUE_ARTIFACT_ICON_SHADER_FILE_NAME = "artifact_single_use_icon_container_entry.vs";
static const std::string MUTATION_MESH_FILE_NAME = "virus.obj";
static const std::string MUTATION_SHADER_FILE_NAME = "virus.vs";
static const std::string MUTATION_TEXTURE_FILE_NAME = "virus.png";
static const std::string MUTATION_TEXT_NAME_PREFIX = "mutation_changes_text";

static const strutils::StringId MUTATION_TEXT_CONTINUE_BUTTON_NAME = strutils::StringId("mutation_text_continue_button");
static const strutils::StringId MUTATION_TEXT_OVERLAY_SCENE_OBJECT_NAME = strutils::StringId("mutation_text_overlay");
static const strutils::StringId MUTATION_SCENE_OBJECT_NAME = strutils::StringId("mutation");
static const strutils::StringId MUTATION_TEXT_COUNT_SCENE_OBJECT_NAME = strutils::StringId("mutation_count");
static const strutils::StringId BACK_BUTTON_NAME = strutils::StringId("back_button");
static const strutils::StringId ARTIFACTS_TITLE_SCENE_OBJECT_NAME = strutils::StringId("inventory_artifacts_title");
static const strutils::StringId MUTATIONS_TITLE_SCENE_OBJECT_NAME = strutils::StringId("inventory_mutations_title");
static const strutils::StringId ARTIFACT_ITEM_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("artifact_item_container");
static const strutils::StringId MUTATION_ITEM_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("mutation_item_container");
static const strutils::StringId POINT_LIGHT_POSITION_UNIFORM_NAME = strutils::StringId("point_light_position");
static const strutils::StringId DIFFUSE_COLOR_UNIFORM_NAME = strutils::StringId("mat_diffuse");
static const strutils::StringId AMBIENT_COLOR_UNIFORM_NAME = strutils::StringId("mat_ambient");
static const strutils::StringId SPEC_COLOR_UNIFORM_NAME = strutils::StringId("mat_spec");
static const strutils::StringId POINT_LIGHT_POWER_UNIFORM_NAME = strutils::StringId("point_light_power");
static const strutils::StringId AFFECTED_BY_LIGHT_UNIFORM_NAME = strutils::StringId("affected_by_light");

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 BACK_BUTTON_POSITION = {0.0f, -0.2f, 23.2f};
static const glm::vec3 ITEM_ENTRY_SCALE = glm::vec3(0.273f/1.5f, 0.2512f/1.5f, 2.0f);
static const glm::vec3 ITEM_TOOLTIP_POSITION_OFFSET = {0.0f, 0.1f, 0.0f};
static const glm::vec3 ITEM_TOOLTIP_BASE_SCALE = {0.3f, 0.274f, 1/10.0f};
static const glm::vec3 ARTIFACT_CONTAINER_ITEM_ENTRY_SCALE = {0.173f, 0.142f, 2.0f};
static const glm::vec3 ARTIFACT_TEXT_SCALE = {0.00025f, 0.00025f, 0.00025f};
static const glm::vec3 ARTIFACT_NAME_TEXT_OFFSET = {-0.06f, 0.05f, 0.1f};
static const glm::vec3 ARTIFACT_COUNT_TEXT_OFFSET = {-0.06f, 0.0f, 0.1f};
static const glm::vec3 ARTIFACT_UNIQUE_ICON_SCALE = {0.05f, 0.05f, 0.05f};
static const glm::vec3 ARTIFACT_UNIQUE_ICON_OFFSET = {-0.05f, -0.0f, 0.1f};
static const glm::vec3 MUTATION_POSITION = { -0.027f, -0.145f, 23.3f };
static const glm::vec3 MUTATION_SCALE = { 0.04f, 0.04f, 0.04f };
static const glm::vec3 MUTATION_TEXT_POSITION = { -0.131f, -0.133f, 23.3f };
static const glm::vec3 MUTATION_COUNT_TEXT_SCALE = {0.00035f, 0.00035f, 0.00035f};
static const glm::vec3 POINT_LIGHT_POSITION = { -1.0f, 0.0f, -1.0f };
static const glm::vec3 DIFFUSE_COLOR = { 1.0f, 1.0f, 1.0f };
static const glm::vec3 SPEC_COLOR = { 1.0f, 1.0f, 1.0f };
static const glm::vec3 AMB_COLOR = { 1.0f, 0.0f, 0.0f };
static const glm::vec3 CONTINUE_BUTTON_POSITION = {-0.078f, -0.2f, 24.1f};
static const glm::vec3 MUTATION_CHANGE_TEXT_SCALE = {0.0003f, 0.0003f, 0.0003f};
static const glm::vec3 MUTATION_CHANGE_TEXT_INIT_POSITION = {-0.134f, 0.207f, 24.1f};

static const glm::vec2 ARTIFACT_ITEM_ENTRY_CUTOFF_VALUES = {-0.047f, 0.183f};
static const glm::vec2 ARTIFACT_ITEM_CONTAINER_CUTOFF_VALUES = {0.076, 0.093f};
static const glm::vec2 NO_MUTATIONS_ARTIFACT_ITEM_ENTRY_CUTOFF_VALUES = {-0.185f, 0.183f};
static const glm::vec2 NO_MUTATIONS_ARTIFACT_ITEM_CONTAINER_CUTOFF_VALUES = {0.076, 0.093f};

static const math::Rectangle ARTIFACT_ITEM_CONTAINER_BOUNDS = {{-0.305f, -0.0525f}, {0.305f, 0.182f}};
static const math::Rectangle NO_MUTATIONS_ARTIFACT_ITEM_CONTAINER_BOUNDS = {{-0.305f, -0.250f}, {0.305f, 0.182f}};

static const float ITEMS_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.05f;
static const float BACK_BUTTON_SNAP_TO_EDGE_FACTOR = 950000.0f;
static const float ITEM_ENTRY_Z = 23.2f;
static const float POINT_LIGHT_POWER = 8.0f;
static const float MUTATION_ROTATION_SPEED = 1.0f/1000.0f;
static const float MUTATION_TEXT_OVERLAY_ALPHA = 0.9f;
static const float MUTATION_TEXT_OVERLAY_FADE_IN_OUT_DURATION_SECS = 0.35f;
static const float MUTATION_TEXT_CONTINUE_BUTTON_FADE_IN_OUT_DURATION_SECS = 0.5f;

static const int MIN_CONTAINER_ENTRIES_TO_ANIMATE = 4;
static const int NO_MUTATIONS_MIN_CONTAINER_ENTRIES_TO_ANIMATE = 7;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::INVENTORY_SCENE
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& InventorySceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

InventorySceneLogicManager::InventorySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

InventorySceneLogicManager::~InventorySceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mScene = scene;

    mArtifactsItemContainer = nullptr;
    mSelectedItemIndex = -1;
    mShowingMutationText = false;
    mToolTipIndex = -1;
    mToolTipPointeePosY = 0.0f;
    mToolTipPointeePosX = 0.0f;
    
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
    mAnimatedButtons.back()->GetSceneObject()->mInvisible = true;
    mAnimatedButtons.back()->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
    
    CreateItemEntriesAndContainer();
    
    // Staggered Item Presentation
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName) || sceneObject->mName == MUTATION_TEXT_OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }

        if (sceneObject->mName != MUTATIONS_TITLE_SCENE_OBJECT_NAME)
        {
            sceneObject->mInvisible = false;
        }
        
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, ITEMS_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=](){});
    }
    
    events::EventSystem::GetInstance().RegisterForEvent<events::WindowResizeEvent>(this, &InventorySceneLogicManager::OnWindowResize);
    mTransitioning = false;
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    if (mTransitioning)
    {
        return;
    }
    
    if (mShowingMutationText)
    {
        mAnimatedButtons.back()->Update(dtMillis);
        return;
    }
    
    UpdateItemContainer(dtMillis, mArtifactsItemContainer);
    
    auto mutationSceneObject = scene->FindSceneObject(MUTATION_SCENE_OBJECT_NAME);
    if (mutationSceneObject)
    {
        UpdateMutationInteraction(dtMillis, scene);
    }
    
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
    
    if (mItemTooltipController)
    {
        mItemTooltipController->Update(dtMillis);
    }
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    DestroyItemTooltip();
    
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, ITEMS_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            if (sceneObject->mName == ARTIFACTS_TITLE_SCENE_OBJECT_NAME || sceneObject->mName == MUTATIONS_TITLE_SCENE_OBJECT_NAME || sceneObject->mName == MUTATION_TEXT_OVERLAY_SCENE_OBJECT_NAME)
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

std::shared_ptr<GuiObjectManager> InventorySceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::UpdateItemContainer(const float dtMillis, std::unique_ptr<SwipeableContainer<ItemEntry>>& itemContainer)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    for (auto i = 0; i < itemContainer->GetItems().size(); ++i)
    {
        for (auto& sceneObject: itemContainer->GetItems()[i].mSceneObjects)
        {
            sceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time + i;
        }
    }
    
    if (itemContainer)
    {
        const auto& itemContainerUpdateResult = itemContainer->Update(dtMillis);
        
        if (itemContainerUpdateResult.mInteractionType == InteractionType::INTERACTED_WITH_ELEMENTS)
        {
            if (mToolTipIndex != itemContainerUpdateResult.mInteractedElementIndex)
            {
                mToolTipIndex = itemContainerUpdateResult.mInteractedElementIndex;
                auto interactedElementEntry = itemContainer->GetItems()[itemContainerUpdateResult.mInteractedElementIndex];
                
                DestroyItemTooltip();
                
                mToolTipPointeePosY = interactedElementEntry.mSceneObjects.front()->mPosition.y;
                mToolTipPointeePosX = interactedElementEntry.mSceneObjects.front()->mPosition.x;
                
                auto productDescription = ProductRepository::GetInstance().GetProductDefinition(interactedElementEntry.mArtifactOrMutationName).mDescription;
                CreateItemTooltip(interactedElementEntry.mSceneObjects.front()->mPosition, productDescription);
            }
        }
        else if (itemContainerUpdateResult.mInteractionType == InteractionType::INTERACTED_WITH_CONTAINER_AREA)
        {
            DestroyItemTooltip();
        }
        
        if (mToolTipIndex != -1)
        {
            auto interactedElementEntry = itemContainer->GetItems()[mToolTipIndex];
            
            if (itemContainer == mArtifactsItemContainer)
            {
                if (math::Abs(interactedElementEntry.mSceneObjects.front()->mPosition.y - mToolTipPointeePosY) > 0.01f)
                {
                    mToolTipIndex = -1;
                    DestroyItemTooltip();
                }
            }
            else
            {
                if (math::Abs(interactedElementEntry.mSceneObjects.front()->mPosition.x - mToolTipPointeePosX) > 0.01f)
                {
                    mToolTipIndex = -1;
                    DestroyItemTooltip();
                }
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::OnWindowResize(const events::WindowResizeEvent&)
{
    mScene->RecalculatePositionOfEdgeSnappingSceneObjects();
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::CreateItemEntriesAndContainer()
{
    // Clean up existing container
    bool containerExists = mArtifactsItemContainer != nullptr;
    if (containerExists)
    {
        for (const auto& containerItem: mArtifactsItemContainer->GetItems())
        {
            for (const auto& sceneObject: containerItem.mSceneObjects)
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
                mScene->RemoveSceneObject(sceneObject->mName);
            }
        }
        
        mArtifactsItemContainer = nullptr;
    }
    
    // Create artifact entries
    auto mutationCount = DataRepository::GetInstance().GetCurrentStoryMutationLevel();
    mArtifactsItemContainer = std::make_unique<SwipeableContainer<ItemEntry>>
    (
        ContainerType::VERTICAL_MATRIX,
        ARTIFACT_CONTAINER_ITEM_ENTRY_SCALE,
        mutationCount == 0 ? NO_MUTATIONS_ARTIFACT_ITEM_CONTAINER_BOUNDS : ARTIFACT_ITEM_CONTAINER_BOUNDS,
        mutationCount == 0 ? NO_MUTATIONS_ARTIFACT_ITEM_CONTAINER_CUTOFF_VALUES : ARTIFACT_ITEM_CONTAINER_CUTOFF_VALUES,
        ARTIFACT_ITEM_CONTAINER_SCENE_OBJECT_NAME,
        ITEM_ENTRY_Z,
        *mScene,
        mutationCount == 0 ? NO_MUTATIONS_MIN_CONTAINER_ENTRIES_TO_ANIMATE : MIN_CONTAINER_ENTRIES_TO_ANIMATE
    );
    
    auto artifactCount = 0;
    auto shaderCutoffValues = mutationCount == 0 ? NO_MUTATIONS_ARTIFACT_ITEM_ENTRY_CUTOFF_VALUES : ARTIFACT_ITEM_ENTRY_CUTOFF_VALUES;
    const auto& artifactEntries = DataRepository::GetInstance().GetCurrentStoryArtifacts();
    for (const auto& artifactEntry: artifactEntries)
    {
        artifactCount += artifactEntry.second;
        const auto& artifactItemProduct = ProductRepository::GetInstance().GetProductDefinition(artifactEntry.first);
        
        auto artifactSceneObject = mScene->CreateSceneObject();
        artifactSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ARTIFACT_ITEM_ENTRY_SHADER);
        artifactSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + std::get<std::string>(artifactItemProduct.mProductTexturePathOrCardId));
        artifactSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = shaderCutoffValues.s;
        artifactSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = shaderCutoffValues.t;
        artifactSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        artifactSceneObject->mScale = ITEM_ENTRY_SCALE;
        
        auto artifactNameTextSceneObject = mScene->CreateSceneObject();
        artifactNameTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ARTIFACT_TEXT_ITEM_ENTRY_SHADER);
        
        scene::TextSceneObjectData artifactNameText;
        artifactNameText.mText = artifactItemProduct.mStoryRareItemName;
        artifactNameText.mFontName = game_constants::DEFAULT_FONT_NAME;
        
        artifactNameTextSceneObject->mSceneObjectTypeData = std::move(artifactNameText);
        artifactNameTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = shaderCutoffValues.s;
        artifactNameTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = shaderCutoffValues.t;
        artifactNameTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        artifactNameTextSceneObject->mScale = ARTIFACT_TEXT_SCALE;
        artifactNameTextSceneObject->mPosition += ARTIFACT_NAME_TEXT_OFFSET;
        
        ItemEntry itemEntry;
        itemEntry.mArtifactOrMutationName = artifactEntry.first;
        itemEntry.mSceneObjects.push_back(artifactSceneObject);
        itemEntry.mSceneObjects.push_back(artifactNameTextSceneObject);
        
        if (artifactItemProduct.mUnique)
        {
            auto artifactUniqueIconSceneObject = mScene->CreateSceneObject();
            artifactUniqueIconSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + UNIQUE_ARTIFACT_ICON_SHADER_FILE_NAME);
            artifactUniqueIconSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + UNIQUE_ARTIFACT_ICON_TEXTURE_FILE_NAME);
            artifactUniqueIconSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = shaderCutoffValues.s;
            artifactUniqueIconSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = shaderCutoffValues.t;
            artifactUniqueIconSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            artifactUniqueIconSceneObject->mScale = ITEM_ENTRY_SCALE;
            artifactUniqueIconSceneObject->mScale = ARTIFACT_UNIQUE_ICON_SCALE;
            artifactUniqueIconSceneObject->mPosition += ARTIFACT_UNIQUE_ICON_OFFSET;
            
            itemEntry.mSceneObjects.push_back(artifactUniqueIconSceneObject);
        }
        else
        {
            auto artifactCountTextSceneObject = mScene->CreateSceneObject();
            artifactCountTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + ARTIFACT_TEXT_ITEM_ENTRY_SHADER);
            
            scene::TextSceneObjectData artifactCountText;
            artifactCountText.mText = std::to_string(artifactEntry.second) + " x";
            artifactCountText.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            artifactCountTextSceneObject->mSceneObjectTypeData = std::move(artifactCountText);
            artifactCountTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = shaderCutoffValues.s;
            artifactCountTextSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = shaderCutoffValues.t;
            artifactCountTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            artifactCountTextSceneObject->mScale = ARTIFACT_TEXT_SCALE;
            artifactCountTextSceneObject->mPosition += ARTIFACT_COUNT_TEXT_OFFSET;
            
            itemEntry.mSceneObjects.push_back(artifactCountTextSceneObject);
        }
        
        mArtifactsItemContainer->AddItem(std::move(itemEntry), EntryAdditionStrategy::ADD_ON_THE_BACK);
    }
    
    if (mutationCount > 0)
    {
        auto mutationSceneObject = mScene->CreateSceneObject(MUTATION_SCENE_OBJECT_NAME);
        mutationSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + MUTATION_TEXTURE_FILE_NAME);
        mutationSceneObject->mMeshResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + MUTATION_MESH_FILE_NAME);
        mutationSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + MUTATION_SHADER_FILE_NAME);
        mutationSceneObject->mShaderVec3UniformValues[POINT_LIGHT_POSITION_UNIFORM_NAME] = POINT_LIGHT_POSITION;
        mutationSceneObject->mShaderVec3UniformValues[DIFFUSE_COLOR_UNIFORM_NAME] = DIFFUSE_COLOR;
        mutationSceneObject->mShaderVec3UniformValues[SPEC_COLOR_UNIFORM_NAME] = SPEC_COLOR;
        mutationSceneObject->mShaderVec3UniformValues[AMBIENT_COLOR_UNIFORM_NAME] = AMB_COLOR;
        mutationSceneObject->mShaderFloatUniformValues[POINT_LIGHT_POWER_UNIFORM_NAME] = POINT_LIGHT_POWER;
        mutationSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        mutationSceneObject->mShaderBoolUniformValues[AFFECTED_BY_LIGHT_UNIFORM_NAME] = true;
        mutationSceneObject->mScale = MUTATION_SCALE;
        mutationSceneObject->mPosition = MUTATION_POSITION;
        mutationSceneObject->mBoundingRectMultiplier *= 1.5f;
        
        auto mutationCountTextSceneObject = mScene->CreateSceneObject(MUTATION_TEXT_COUNT_SCENE_OBJECT_NAME);
        scene::TextSceneObjectData mutationCountText;
        mutationCountText.mText = std::to_string(mutationCount) + " x";
        mutationCountText.mFontName = game_constants::DEFAULT_FONT_NAME;
        
        mutationCountTextSceneObject->mSceneObjectTypeData = std::move(mutationCountText);
        mutationCountTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        mutationCountTextSceneObject->mScale = MUTATION_COUNT_TEXT_SCALE;
        mutationCountTextSceneObject->mPosition = MUTATION_TEXT_POSITION;
    }
    
    // Toggle mutations title off if necessary
    mScene->FindSceneObject(MUTATIONS_TITLE_SCENE_OBJECT_NAME)->mInvisible = mutationCount == 0;
    
    // Append item count to containers
    std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(ARTIFACTS_TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = "Artifacts (" + std::to_string(artifactCount) + ")";
    std::get<scene::TextSceneObjectData>(mScene->FindSceneObject(MUTATIONS_TITLE_SCENE_OBJECT_NAME)->mSceneObjectTypeData).mText = "Mutations";
    
    // If container doesn't exist the staggered fade in will happen automatically at the end of VInitScene
    if (containerExists)
    {
        // Staggered Item Presentation
        size_t sceneObjectIndex = 0;
        for (const auto& containerItems: mArtifactsItemContainer->GetItems())
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

void InventorySceneLogicManager::CreateItemTooltip(const glm::vec3& itemOriginPostion, const std::string& tooltipText)
{
    bool shouldBeHorFlipped = itemOriginPostion.x > 0.0f;
    bool shouldBeVerFlipped = itemOriginPostion.y > 0.0f;
    
    mItemTooltipController = std::make_unique<CardTooltipController>
    (
        itemOriginPostion + ITEM_TOOLTIP_POSITION_OFFSET,
        ITEM_TOOLTIP_BASE_SCALE,
        tooltipText,
        false,
        shouldBeHorFlipped,
        shouldBeVerFlipped,
        *mScene
    );
}

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::DestroyItemTooltip()
{
    if (mItemTooltipController)
    {
        for (auto sceneObject: mItemTooltipController->GetSceneObjects())
        {
            mScene->RemoveSceneObject(sceneObject->mName);
        }
    }
    
    mItemTooltipController = nullptr;
}

///------------------------------------------------------------------------------------------------

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <imgui/backends/imgui_impl_sdl2.h>
    #define CREATE_DEBUG_WIDGETS
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #undef CREATE_DEBUG_WIDGETS
    #else
        #include <imgui/backends/imgui_impl_sdl2.h>
        #define CREATE_DEBUG_WIDGETS
    #endif
#endif

#if ((!defined(NDEBUG)) || defined(IMGUI_IN_RELEASE)) && (defined(CREATE_DEBUG_WIDGETS))
void InventorySceneLogicManager::VCreateDebugWidgets()
{
//    bool interactedWithSlider = false;
//
//    ImGui::Begin("Swipeable Container", nullptr, GLOBAL_IMGUI_WINDOW_FLAGS);
//
//    ImGui::SeparatorText("Shader Cutoff Values");
//    interactedWithSlider |= ImGui::SliderFloat("Shader Cutoff Min", &MUTATION_ITEM_ENTRY_CUTOFF_VALUES.x, -1.0f, 1.0f);
//    interactedWithSlider |= ImGui::SliderFloat("Shader Cutoff Max", &MUTATION_ITEM_ENTRY_CUTOFF_VALUES.y, -1.0f, 1.0f);
//
//    ImGui::SeparatorText("Container Cutoff Values");
//    interactedWithSlider |= ImGui::SliderFloat("Container Cutoff Min", &MUTATION_ITEM_CONTAINER_CUTOFF_VALUES.x, -1.0f, 1.0f);
//    interactedWithSlider |= ImGui::SliderFloat("Container Cutoff Max", &MUTATION_ITEM_CONTAINER_CUTOFF_VALUES.y, -1.0f, 1.0f);
//
//    ImGui::SeparatorText("Container Hor Bounds");
//    interactedWithSlider |= ImGui::SliderFloat("Container Hor Min", &MUTATION_ITEM_CONTAINER_BOUNDS.bottomLeft.x, -1.0f, 1.0f);
//    interactedWithSlider |= ImGui::SliderFloat("Container Hor Max", &MUTATION_ITEM_CONTAINER_BOUNDS.topRight.x, -1.0f, 1.0f);
//
//    ImGui::SeparatorText("Container Ver Bounds");
//    interactedWithSlider |= ImGui::SliderFloat("Container Ver Min", &MUTATION_ITEM_CONTAINER_BOUNDS.bottomLeft.y, -1.0f, 1.0f);
//    interactedWithSlider |= ImGui::SliderFloat("Container Ver Max", &MUTATION_ITEM_CONTAINER_BOUNDS.topRight.y, -1.0f, 1.0f);
//
//    ImGui::End();
//
//    if (interactedWithSlider)
//    {
//        CreateItemEntriesAndContainer();
//    }
}
#else
void InventorySceneLogicManager::VCreateDebugWidgets()
{
}
#endif

///------------------------------------------------------------------------------------------------

void InventorySceneLogicManager::UpdateMutationInteraction(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    auto mutationSceneObject = scene->FindSceneObject(MUTATION_SCENE_OBJECT_NAME);
    mutationSceneObject->mRotation.y += dtMillis * MUTATION_ROTATION_SPEED;
    auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*mutationSceneObject);
    auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(scene->GetCamera().GetViewMatrix(), scene->GetCamera().GetProjMatrix());
    bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
    if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
    {
        mShowingMutationText = true;
        auto mutationTextOverlaySceneObject = mScene->FindSceneObject(MUTATION_TEXT_OVERLAY_SCENE_OBJECT_NAME);
        mutationTextOverlaySceneObject->mInvisible = false;
        animationManager.StopAllAnimationsPlayingForSceneObject(mutationTextOverlaySceneObject->mName);
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mutationTextOverlaySceneObject, MUTATION_TEXT_OVERLAY_ALPHA, MUTATION_TEXT_OVERLAY_FADE_IN_OUT_DURATION_SECS), [=](){});
        
        mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
        (
            CONTINUE_BUTTON_POSITION,
            BUTTON_SCALE,
            game_constants::DEFAULT_FONT_NAME,
            "Continue",
            MUTATION_TEXT_CONTINUE_BUTTON_NAME,
            [=]()
            {
                auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
                
                animationManager.StopAllAnimationsPlayingForSceneObject(MUTATION_TEXT_CONTINUE_BUTTON_NAME);
                animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mAnimatedButtons.back()->GetSceneObject(), 0.0f, MUTATION_TEXT_CONTINUE_BUTTON_FADE_IN_OUT_DURATION_SECS), [=](){});
                
                animationManager.StopAllAnimationsPlayingForSceneObject(mutationTextOverlaySceneObject->mName);
                animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mutationTextOverlaySceneObject, 0.0f, MUTATION_TEXT_OVERLAY_FADE_IN_OUT_DURATION_SECS), [=]()
                {
                    for (auto i = 0; i < DataRepository::GetInstance().GetCurrentStoryMutationLevel(); ++i)
                    {
                        mScene->RemoveSceneObject(strutils::StringId(MUTATION_TEXT_NAME_PREFIX + std::to_string(i)));
                    }
                    
                    mScene->RemoveSceneObject(MUTATION_TEXT_CONTINUE_BUTTON_NAME);
                    mAnimatedButtons.pop_back();
                    mutationTextOverlaySceneObject->mInvisible = true;
                    mShowingMutationText = false;
                });
                
                for (auto i = 0; i < DataRepository::GetInstance().GetCurrentStoryMutationLevel(); ++i)
                {
                    auto mutationTextOverlaySceneObject = mScene->FindSceneObject(strutils::StringId(MUTATION_TEXT_NAME_PREFIX + std::to_string(i)));
                    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mutationTextOverlaySceneObject, 0.0f, MUTATION_TEXT_CONTINUE_BUTTON_FADE_IN_OUT_DURATION_SECS), [](){});
                }
            },
            *scene
        ));
        
        mAnimatedButtons.back()->GetSceneObject()->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mAnimatedButtons.back()->GetSceneObject(), 1.0f, MUTATION_TEXT_CONTINUE_BUTTON_FADE_IN_OUT_DURATION_SECS), [=](){});
        
        for (auto i = 0; i < DataRepository::GetInstance().GetCurrentStoryMutationLevel(); ++i)
        {
            mScene->RemoveSceneObject(strutils::StringId(MUTATION_TEXT_NAME_PREFIX + std::to_string(i)));
        }
        
        for (auto i = 0; i < DataRepository::GetInstance().GetCurrentStoryMutationLevel(); ++i)
        {
            scene::TextSceneObjectData textMutationChange;
            textMutationChange.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            auto text = game_constants::MUTATION_TEXTS[i];
            for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
            {
                strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), text);
            }
            
            textMutationChange.mText = std::string(1, symbolic_glyph_names::SYMBOLIC_NAMES.at(symbolic_glyph_names::SKULL)) + text;
            
            auto mutationChangeSceneObject = scene->CreateSceneObject(strutils::StringId(MUTATION_TEXT_NAME_PREFIX + std::to_string(i)));
            mutationChangeSceneObject->mSceneObjectTypeData = std::move(textMutationChange);
            mutationChangeSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            mutationChangeSceneObject->mPosition = MUTATION_CHANGE_TEXT_INIT_POSITION - glm::vec3(0.0f, i * 0.04f, 0.0f);
            mutationChangeSceneObject->mScale = MUTATION_CHANGE_TEXT_SCALE;
            
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(mutationChangeSceneObject, 1.0f, MUTATION_TEXT_CONTINUE_BUTTON_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, i * 0.1f), [=](){});
        }
    }
}

///------------------------------------------------------------------------------------------------
