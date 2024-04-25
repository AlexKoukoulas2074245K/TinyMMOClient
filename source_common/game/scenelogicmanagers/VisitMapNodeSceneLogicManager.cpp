///------------------------------------------------------------------------------------------------
///  VisitMapNodeSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 14/12/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/scene/SceneManager.h>
#include <game/AnimatedButton.h>
#include <game/ArtifactProductIds.h>
#include <game/Cards.h>
#include <game/events/EventSystem.h>
#include <game/DataRepository.h>
#include <game/scenelogicmanagers/VisitMapNodeSceneLogicManager.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

static const std::string CUSTOM_COLOR_SHADER_FILE_NAME = "basic_custom_color.vs";
static const std::string SKIP_BUTTON_ICON_SHADER_FILE_NAME = "rare_item.vs";
static const std::string SKIP_BUTTON_ICON_TEXTURE_FILE_NAME = "rare_item_rewards/bunny_hop.png";

static const strutils::StringId BUNNY_HOP_SCENE_NAME = strutils::StringId("bunny_hop_scene");
static const strutils::StringId VISIT_MAP_NODE_SCENE_NAME = strutils::StringId("visit_map_node_scene");
static const strutils::StringId NODE_DESCRIPTION_TEXT_SCENE_OBJECT_NAME = strutils::StringId("node_description_text");
static const strutils::StringId SKIP_BUTTON_ICON_SCENE_OBJECT_NAME = strutils::StringId("skip_button_icon");
static const strutils::StringId VISIT_BUTTON_NAME = strutils::StringId("visit_button");
static const strutils::StringId BACK_BUTTON_NAME = strutils::StringId("back_button");
static const strutils::StringId SKIP_BUTTON_NAME = strutils::StringId("skip_button");

static const glm::vec3 BUTTON_SCALE = {0.0004f, 0.0004f, 0.0004f};
static const glm::vec3 WHITE_NODE_DESC_COLOR = {0.96f, 0.96f, 0.96f};
static const glm::vec3 RED_NODE_DESC_COLOR = {0.86f, 0.1f, 0.1f};
static const glm::vec3 DARK_ORANGE_NODE_DESC_COLOR = {0.9f, 0.27f, 0.125f};
static const glm::vec3 ORANGE_NODE_DESC_COLOR = {0.96f, 0.47f, 0.25f};
static const glm::vec3 PURPLE_NODE_DESC_COLOR = {0.66f, 0.35f, 1.0f};
static const glm::vec3 SKIP_BUTTON_ICON_SCALE = {0.1f, 0.1f, 1.0f};

static const glm::vec2 NODE_DESC_MIN_MAX_X_OFFSETS = {-0.1f, -0.23f};
static const glm::vec2 NODE_DESC_MIN_MAX_Y_OFFSETS = {0.14f, -0.11f};

static const float VISIT_BUTTON_HOR_DISTANCE_FROM_NODE = 0.1f;
static const float VISIT_BUTTON_Y_OFFSET_FROM_NODE = 0.05f;
static const float VISIT_BUTTON_Y_OFFSET_FROM_NODE_WITH_BUNNY_HOP = 0.07f;

static const float BACK_BUTTON_HOR_DISTANCE_FROM_NODE = 0.1f;
static const float BACK_BUTTON_Y_OFFSET_FROM_NODE = -0.03f;
static const float BACK_BUTTON_Y_OFFSET_FROM_NODE_WITH_BUNNY_HOP = -0.05f;

static const float SKIP_BUTTON_HOR_DISTANCE_FROM_NODE = 0.1f;
static const float SKIP_BUTTON_Y_OFFSET_FROM_NODE = 0.011f;
static const float SKIP_BUTTON_ICON_HOR_DISTANCE_FROM_NODE = 0.13f;
static const float SKIP_BUTTON_ICON_Y_OFFSET_FROM_NODE = -0.0005f;

static const float BUTTON_Z = 24.0f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;
static const float FADE_IN_OUT_DURATION_SECS = 0.25f;

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    VISIT_MAP_NODE_SCENE_NAME
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    game_constants::OVERLAY_SCENE_OBJECT_NAME
};

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& VisitMapNodeSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

VisitMapNodeSceneLogicManager::VisitMapNodeSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

VisitMapNodeSceneLogicManager::~VisitMapNodeSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void VisitMapNodeSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene> scene)
{
    scene->GetCamera().SetPosition(CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(mPreviousScene)->GetCamera().GetPosition());
}

///------------------------------------------------------------------------------------------------

void VisitMapNodeSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mTransitioning = false;
    
    scene->RemoveAllSceneObjectsButTheOnesNamed(STATIC_SCENE_ELEMENTS);
    
    mAnimatedButtons.clear();
    
    auto& targetNodePosition = DataRepository::GetInstance().GetSelectedStoryMapNodePosition();
    auto& previousSceneCameraPosition = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(mPreviousScene)->GetCamera().GetPosition();
    
    auto* selectedNodeData = DataRepository::GetInstance().GetSelectedStoryMapNodeData();
    bool shouldShowSkipNodeAction = DataRepository::GetInstance().GetStoryArtifactCount(artifacts::BUNNY_HOP) > 0 &&
                       selectedNodeData->mNodeType != StoryMap::NodeType::BOSS_ENCOUNTER &&
                       selectedNodeData->mNodeType != StoryMap::NodeType::SHOP &&
                       (selectedNodeData->mCoords != game_constants::TUTORIAL_MAP_BOSS_COORD || DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP);
    
    // Don't visit Tent node
    if (DataRepository::GetInstance().GetSelectedStoryMapNodeData()->mCoords != DataRepository::GetInstance().GetCurrentStoryMapNodeCoord())
    {
        auto visitButtonPosition = targetNodePosition;
        
        visitButtonPosition.x += (targetNodePosition.x < previousSceneCameraPosition.x ? VISIT_BUTTON_HOR_DISTANCE_FROM_NODE : -1.5f * VISIT_BUTTON_HOR_DISTANCE_FROM_NODE);
        visitButtonPosition.y += shouldShowSkipNodeAction ? VISIT_BUTTON_Y_OFFSET_FROM_NODE_WITH_BUNNY_HOP : VISIT_BUTTON_Y_OFFSET_FROM_NODE;
        visitButtonPosition.z = BUTTON_Z;
        mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
        (
            visitButtonPosition,
            BUTTON_SCALE,
            game_constants::DEFAULT_FONT_NAME,
            "Visit",
            VISIT_BUTTON_NAME,
            [=]()
            {
                mTransitioning = true;
                InitializeNodeVisitData();
            },
            *scene
        ));
    }
    
    // Bunny Hop
    if (shouldShowSkipNodeAction)
    {
        auto skipButtonPosition = targetNodePosition;
        skipButtonPosition.x += (targetNodePosition.x < previousSceneCameraPosition.x ? SKIP_BUTTON_HOR_DISTANCE_FROM_NODE : -1.5f * BACK_BUTTON_HOR_DISTANCE_FROM_NODE);
        skipButtonPosition.y += SKIP_BUTTON_Y_OFFSET_FROM_NODE;
        skipButtonPosition.z = BUTTON_Z;
        mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
        (
            skipButtonPosition,
            BUTTON_SCALE,
            game_constants::DEFAULT_FONT_NAME,
            "Skip",
            SKIP_BUTTON_NAME,
            [=]()
            {
                mTransitioning = true;
                SkipNode();
            },
            *scene
        ));
        
        auto skipButtonIconPosition = targetNodePosition;
        skipButtonIconPosition.x += (targetNodePosition.x < previousSceneCameraPosition.x ? 1.4f * SKIP_BUTTON_ICON_HOR_DISTANCE_FROM_NODE : -1.5f * SKIP_BUTTON_ICON_HOR_DISTANCE_FROM_NODE);
        skipButtonIconPosition.y += SKIP_BUTTON_ICON_Y_OFFSET_FROM_NODE;
        skipButtonIconPosition.z = BUTTON_Z;
        
        auto skipButtonIconSceneObject = scene->CreateSceneObject(SKIP_BUTTON_ICON_SCENE_OBJECT_NAME);
        skipButtonIconSceneObject->mPosition = skipButtonIconPosition;
        skipButtonIconSceneObject->mScale = SKIP_BUTTON_ICON_SCALE;
        skipButtonIconSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + SKIP_BUTTON_ICON_TEXTURE_FILE_NAME);
        skipButtonIconSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SKIP_BUTTON_ICON_SHADER_FILE_NAME);
    }
    
    auto backButtonPosition = targetNodePosition;
    backButtonPosition.x += (targetNodePosition.x < previousSceneCameraPosition.x ? BACK_BUTTON_HOR_DISTANCE_FROM_NODE : -1.5f * BACK_BUTTON_HOR_DISTANCE_FROM_NODE);
    backButtonPosition.y += shouldShowSkipNodeAction ? BACK_BUTTON_Y_OFFSET_FROM_NODE_WITH_BUNNY_HOP : BACK_BUTTON_Y_OFFSET_FROM_NODE;
    backButtonPosition.z = BUTTON_Z;
    mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        backButtonPosition,
        BUTTON_SCALE,
        game_constants::DEFAULT_FONT_NAME,
        "Back",
        BACK_BUTTON_NAME,
        [=]()
        {
            mTransitioning = true;
            events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
        },
        *scene
    ));
    
    auto nodeDescriptionSceneObject = scene->CreateSceneObject(NODE_DESCRIPTION_TEXT_SCENE_OBJECT_NAME);
    nodeDescriptionSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CUSTOM_COLOR_SHADER_FILE_NAME);
    
    scene::TextSceneObjectData textDataNodeDescription;
    textDataNodeDescription.mFontName = game_constants::DEFAULT_FONT_NAME;
    
    const auto effectiveNodeType = DataRepository::GetInstance().GetSelectedStoryMapNodeData()->mCoords == DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() ? StoryMap::NodeType::STARTING_LOCATION : DataRepository::GetInstance().GetSelectedStoryMapNodeData()->mNodeType;
    switch(effectiveNodeType)
    {
        case StoryMap::NodeType::NORMAL_ENCOUNTER:
        {
            nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = WHITE_NODE_DESC_COLOR;
            textDataNodeDescription.mText = "Normal Encounter";
        } break;
        
        case StoryMap::NodeType::ELITE_ENCOUNTER:
        {
            textDataNodeDescription.mText = "Elite Encounter";
            nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = ORANGE_NODE_DESC_COLOR;
            
            if (DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP && DataRepository::GetInstance().GetSelectedStoryMapNodeData()->mCoords == game_constants::TUTORIAL_MAP_BOSS_COORD)
            {
                textDataNodeDescription.mText = "Mini Boss Encounter";
                nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = DARK_ORANGE_NODE_DESC_COLOR;
            }
        } break;
        
        case StoryMap::NodeType::EVENT:
        {
            textDataNodeDescription.mText = "Random Event";
            nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = PURPLE_NODE_DESC_COLOR;
        } break;
        
        case StoryMap::NodeType::BOSS_ENCOUNTER:
        {
            textDataNodeDescription.mText = "Boss Encounter";
            nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = RED_NODE_DESC_COLOR;
        } break;
        
        case StoryMap::NodeType::SHOP:
        {
            textDataNodeDescription.mText = "Merchant Encounter";
            nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = PURPLE_NODE_DESC_COLOR;
        } break;
        
        case StoryMap::NodeType::STARTING_LOCATION:
        {
            textDataNodeDescription.mText = "Your Tent!";
            nodeDescriptionSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = WHITE_NODE_DESC_COLOR;
        }break;
        default: break;
    }
    
    nodeDescriptionSceneObject->mSceneObjectTypeData = std::move(textDataNodeDescription);
    nodeDescriptionSceneObject->mPosition = targetNodePosition;
    nodeDescriptionSceneObject->mPosition.x += (targetNodePosition.x < previousSceneCameraPosition.x ? NODE_DESC_MIN_MAX_X_OFFSETS.s : NODE_DESC_MIN_MAX_X_OFFSETS.t);
    nodeDescriptionSceneObject->mPosition.y += (targetNodePosition.y < previousSceneCameraPosition.y ? NODE_DESC_MIN_MAX_Y_OFFSETS.s : NODE_DESC_MIN_MAX_Y_OFFSETS.t);
    
    if (effectiveNodeType == StoryMap::NodeType::SHOP)
    {
        nodeDescriptionSceneObject->mPosition.y = targetNodePosition.y + NODE_DESC_MIN_MAX_Y_OFFSETS.t;
    }
    
    nodeDescriptionSceneObject->mPosition.z = BUTTON_Z;
    nodeDescriptionSceneObject->mScale = BUTTON_SCALE;
    
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
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
        });
    }
}

///------------------------------------------------------------------------------------------------

void VisitMapNodeSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene>)
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

void VisitMapNodeSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == game_constants::OVERLAY_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, FADE_IN_OUT_DURATION_SECS), [=]()
        {
            sceneObject->mInvisible = true;
        });
    }
    
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto previousScene = sceneManager.FindScene(mPreviousScene);
    
    animationManager.StopAnimation(game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(previousScene->GetUpdateTimeSpeedFactor(), 1.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> VisitMapNodeSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void VisitMapNodeSceneLogicManager::SkipNode()
{
    auto* selectedNodeData = DataRepository::GetInstance().GetSelectedStoryMapNodeData();
    
    assert(selectedNodeData);
    
    auto currentStoryArtifacts = DataRepository::GetInstance().GetCurrentStoryArtifacts();
    currentStoryArtifacts.erase(std::remove_if(currentStoryArtifacts.begin(), currentStoryArtifacts.end(), [](const std::pair<strutils::StringId, int>& artifactEntry)
    {
        return artifactEntry.first == artifacts::BUNNY_HOP;
    }), currentStoryArtifacts.end());
    DataRepository::GetInstance().SetCurrentStoryArtifacts(currentStoryArtifacts);
    
    
    DataRepository::GetInstance().SetCurrentStoryMapNodeCoord(selectedNodeData->mCoords);
    DataRepository::GetInstance().FlushStateToFile();
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(BUNNY_HOP_SCENE_NAME, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

void VisitMapNodeSceneLogicManager::InitializeNodeVisitData()
{
    auto* selectedNodeData = DataRepository::GetInstance().GetSelectedStoryMapNodeData();
    
    assert(selectedNodeData);
    assert(selectedNodeData->mNodeRandomSeed != 0);
    
    DataRepository::GetInstance().SetCurrentStoryMapNodeSeed(selectedNodeData->mNodeRandomSeed);
    DataRepository::GetInstance().SetCurrentStoryMapNodeCoord(selectedNodeData->mCoords);
    DataRepository::GetInstance().SetCurrentStoryMapNodeType(selectedNodeData->mNodeType);
    
    std::vector<int> opponentDeckBuilder;
    bool isEliteOrTutorialBossFightEncounter = false;
    bool isFinalBossFightEncounter = false;
    
    switch (selectedNodeData->mNodeType)
    {
        case StoryMap::NodeType::EVENT:
        {
            DataRepository::GetInstance().SetCurrentEventIndex(-1);
            DataRepository::GetInstance().SetCurrentEventScreenIndex(0);
            events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::EVENT_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
        } break;
        
        case StoryMap::NodeType::SHOP:
        {
            DataRepository::GetInstance().ClearShopBoughtProductCoordinates();
            DataRepository::GetInstance().SetCurrentShopBehaviorType(ShopBehaviorType::STORY_SHOP);
            events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::SHOP_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
        } break;
            
        case StoryMap::NodeType::BOSS_ENCOUNTER:
        {
            auto eliteCards = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::DEMONS_HARD_FAMILY_NAME);
            opponentDeckBuilder.insert(opponentDeckBuilder.end(), eliteCards.begin(), eliteCards.end());
            isFinalBossFightEncounter = true;
        } // Intentional fallthrough
        case StoryMap::NodeType::ELITE_ENCOUNTER:
        {
            if (DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP || DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD)
            {
                if (!isFinalBossFightEncounter)
                {
                    auto mediumCards = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::DEMONS_MEDIUM_FAMILY_NAME);
                    opponentDeckBuilder.insert(opponentDeckBuilder.end(), mediumCards.begin(), mediumCards.end());
                    isEliteOrTutorialBossFightEncounter = true;
                }
            }
        } // Intentional fallthrough
        case StoryMap::NodeType::NORMAL_ENCOUNTER:
        {
            if (!isEliteOrTutorialBossFightEncounter && !isFinalBossFightEncounter)
            {
                auto normalCards = CardDataRepository::GetInstance().GetCardIdsByFamily(game_constants::DEMONS_NORMAL_FAMILY_NAME);
                opponentDeckBuilder.insert(opponentDeckBuilder.end(), normalCards.begin(), normalCards.end());
            }
            
            // Populate opponent deck and battle control type
            DataRepository::GetInstance().SetNextTopPlayerDeck(opponentDeckBuilder);
            DataRepository::GetInstance().SetNextBattleControlType(BattleControlType::AI_TOP_ONLY);
            
            // Populate opponent hero card name & texture
            auto storyMapScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::STORY_MAP_SCENE);
            auto nodePortraitSceneObject = storyMapScene->FindSceneObject(strutils::StringId(MapCoord(selectedNodeData->mCoords.x, selectedNodeData->mCoords.y).ToString() + game_constants::STORY_MAP_NODE_PORTRAIT_SO_NAME_POST_FIX));
            auto nodeHealthTextSceneObject = storyMapScene->FindSceneObject(strutils::StringId(MapCoord(selectedNodeData->mCoords.x, selectedNodeData->mCoords.y).ToString() + game_constants::STORY_MAP_NODE_HEALTH_TEXT_SO_NAME_POST_FIX));
            auto nodeDamageTextSceneObject = storyMapScene->FindSceneObject(strutils::StringId(MapCoord(selectedNodeData->mCoords.x, selectedNodeData->mCoords.y).ToString() + game_constants::STORY_MAP_NODE_DAMAGE_TEXT_SO_NAME_POST_FIX));
            auto nodeWeightTextSceneObject = storyMapScene->FindSceneObject(strutils::StringId(MapCoord(selectedNodeData->mCoords.x, selectedNodeData->mCoords.y).ToString() + game_constants::STORY_MAP_NODE_WEIGHT_TEXT_SO_NAME_POST_FIX));
            auto nodeNameTextSceneObject = storyMapScene->FindSceneObject(strutils::StringId(MapCoord(selectedNodeData->mCoords.x, selectedNodeData->mCoords.y).ToString() + game_constants::STORY_MAP_NODE_TEXT_SO_NAME_POST_FIX));
            
            DataRepository::GetInstance().SetNextStoryOpponentTexturePath(CoreSystemsEngine::GetInstance().GetResourceLoadingService().GetResourcePath(nodePortraitSceneObject->mTextureResourceId));
            DataRepository::GetInstance().SetNextStoryOpponentName(std::get<scene::TextSceneObjectData>(nodeNameTextSceneObject->mSceneObjectTypeData).mText);
            DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::BATTLE);
            
            // Populate opponent stats
            DataRepository::GetInstance().SetNextStoryOpponentDamage(std::stoi(std::get<scene::TextSceneObjectData>(nodeDamageTextSceneObject->mSceneObjectTypeData).mText));
            DataRepository::GetInstance().SetNextBattleTopPlayerHealth(std::stoi(std::get<scene::TextSceneObjectData>(nodeHealthTextSceneObject->mSceneObjectTypeData).mText));
            DataRepository::GetInstance().SetNextBattleTopPlayerInitWeight(std::stoi(std::get<scene::TextSceneObjectData>(nodeWeightTextSceneObject->mSceneObjectTypeData).mText) - 1);
            DataRepository::GetInstance().SetNextBattleTopPlayerWeightLimit(std::stoi(std::get<scene::TextSceneObjectData>(nodeWeightTextSceneObject->mSceneObjectTypeData).mText));
            
            // Populate local player stats
            DataRepository::GetInstance().SetNextBotPlayerDeck(DataRepository::GetInstance().GetCurrentStoryPlayerDeck());
            DataRepository::GetInstance().SetNextBattleBotPlayerHealth(DataRepository::GetInstance().StoryCurrentHealth().GetValue());
            DataRepository::GetInstance().SetNextBattleBotPlayerWeightLimit(game_constants::BOT_PLAYER_DEFAULT_WEIGHT_LIMIT * 2);
            
            events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::BATTLE_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
        } break;
            
        default:
        {
            assert(false);
        } break;
    }
    
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------
