///------------------------------------------------------------------------------------------------
///  BattleSceneLogicManager.cpp
///  Predators
///
///  Created by Alex Koukoulas on 11/10/2023
///------------------------------------------------------------------------------------------------

#include <game/AnimatedButton.h>
#include <game/AnimatedStatContainer.h>
#include <game/ArtifactProductIds.h>
#include <game/BoardState.h>
#include <game/CardUtils.h>
#include <game/events/EventSystem.h>
#include <game/GameConstants.h>
#include <game/GameRuleEngine.h>
#include <game/GuiObjectManager.h>
#include <game/utils/BattleDeserializer.h>
#include <game/utils/BattleSerializer.h>
#include <game/gameactions/BattleInitialSetupAndAnimationGameAction.h>
#include <game/gameactions/PlayCardGameAction.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/gameactions/PlayerActionGenerationEngine.h>
#include <game/ProductRepository.h>
#include <game/scenelogicmanagers/BattleSceneLogicManager.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/rendering/Fonts.h>
#include <engine/resloading/MeshResource.h>
#include <engine/scene/SceneManager.h>
#include <engine/scene/Scene.h>
#include <engine/scene/SceneObject.h>
#include <engine/scene/SceneObjectUtils.h>
#include <engine/sound/SoundManager.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#if defined(MOBILE_FLOW)
#include <platform_specific/IOSUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static const strutils::StringId HISTORY_SCENE = strutils::StringId("battle_history_scene");
static const strutils::StringId CARD_INSPECTION_SCENE = strutils::StringId("card_inspection_scene");
static const strutils::StringId CARD_HISTORY_CONTAINER_NAME = strutils::StringId("card_history_container");
static const strutils::StringId COINS_LOOT_INDICATOR_SCENE_OBJECT_NAME = strutils::StringId("coins_loot_indicator");
static const strutils::StringId HEALTH_LOOT_INDICATOR_SCENE_OBJECT_NAME = strutils::StringId("health_loot_indicator");
static const strutils::StringId FLAWLESS_VICTORY_INDICATOR_SCENE_OBJECT_NAME = strutils::StringId("flawless_victory_indicator");
static const strutils::StringId HISTORY_TROLLEY_SCENE_OBJECT_NAME = strutils::StringId("history_trolley");
static const strutils::StringId CARD_LOCATION_INDICATOR_SCENE_OBJECT_NAME = strutils::StringId("card_location_indicator");
static const strutils::StringId CARD_HISTORY_CAPSULE_SCENE_OBJECT_NAME = strutils::StringId("card_history_capsule");
static const strutils::StringId CARD_TOOLTIP_SCENE_OBJECT_NAME = strutils::StringId("card_tooltip");
static const strutils::StringId HISTORY_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("history_button");
static const strutils::StringId HISTORY_OVERLAY_SCENE_OBJECT_NAME = strutils::StringId("history_overlay");
static const strutils::StringId REPLAY_TEXT_SCENE_OBJECT_NAME = strutils::StringId("replay_text");
static const strutils::StringId CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME = strutils::StringId("reveal_threshold");
static const strutils::StringId CARD_TOOLTIP_REVEAL_RGB_EXPONENT_UNIFORM_NAME = strutils::StringId("reveal_rgb_exponent");
static const strutils::StringId IDLE_GAME_ACTION_NAME = strutils::StringId("IdleGameAction");
static const strutils::StringId BATTLE_INITIAL_SETUP_AND_ANIMATION_GAME_ACTION_NAME = strutils::StringId("BattleInitialSetupAndAnimationGameAction");
static const strutils::StringId PLAY_CARD_ACTION_NAME = strutils::StringId("PlayCardGameAction");
static const strutils::StringId NEXT_PLAYER_ACTION_NAME = strutils::StringId("NextPlayerGameAction");
static const strutils::StringId CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME = strutils::StringId("CardBuffedDebuffedAnimationGameAction");
static const strutils::StringId CARD_EFFECT_GAME_ACTION_NAME = strutils::StringId("CardEffectGameAction");
static const strutils::StringId GAME_OVER_GAME_ACTION_NAME = strutils::StringId("GameOverGameAction");
static const strutils::StringId HERO_CARD_ENTRY_GAME_ACTION_NAME = strutils::StringId("HeroCardEntryGameAction");
static const strutils::StringId CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES [game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT] =
{
    strutils::StringId("card_tooltip_text_0"),
    strutils::StringId("card_tooltip_text_1"),
    strutils::StringId("card_tooltip_text_2"),
    strutils::StringId("card_tooltip_text_3")
};
static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::BATTLE_SCENE,
    HISTORY_SCENE
};


static const std::string MAKE_SPACE_REVERT_TO_POSITION_ANIMATION_NAME_PREFIX = "make_space_revert_";
static const std::string BATTLE_ICON_TEXTURE_FILE_NAME = "battle_icon.png";
static const std::string TURN_POINTER_TEXTURE_FILE_NAME = "turn_pointer.png";
static const std::string HEALTH_CRYSTAL_TEXTURE_FILE_NAME = "health_icon.png";
static const std::string METALLIC_HEALTH_CRYSTAL_TEXTURE_FILE_NAME = "metal_health_icon.png";
static const std::string WEIGHT_CRYSTAL_TEXTURE_FILE_NAME = "weight_crystal.png";
static const std::string POISON_STACK_TEXTURE_FILE_NAME = "poison_splatter.png";
static const std::string BOARD_SIDE_STAT_EFFECT_SHADER_FILE_NAME = "board_side_stat_effect.vs";
static const std::string CARD_TOOLTIP_TEXTURE_FILE_NAME = "tooltip.png";
static const std::string CARD_TOOLTIP_SHADER_FILE_NAME = "diagonal_reveal.vs";
static const std::string HISTORY_ICON_TEXTURE_FILE_NAME = "history_button_icon.png";
static const std::string HISTORY_OVERLAY_TEXTURE_FILE_NAME = "overlay.png";
static const std::string CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX = "highlighter_card_";
static const std::string HEALTH_CRYSTAL_TOP_SCENE_OBJECT_NAME_PREFIX = "health_crystal_top_";
static const std::string HEALTH_CRYSTAL_BOT_SCENE_OBJECT_NAME_PREFIX = "health_crystal_bot_";
static const std::string ARMOR_CONTAINER_TOP_SCENE_OBJECT_NAME_PREFIX = "armor_health_crystal_top_";
static const std::string ARMOR_CONTAINER_BOT_SCENE_OBJECT_NAME_PREFIX = "armor_health_crystal_bot_";
static const std::string WEIGHT_CRYSTAL_TOP_SCENE_OBJECT_NAME_PREFIX = "weight_crystal_top_";
static const std::string WEIGHT_CRYSTAL_BOT_SCENE_OBJECT_NAME_PREFIX = "weight_crystal_bot_";
static const std::string POISON_STACK_TOP_SCENE_OBJECT_NAME_PREFIX = "poison_stack_top_";
static const std::string POISON_STACK_BOT_SCENE_OBJECT_NAME_PREFIX = "poison_stack_bot_";
static const std::string CARD_HISTORY_ENTRY_SHADER_FILE_NAME = "card_history_entry.vs";
static const std::string TURN_COUNTER_HISTORY_ENTRY_SHADER_FILE_NAME = "turn_counter_history_entry.vs";
static const std::string TURN_COUNTER_STRING_HISTORY_ENTRY_SHADER_FILE_NAME = "turn_counter_string_history_entry.vs";
static const std::string HISTORY_ENTRY_MASK_TEXTURE_FILE_NAME = "history_entry_mask.png";
static const std::string HISTORY_ENTRY_SPELL_MASK_TEXTURE_FILE_NAME = "history_entry_spell_mask.png";
static const std::string HISTORY_ENTRY_TURN_COUNTER_MASK_TEXTURE_FILE_NAME = "history_entry_turn_counter_mask.png";
static const std::string TURN_COUNTER_HISTORY_ENTRY_TEXTURE_FILE_NAME = "history_turn_counter.png";
static const std::string METALLIC_TEXTURE_FILE_NAME = "metallic_texture.png";
static const std::string HEALTH_CHANGE_TEXT_TOP_SCENE_OBJECT_NAME_PREFIX = "health_change_text_top_";
static const std::string HEALTH_CHANGE_TEXT_BOT_SCENE_OBJECT_NAME_PREFIX = "health_change_text_bot_";
static const std::string RARE_ITEM_SHADER = "rare_item.vs";
static const std::string FIREWORKS_SFX = "sfx_fireworks";

static const glm::vec3 BOARD_SIDE_EFFECT_TOP_POSITION = { 0.0f, 0.044f, 1.0f};
static const glm::vec3 BOARD_SIDE_EFFECT_BOT_POSITION = { 0.0f, -0.044f, 1.0f};
static const glm::vec3 CARD_TOOLTIP_SCALE = {0.15f, 0.137f, 1/10.0f};
static const glm::vec3 CARD_TOOLTIP_HISTORY_SCALE = {0.3f, 0.274f, 1/10.0f};
static const glm::vec3 CARD_TOOLTIP_OFFSET = {0.084f, 0.08f, 0.1f};
static const glm::vec3 CARD_TOOLTIP_HISTORY_OFFSET = {0.06f, 0.013f, 0.2f};
static const glm::vec3 HISTORY_BUTTON_POSITION = {0.145f, -0.064f, 19.0f};
static const glm::vec3 HISTORY_BUTTON_SCALE = {0.025f, 0.025f, 0.025f};
static const glm::vec3 CARD_HISTORY_ENTRY_SCALE = {0.3f, -0.3f, 0.3f};
static const glm::vec3 CARD_HISTORY_TURN_COUNTER_ENTRY_SCALE = {0.266f, -0.3f, 0.3f};
static const glm::vec3 CARD_HISTORY_CAPSULE_POSITION = {0.0f, -0.102f, 25.0f};
static const glm::vec3 CARD_HISTORY_TURN_COUNTER_TEXT_OFFSET = {-0.032f, 0.003f, 0.001f};
static const glm::vec3 HEALTH_CHANGE_TEXT_COLOR_GAIN = {0.11f, 0.8f, 0.11f};
static const glm::vec3 HEALTH_CHANGE_TEXT_COLOR_LOSS = {0.8f, 0.11f, 0.11f};
static const glm::vec3 HEALTH_CHANGE_TEXT_SCALE = {0.0002f, 0.0002f, 0.0002f};
static const glm::vec3 HEALTH_CHANGE_TEXT_OFFSET = {-0.04f, 0.0f, 0.01f};
static const glm::vec3 RARE_ITEM_INIT_SCALE = glm::vec3(0.0001f, 0.0001f, 0.0001f);
static const glm::vec3 RARE_ITEM_TARGET_SCALE = glm::vec3(0.15f, 0.15f, 0.15f);
static const glm::vec3 CARD_TOOLTIP_TEXT_OFFSETS[game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT] =
{
    { -0.054f, 0.029f, 0.1f },
    { -0.054f, 0.014f, 0.1f },
    { -0.054f, -0.000f, 0.1f },
    { -0.054f, -0.014f, 0.1f }
};

static const math::Rectangle CARD_HISTORY_CONTAINER_BOUNDS = {{-0.4f, -0.218f}, {0.4f, 0.0f}};
static const glm::vec2 CARD_HISTORY_CONTAINER_CUTOFF_VALUES = {-0.23f, 0.23f};

static const float BOARD_SIDE_EFFECT_SHOWING_HIDING_ANIMATION_DURATION_SECS = 0.5f;
static const float EMPTY_DECK_CARD_TOKEN_NEW_CARD_SCALE_IN_ANIMATION_DURATION_SECS = 0.3f;
static const float CARD_SELECTION_ANIMATION_DURATION = 0.15f;
static const float CARD_HIGHLIGHT_ANIMATION_DURATION = 0.1f;
static const float CARD_LOCATION_EFFECT_MIN_TARGET_ALPHA = 0.25f;
static const float CARD_LOCATION_EFFECT_MAX_TARGET_ALPHA = 1.0f;
static const float CARD_LOCATION_EFFECT_ALPHA_SPEED = 0.003f;
static const float CARD_TOOLTIP_TEXT_FONT_SIZE = 0.00016f;
static const float CARD_TOOLTIP_MAX_REVEAL_THRESHOLD = 2.0f;
static const float CARD_TOOLTIP_REVEAL_SPEED = 1.0f/200.0f;
static const float CARD_TOOLTIP_TEXT_REVEAL_SPEED = 1.0f/500.0f;
static const float CARD_TOOLTIP_FLIPPED_X_OFFSET = -0.17f;
static const float CARD_TOOLTIP_TEXT_FLIPPED_X_OFFSET = -0.002f;
static const float CARD_TOOLTIP_CREATION_DELAY_SECS = 0.5f;
static const float INDIVIDUAL_CARD_BOARD_EFFECT_BASE_Z = 1.1f;
static const float INDIVIDUAL_CARD_BOARD_EFFECT_Z_INCREMENT = 0.01f;
static const float BOARD_EFFECT_MAX_ALPHA = 0.25f;
static const float TURN_POINTER_INTERACTOR_SCALE_FACTOR = 0.5f;
static const float TURN_POINTER_INTERACTION_PULSE_DURATION = 0.1f;
static const float OVERLAY_SCENE_SPEED_ANIMATION_TARGET_DURATION = 0.5f;
static const float CARD_HISTORY_CONTAINER_Z = 24.0f;
static const float HISTORY_SCENE_FADE_IN_OUT_DURATION_SECS = 0.5f;
static const float REPLAY_TEXT_FADE_IN_OUT_DURATION_SECS = 0.5f;
static const float HISTORY_SCENE_FADE_IN_OUT_ITEM_OFFSETS = 0.4f;
static const float HISTORY_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 98.5f;
static const float REPLAY_TEXT_PULSE_SCALE_FACTOR = 1.05f;
static const float REPLAY_TEXT_INTER_PULSE_DURATION_SECS = 1.0f;
static const float REPLAY_TEXT_MAX_ALPHA = 0.75f;
static const float HEALTH_CHANGE_TARGET_Y_OFFSET = 0.05f;
static const float HEALTH_CHANGE_TEXT_ANIMATION_DURATION_SECS = 0.5f;
static const float HEALTH_CHANGE_TEXT_ANIMATION_DELAY_SECS = 0.25f;
static const float RARE_ITEM_Z_OFFSET = 10.0f;
static const float RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS = 1.0f;

#if defined(MOBILE_FLOW)
static const float IPAD_HISTORY_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR = 69.0f;
static const float MOBILE_DISTANCE_FROM_CARD_LOCATION_INDICATOR = 0.003f;
#else
static const float DESKTOP_DISTANCE_FROM_CARD_LOCATION_INDICATOR = 0.003f;
#endif

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& BattleSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

BattleSceneLogicManager::BattleSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

BattleSceneLogicManager::~BattleSceneLogicManager()
{
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    mCurrentBattleControlType = DataRepository::GetInstance().GetNextBattleControlType();
    
    if (scene->GetName() == game_constants::BATTLE_SCENE)
    {
        InitBattleScene(scene);
    }
    else if (scene->GetName() == HISTORY_SCENE)
    {
        InitHistoryScene(scene);
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene> scene)
{
    if (scene->GetName() != HISTORY_SCENE)
    {
        scene->GetCamera().SetZoomFactor(game_constants::GAME_BOARD_BASED_SCENE_ZOOM_FACTOR);
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::InitBattleScene(std::shared_ptr<scene::Scene> scene)
{
    RegisterForEvents();
    CoreSystemsEngine::GetInstance().GetResourceLoadingService().UnloadAllDynamicallyCreatedTextures();
    CardDataRepository::GetInstance().LoadCardData(true);
    
    mPreviousProspectiveBoardCardsPushState = ProspectiveBoardCardsPushState::NONE;
    mSecsCardHighlighted = 0.0f;
    mShouldShowCardLocationIndicator = false;
    mCanPlayNextCard = false;
    mCanIssueNextTurnInteraction = false;
    mCanInteractWithAnyHeldCard = true;
    mPendingCardReleasedThisFrame = nullptr;
    
    mBattleSceneAnimatedButtons.clear();
    mActiveIndividualCardBoardEffectSceneObjects.clear();
    mPlayerHeldCardSceneObjectWrappers.clear();
    mPlayerBoardCardSceneObjectWrappers.clear();
    mAnimatedStatContainers.clear();
    mPendingCardsToBePlayed.clear();
    
    mBoardState = std::make_unique<BoardState>();
    mBoardState->GetPlayerStates().emplace_back();
    mBoardState->GetPlayerStates().emplace_back();
    
    mGuiManager = std::make_shared<GuiObjectManager>(scene);
    
    auto* quickPlayData = DataRepository::GetInstance().GetQuickPlayData();
    if (quickPlayData)
    {
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerHealth = game_constants::TOP_PLAYER_DEFAULT_HEALTH;
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth = game_constants::BOT_PLAYER_DEFAULT_HEALTH;
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerTotalWeightAmmo = game_constants::TOP_PLAYER_DEFAULT_WEIGHT;
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerTotalWeightAmmo = game_constants::BOT_PLAYER_DEFAULT_WEIGHT;
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerCurrentWeightAmmo = game_constants::TOP_PLAYER_DEFAULT_WEIGHT;
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerCurrentWeightAmmo = game_constants::BOT_PLAYER_DEFAULT_WEIGHT;
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerWeightAmmoLimit = game_constants::TOP_PLAYER_DEFAULT_WEIGHT_LIMIT;
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerWeightAmmoLimit = game_constants::TOP_PLAYER_DEFAULT_WEIGHT_LIMIT;
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards = quickPlayData->mTopPlayerDeck;
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards = quickPlayData->mBotPlayerDeck;
        mCurrentBattleControlType = quickPlayData->mBattleControlType;
        
        scene->FindSceneObject(game_constants::GUI_STORY_CARDS_BUTTON_SCENE_OBJECT_NAME)->mInvisible = true;
        scene->FindSceneObject(game_constants::GUI_INVENTORY_BUTTON_SCENE_OBJECT_NAME)->mInvisible = true;
    }
    else
    {
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerHealth = DataRepository::GetInstance().GetNextBattleTopPlayerHealth();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth = DataRepository::GetInstance().GetNextBattleBotPlayerHealth();
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerTotalWeightAmmo = DataRepository::GetInstance().GetNextBattleTopPlayerInitWeight();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerTotalWeightAmmo = DataRepository::GetInstance().GetNextBattleBotPlayerInitWeight();
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerCurrentWeightAmmo = DataRepository::GetInstance().GetNextBattleTopPlayerInitWeight();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerCurrentWeightAmmo = DataRepository::GetInstance().GetNextBattleBotPlayerInitWeight();
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerWeightAmmoLimit = DataRepository::GetInstance().GetNextBattleTopPlayerWeightLimit();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerWeightAmmoLimit = DataRepository::GetInstance().GetNextBattleBotPlayerWeightLimit();
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards = DataRepository::GetInstance().GetNextTopPlayerDeck();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards = DataRepository::GetInstance().GetNextBotPlayerDeck();
    }
    
    mActiveIndividualCardBoardEffectSceneObjects.emplace_back();
    mActiveIndividualCardBoardEffectSceneObjects.emplace_back();
    
    mPlayerHeldCardSceneObjectWrappers.emplace_back();
    mPlayerHeldCardSceneObjectWrappers.emplace_back();
    
    mPlayerBoardCardSceneObjectWrappers.emplace_back();
    mPlayerBoardCardSceneObjectWrappers.emplace_back();
    
    mRuleEngine = std::make_unique<GameRuleEngine>(mBoardState.get());

    auto seed = quickPlayData ? math::RandomInt() : DataRepository::GetInstance().GetCurrentStoryMapNodeSeed();
    std::unique_ptr<BattleDeserializer> replayEngine = nullptr;
    
    if (mCurrentBattleControlType == BattleControlType::REPLAY)
    {
        replayEngine = std::make_unique<BattleDeserializer>();
        seed = quickPlayData ? replayEngine->GetGameFileSeed() : seed;
        
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards = replayEngine->GetTopPlayerDeck();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards = replayEngine->GetBotPlayerDeck();
        mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerHealth = replayEngine->GetTopPlayerStartingHealth();
        mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth = replayEngine->GetBotPlayerStartingHealth();
        
        if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !quickPlayData)
        {
            mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerHealth = DataRepository::GetInstance().GetNextBattleTopPlayerHealth();
            mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth = DataRepository::GetInstance().GetNextBattleBotPlayerHealth();
            
            mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards = DataRepository::GetInstance().GetNextTopPlayerDeck();
            mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards = DataRepository::GetInstance().GetNextBotPlayerDeck();
        }
    }
    
    for (const auto& goldenCardEntry: DataRepository::GetInstance().GetGoldenCardIdMap())
    {
        if (goldenCardEntry.second)
        {
            mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mGoldenCardIds.push_back(goldenCardEntry.first);
        }
    }
        
    CardDataRepository::GetInstance().CleanDeckFromTempIds(mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards);
    CardDataRepository::GetInstance().CleanDeckFromTempIds(mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards);
    
    mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerInitialDeckCards = mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards;
    mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerInitialDeckCards = mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards;
    
    mBattleSerializer = std::make_unique<BattleSerializer>(seed, mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerDeckCards, mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerDeckCards, mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mPlayerHealth, mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth);
    
    mActionEngine = std::make_unique<GameActionEngine>(GameActionEngine::EngineOperationMode::ANIMATED, seed, mBoardState.get(), this, mRuleEngine.get());
    mPlayerActionGenerationEngine = std::make_unique<PlayerActionGenerationEngine>(mRuleEngine.get(), mActionEngine.get(), PlayerActionGenerationEngine::ActionGenerationType::OPTIMISED);
    
    mActionEngine->AddGameAction(BATTLE_INITIAL_SETUP_AND_ANIMATION_GAME_ACTION_NAME, {{ BattleInitialSetupAndAnimationGameAction::CURRENT_BATTLE_SUBSCENE_PARAM, std::to_string(static_cast<int>(DataRepository::GetInstance().GetCurrentBattleSubSceneType())) }});
    
    // Story battle, hero card action needs to be created
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !quickPlayData)
    {
        DataRepository::GetInstance().SetCurrentStoryMapSceneType(StoryMapSceneType::BATTLE);
        mActionEngine->AddGameAction(HERO_CARD_ENTRY_GAME_ACTION_NAME);
    }
    
    if (mCurrentBattleControlType == BattleControlType::REPLAY)
    {
        replayEngine->ReplayActions(mActionEngine.get());
        
        if (mActionEngine->GetActionCount() > 2)
        {
            auto replayTextSceneObject = scene->FindSceneObject(REPLAY_TEXT_SCENE_OBJECT_NAME);
            replayTextSceneObject->mInvisible = false;
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(replayTextSceneObject, REPLAY_TEXT_MAX_ALPHA, REPLAY_TEXT_FADE_IN_OUT_DURATION_SECS), [=](){});
            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(replayTextSceneObject, REPLAY_TEXT_PULSE_SCALE_FACTOR, REPLAY_TEXT_INTER_PULSE_DURATION_SECS, animation_flags::ANIMATE_CONTINUOUSLY), [](){});
        }
    }
    else
    {
        mActionEngine->AddGameAction(NEXT_PLAYER_ACTION_NAME);
    }
    
    mBattleSerializer->FlushStateToFile();
    
    // Stat Containers
    // Health
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::HEALTH_CRYSTAL_TOP_POSITION, HEALTH_CRYSTAL_TEXTURE_FILE_NAME, HEALTH_CRYSTAL_TOP_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[0].mPlayerHealth, false, *scene)));
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::HEALTH_CRYSTAL_BOT_POSITION, HEALTH_CRYSTAL_TEXTURE_FILE_NAME, HEALTH_CRYSTAL_BOT_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[1].mPlayerHealth, false, *scene)));
    
    // Weight
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::WEIGHT_CRYSTAL_TOP_POSITION, WEIGHT_CRYSTAL_TEXTURE_FILE_NAME, WEIGHT_CRYSTAL_TOP_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[0].mPlayerCurrentWeightAmmo, false, *scene)));
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::WEIGHT_CRYSTAL_BOT_POSITION, WEIGHT_CRYSTAL_TEXTURE_FILE_NAME, WEIGHT_CRYSTAL_BOT_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[1].mPlayerCurrentWeightAmmo, false, *scene)));

    // Poison stacks
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::POISON_STACK_TOP_POSITION, POISON_STACK_TEXTURE_FILE_NAME, POISON_STACK_TOP_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[0].mPlayerPoisonStack, true, *scene)));
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::POISON_STACK_BOT_POSITION, POISON_STACK_TEXTURE_FILE_NAME, POISON_STACK_BOT_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[1].mPlayerPoisonStack, true, *scene)));
    
    // Armor
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::ARMOR_CONTAINER_TOP_POSITION, METALLIC_HEALTH_CRYSTAL_TEXTURE_FILE_NAME, ARMOR_CONTAINER_TOP_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[0].mPlayerCurrentArmor, true, *scene)));
    mAnimatedStatContainers.back().second->GetSceneObjects().front()->mShaderBoolUniformValues[game_constants::METALLIC_STAT_CONTAINER_UNIFORM_NAME] = true;
    mAnimatedStatContainers.back().second->GetSceneObjects().front()->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + METALLIC_TEXTURE_FILE_NAME);
    mAnimatedStatContainers.emplace_back(std::make_pair(false, std::make_unique<AnimatedStatContainer>(game_constants::ARMOR_CONTAINER_BOT_POSITION, METALLIC_HEALTH_CRYSTAL_TEXTURE_FILE_NAME, ARMOR_CONTAINER_BOT_SCENE_OBJECT_NAME_PREFIX, &mBoardState->GetPlayerStates()[1].mPlayerCurrentArmor, true, *scene)));
    mAnimatedStatContainers.back().second->GetSceneObjects().front()->mShaderBoolUniformValues[game_constants::METALLIC_STAT_CONTAINER_UNIFORM_NAME] = true;
    mAnimatedStatContainers.back().second->GetSceneObjects().front()->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + METALLIC_TEXTURE_FILE_NAME);
    
    // Board Effect Animation Factory lambda
    auto cardBoardEffectAnimation = [=]
    (
        const strutils::StringId& topSceneObjectName,
        const strutils::StringId& botSceneObjectName
    )
    {
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(scene->FindSceneObject(topSceneObjectName), game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_SCALE_UP_FACTOR, game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_PULSE_ANIMATION_PULSE_DURATION_SECS, animation_flags::ANIMATE_CONTINUOUSLY), [](){});
   
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::PulseAnimation>(scene->FindSceneObject(botSceneObjectName), game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_SCALE_UP_FACTOR, game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_PULSE_ANIMATION_PULSE_DURATION_SECS, animation_flags::ANIMATE_CONTINUOUSLY), [](){});
    };
    
    // Kill Side Effects
    cardBoardEffectAnimation(game_constants::KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Demon Kill Side Effects
    cardBoardEffectAnimation(game_constants::DEMON_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::DEMON_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Insect Duplication Effects
    cardBoardEffectAnimation(game_constants::INSECT_DUPLICATION_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::INSECT_DUPLICATION_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Double Dino Damage Effects
    cardBoardEffectAnimation(game_constants::NEXT_DINO_DAMAGE_DOUBLING_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::NEXT_DINO_DAMAGE_DOUBLING_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Heal next Dino Damage Effects
    cardBoardEffectAnimation(game_constants::NEXT_DINO_HEAL_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::NEXT_DINO_HEAL_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Double Poison Attacks Effects
    cardBoardEffectAnimation(game_constants::DOUBLE_POISON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::DOUBLE_POISON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Insect Virus Effects
    cardBoardEffectAnimation(game_constants::INSECT_VIRUS_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::INSECT_VIRUS_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Rodent Lifesteal on Attack
    cardBoardEffectAnimation(game_constants::RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Dig no Fail Effects
    cardBoardEffectAnimation(game_constants::DIG_NO_FAIL_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::DIG_NO_FAIL_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Permanent Continual Weight Reduction Effects
    cardBoardEffectAnimation(game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    // Permanent Continual Weight Reduction Effects
    cardBoardEffectAnimation(game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_TOP_SCENE_OBJECT_NAME, game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_BOT_SCENE_OBJECT_NAME);
    
    auto historyButtonSnapToEdgeFactor = HISTORY_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR;
#if defined(MOBILE_FLOW)
    if (ios_utils::IsIPad())
    {
        historyButtonSnapToEdgeFactor = IPAD_HISTORY_BUTTON_SNAP_TO_EDGE_OFFSET_SCALE_FACTOR;
    }
#endif
    
    mBattleSceneAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
    (
        HISTORY_BUTTON_POSITION,
        HISTORY_BUTTON_SCALE,
        HISTORY_ICON_TEXTURE_FILE_NAME,
        HISTORY_BUTTON_SCENE_OBJECT_NAME,
        [=](){ OnHistoryButtonPressed(); },
        *scene,
        scene::SnapToEdgeBehavior::SNAP_TO_RIGHT_EDGE,
        historyButtonSnapToEdgeFactor
    ));
    
    auto historyScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(HISTORY_SCENE);

    mCardHistoryContainer = std::make_unique<SwipeableContainer<CardHistoryEntry>>
    (
        ContainerType::HORIZONTAL_LINE,
        CARD_HISTORY_ENTRY_SCALE,
        CARD_HISTORY_CONTAINER_BOUNDS,
        CARD_HISTORY_CONTAINER_CUTOFF_VALUES,
        CARD_HISTORY_CONTAINER_NAME,
        CARD_HISTORY_CONTAINER_Z,
        *historyScene
    );
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::InitHistoryScene(std::shared_ptr<scene::Scene> scene)
{
    mCardHistoryContainer->ResetItemPositions();
    mCardHistoryContainer->SetBlockedUpdate(true);
    
    auto capsuleSceneObject = scene->FindSceneObject(CARD_HISTORY_CAPSULE_SCENE_OBJECT_NAME);
    capsuleSceneObject->mPosition.y = HISTORY_SCENE_FADE_IN_OUT_ITEM_OFFSETS;
    capsuleSceneObject->mInvisible = false;
    capsuleSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(capsuleSceneObject, CARD_HISTORY_CAPSULE_POSITION, capsuleSceneObject->mScale, 1.0f, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=]()
    {
        if (mActiveScene == CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(HISTORY_SCENE))
        {
            for (auto& containerItem: mCardHistoryContainer->GetItems())
            {
                for (auto& sceneObject: containerItem.mSceneObjects)
                {
                    auto targetPosition = sceneObject->mPosition;
                    sceneObject->mInvisible = false;
                    sceneObject->mPosition.x += HISTORY_SCENE_FADE_IN_OUT_ITEM_OFFSETS;
                    sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(sceneObject, targetPosition, sceneObject->mScale, 1.0f, animation_flags::NONE, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=]()
                    {
                        mCardHistoryContainer->SetBlockedUpdate(false);
                    });
                }
            }
        }
    });
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> activeScene)
{
    static float time;
    time += dtMillis * 0.001f;
    mActiveScene = activeScene;
    
    if (activeScene->GetName() == game_constants::BATTLE_SCENE)
    {
        if (mActionEngine->GetActiveGameActionName() == IDLE_GAME_ACTION_NAME)
        {
            if (mCurrentBattleControlType == BattleControlType::AI_TOP_BOT || (mCurrentBattleControlType == BattleControlType::AI_TOP_ONLY && mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX))
            {
                mPlayerActionGenerationEngine->DecideAndPushNextActions(mBoardState.get());
            }
            else if (mCurrentBattleControlType == BattleControlType::REPLAY)
            {
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(activeScene->FindSceneObject(REPLAY_TEXT_SCENE_OBJECT_NAME), 0.0f, REPLAY_TEXT_FADE_IN_OUT_DURATION_SECS), [=]()
                {
                    activeScene->FindSceneObject(REPLAY_TEXT_SCENE_OBJECT_NAME)->mInvisible = true;
                });
                
                mCurrentBattleControlType = BattleControlType::AI_TOP_ONLY;
                DataRepository::GetInstance().SetNextBattleControlType(mCurrentBattleControlType);
            }
        }
        
        if (mCurrentBattleControlType == BattleControlType::AI_TOP_ONLY && mBoardState->GetActivePlayerIndex() == game_constants::LOCAL_PLAYER_INDEX)
        {
            HandleTouchInput(dtMillis);
        }
        
        if (mGuiManager && mActionEngine->GetActiveGameActionName() != BATTLE_INITIAL_SETUP_AND_ANIMATION_GAME_ACTION_NAME)
        {
            mGuiManager->Update(dtMillis);
        }
        
        UpdateMiscSceneObjects(dtMillis);
        
        bool foundActiveStatContainer = false;
        for (auto& animatedStatContainer: mAnimatedStatContainers)
        {
            if (animatedStatContainer.first)
            {
                foundActiveStatContainer = true;
            }
            else if (mActionEngine->GetActiveGameActionName() != HERO_CARD_ENTRY_GAME_ACTION_NAME)
            {
                animatedStatContainer.second->RealignBaseAndValueSceneObjects();
            }
        }
        
        if (!foundActiveStatContainer || mActionEngine->GetActiveGameActionName() == GAME_OVER_GAME_ACTION_NAME)
        {
            mActionEngine->Update(dtMillis);
        }
        
        if (mActionEngine->GetActiveGameActionName() == IDLE_GAME_ACTION_NAME)
        {
            mCanInteractWithAnyHeldCard = true;
            mCanPlayNextCard = true;
            mPendingCardReleasedThisFrame = nullptr;
            
            if (!mPendingCardsToBePlayed.empty())
            {
                mPendingCardReleasedThisFrame = mPendingCardsToBePlayed.front();
                mPendingCardsToBePlayed.erase(mPendingCardsToBePlayed.begin());
            }
        }
    }
    else if (activeScene->GetName() == HISTORY_SCENE)
    {
        static int sToolTipIndex = -1;
        static float sToolTipPointeePosX = 0.0f;
        
        const auto& cardHistoryContainerUpdateResult = mCardHistoryContainer->Update(dtMillis);
        if (cardHistoryContainerUpdateResult.mInteractionType == InteractionType::NONE)
        {
            if (CoreSystemsEngine::GetInstance().GetInputStateManager().VButtonTapped(input::Button::MAIN_BUTTON))
            {
                events::EventSystem::GetInstance().DispatchEvent<events::PopSceneModalEvent>();
            }
        }
        else if (cardHistoryContainerUpdateResult.mInteractionType == InteractionType::INTERACTED_WITH_ELEMENTS)
        {
            if (sToolTipIndex != cardHistoryContainerUpdateResult.mInteractedElementIndex)
            {
                sToolTipIndex = cardHistoryContainerUpdateResult.mInteractedElementIndex;
                auto interactedElementEntry = mCardHistoryContainer->GetItems()[cardHistoryContainerUpdateResult.mInteractedElementIndex];
                if (!interactedElementEntry.mIsTurnCounter)
                {
                    auto cardData = CardDataRepository::GetInstance().GetCardData(interactedElementEntry.mCardId, interactedElementEntry.mForOpponent ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX);
                    
                    DestroyCardTooltip(activeScene);
                    
                    if (cardData.IsSpell())
                    {
                        sToolTipPointeePosX = interactedElementEntry.mSceneObjects.front()->mPosition.x;
                        
                        CreateCardTooltip(interactedElementEntry.mSceneObjects.front()->mPosition, cardData.mCardEffectTooltip, interactedElementEntry.mSceneObjects.front()->mPosition.x < 0.0f ? 0 : 10, activeScene);
                    }
                }
            }
        }
        
        // Card tooltip
        if (sToolTipIndex != -1)
        {
            auto interactedElementEntry = mCardHistoryContainer->GetItems()[sToolTipIndex];
            if (math::Abs(interactedElementEntry.mSceneObjects.front()->mPosition.x - sToolTipPointeePosX) > 0.01f)
            {
                sToolTipIndex = -1;
                DestroyCardTooltip(activeScene);
            }
        }
        auto cardTooltipSceneObject = activeScene->FindSceneObject(CARD_TOOLTIP_SCENE_OBJECT_NAME);
        
        cardTooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] += dtMillis * CARD_TOOLTIP_REVEAL_SPEED;
        if (cardTooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] >= CARD_TOOLTIP_MAX_REVEAL_THRESHOLD)
        {
            cardTooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] = CARD_TOOLTIP_MAX_REVEAL_THRESHOLD;
            
            for (auto i = 0; i < game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT; ++i)
            {
                auto tooltipTextSceneObject = activeScene->FindSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[i]);
                tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = math::Min(1.0f, tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] +  dtMillis * CARD_TOOLTIP_TEXT_REVEAL_SPEED);
            }
        }
        
        for (auto& entry: mCardHistoryContainer->GetItems())
        {
            for (auto& sceneObject: entry.mSceneObjects)
            {
                sceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
            }
        }
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene> scene)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    if (scene->GetName() == HISTORY_SCENE)
    {
        mCardHistoryContainer->SetBlockedUpdate(false);
        
        for (const auto& cardHistoryEntry: mCardHistoryContainer->GetItems())
        {
            for (auto sceneObject: cardHistoryEntry.mSceneObjects)
            {
                animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, HISTORY_SCENE_FADE_IN_OUT_DURATION_SECS), [=]()
                {
                    sceneObject->mInvisible = true;
                });
            }
        }
        
        animationManager.StopAnimation(game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
        animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE)->GetUpdateTimeSpeedFactor(), 1.0f, OVERLAY_SCENE_SPEED_ANIMATION_TARGET_DURATION), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(CARD_HISTORY_CAPSULE_SCENE_OBJECT_NAME), 0.0f, HISTORY_SCENE_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            scene->FindSceneObject(CARD_HISTORY_CAPSULE_SCENE_OBJECT_NAME)->mInvisible = true;
        });
        DestroyCardTooltip(scene);
    }
    else if (scene->GetName() == game_constants::BATTLE_SCENE)
    {
        // Serialize story battle
        if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !DataRepository::GetInstance().GetQuickPlayData())
        {
            DataRepository::GetInstance().SetNextBattleControlType(BattleControlType::REPLAY);
            mBattleSerializer->FlushStateToFile();
        }
        
        mGuiManager = nullptr;
        CoreSystemsEngine::GetInstance().GetSceneManager().RemoveScene(HISTORY_SCENE);
        events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
    }
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> BattleSceneLogicManager::VGetGuiObjectManager()
{
    return mGuiManager;
}

///------------------------------------------------------------------------------------------------

const BoardState& BattleSceneLogicManager::GetBoardState() const
{
    return *mBoardState;
}

///------------------------------------------------------------------------------------------------

GameActionEngine& BattleSceneLogicManager::GetActionEngine()
{
    return *mActionEngine;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::vector<std::shared_ptr<CardSoWrapper>>>& BattleSceneLogicManager::GetHeldCardSoWrappers() const
{
    return mPlayerHeldCardSceneObjectWrappers;
}

///------------------------------------------------------------------------------------------------

const std::vector<std::vector<std::shared_ptr<CardSoWrapper>>>& BattleSceneLogicManager::GetBoardCardSoWrappers() const
{
    return mPlayerBoardCardSceneObjectWrappers;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::HandleTouchInput(const float dtMillis)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(battleScene->GetCamera().GetViewMatrix(), battleScene->GetCamera().GetProjMatrix());
    
    auto& localPlayerCards = mPlayerHeldCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
    const auto localPlayerCardCount = static_cast<int>(localPlayerCards.size());
    
    std::vector<int> candidateHighlightIndices;
    mShouldShowCardLocationIndicator = false;
    bool freeMovingCardThisFrame = false;
    for (int i = 0; i < localPlayerCardCount; ++i)
    {
        auto& currentCardSoWrapper = localPlayerCards.at(i);
        
        if (currentCardSoWrapper->mState == CardSoState::FREE_MOVING)
        {
            DestroyCardTooltip(battleScene);
        }
        
        bool otherHighlightedCardExists = std::find_if(localPlayerCards.begin(), localPlayerCards.end(), [&](const std::shared_ptr<CardSoWrapper>& cardSoWrapper){ return cardSoWrapper.get() != currentCardSoWrapper.get() && cardSoWrapper->mState == CardSoState::HIGHLIGHTED; }) != localPlayerCards.cend();
        
        auto cardBaseSceneObject = currentCardSoWrapper->mSceneObject;
        auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*cardBaseSceneObject);
        
        bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
        
        // Check for card tooltip creation
        if (cursorInSceneObject && currentCardSoWrapper->mState == CardSoState::HIGHLIGHTED)
        {
            mSecsCardHighlighted += dtMillis/1000.0f;
            if (mSecsCardHighlighted > CARD_TOOLTIP_CREATION_DELAY_SECS && battleScene->FindSceneObject(CARD_TOOLTIP_SCENE_OBJECT_NAME)->mInvisible)
            {
                if (currentCardSoWrapper->mCardData.IsSpell())
                {
                    CreateCardTooltip(currentCardSoWrapper->mSceneObject->mPosition, currentCardSoWrapper->mCardData.mCardEffectTooltip, localPlayerCardCount - i - 1, battleScene);
                }
            }
        }
        
#if defined(MOBILE_FLOW)
        static std::unique_ptr<glm::vec2> selectedCardInitialTouchPosition = nullptr;
        if (inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON) &&
            mRuleEngine->CanCardBePlayed(&currentCardSoWrapper->mCardData, i, game_constants::LOCAL_PLAYER_INDEX) &&
            ((currentCardSoWrapper->mState == CardSoState::HIGHLIGHTED && glm::distance(worldTouchPos, *selectedCardInitialTouchPosition) > 0.005f) || currentCardSoWrapper->mState == CardSoState::FREE_MOVING) &&
            !freeMovingCardThisFrame)
        {
            currentCardSoWrapper->mState = CardSoState::FREE_MOVING;
            
            if (std::find(mPendingCardsToBePlayed.begin(), mPendingCardsToBePlayed.end(), currentCardSoWrapper) == mPendingCardsToBePlayed.end())
            {
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, glm::vec3(worldTouchPos.x, worldTouchPos.y + game_constants::IN_GAME_MOBILE_ONLY_FREE_MOVING_CARD_Y_OFFSET, game_constants::IN_GAME_HIGHLIGHTED_CARD_Z), currentCardSoWrapper->mSceneObject->mScale, game_constants::IN_GAME_CARD_FREE_MOVEMENT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [](){});
                
                const auto& currentLocalPlayerBoardCards = mBoardState->GetPlayerStates().at(game_constants::LOCAL_PLAYER_INDEX).mPlayerBoardCards;
                const auto& currentLocalPlayerDeadBoardCardIndices = mBoardState->GetPlayerStates().at(game_constants::LOCAL_PLAYER_INDEX).mBoardCardIndicesToDestroy;
                const auto& currentLocalPlayerBoardCardCount = card_utils::CalculateNonDeadCardsCount(currentLocalPlayerBoardCards, currentLocalPlayerDeadBoardCardIndices);
                
                auto cardLocationIndicatorSo = battleScene->FindSceneObject(CARD_LOCATION_INDICATOR_SCENE_OBJECT_NAME);
                cardLocationIndicatorSo->mPosition = card_utils::CalculateBoardCardPosition(currentLocalPlayerBoardCardCount, currentLocalPlayerBoardCardCount + 1, false);
                cardLocationIndicatorSo->mPosition.z = game_constants::CARD_LOCATION_EFFECT_Z;
                mShouldShowCardLocationIndicator = true;
                freeMovingCardThisFrame = true;
            }
        }
        else if (inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) && cursorInSceneObject && !otherHighlightedCardExists && mCanInteractWithAnyHeldCard && currentCardSoWrapper->mState != CardSoState::MOVING_TO_SET_POSITION)
        {
            auto originalCardPosition = card_utils::CalculateHeldCardPosition(i, localPlayerCardCount, false, battleScene->GetCamera());
            if (currentCardSoWrapper->mSceneObject->mPosition.y <= originalCardPosition.y)
            {
                selectedCardInitialTouchPosition = std::make_unique<glm::vec2>(worldTouchPos);
                candidateHighlightIndices.push_back(i);
            }
        }
        else if (!inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON))
        {
            switch (currentCardSoWrapper->mState)
            {
                case CardSoState::FREE_MOVING:
                {
                    OnFreeMovingCardRelease(currentCardSoWrapper);
                } break;
                    
                    
                case CardSoState::HIGHLIGHTED:
                {
                    auto originalCardPosition = card_utils::CalculateHeldCardPosition(i, localPlayerCardCount, false, battleScene->GetCamera());
                    animationManager.StopAllAnimationsPlayingForSceneObject(currentCardSoWrapper->mSceneObject->mName);
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::IGNORE_X_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){currentCardSoWrapper->mState = CardSoState::IDLE; });
                    currentCardSoWrapper->mState = CardSoState::MOVING_TO_SET_POSITION;
                    DestroyCardHighlighterAtIndex(i);
                    mSecsCardHighlighted = 0.0f;
                } break;
                    
                default: break;
            }
        }
        
#else
        if (inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON) &&
            currentCardSoWrapper->mState == CardSoState::FREE_MOVING &&
            !freeMovingCardThisFrame)
        {
            if (std::find(mPendingCardsToBePlayed.begin(), mPendingCardsToBePlayed.end(), currentCardSoWrapper) == mPendingCardsToBePlayed.end())
            {
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, glm::vec3(worldTouchPos.x, worldTouchPos.y, game_constants::IN_GAME_HIGHLIGHTED_CARD_Z), currentCardSoWrapper->mSceneObject->mScale, game_constants::IN_GAME_CARD_FREE_MOVEMENT_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [](){});
                
                const auto& currentLocalPlayerBoardCards = mBoardState->GetPlayerStates().at(game_constants::LOCAL_PLAYER_INDEX).mPlayerBoardCards;
                const auto& currentLocalPlayerDeadBoardCardIndices = mBoardState->GetPlayerStates().at(game_constants::LOCAL_PLAYER_INDEX).mBoardCardIndicesToDestroy;
                const auto& currentLocalPlayerBoardCardCount = card_utils::CalculateNonDeadCardsCount(currentLocalPlayerBoardCards, currentLocalPlayerDeadBoardCardIndices);
                
                auto cardLocationIndicatorSo = battleScene->FindSceneObject(CARD_LOCATION_INDICATOR_SCENE_OBJECT_NAME);
                cardLocationIndicatorSo->mPosition = card_utils::CalculateBoardCardPosition(currentLocalPlayerBoardCardCount, currentLocalPlayerBoardCardCount + 1, false);
                cardLocationIndicatorSo->mPosition.z = game_constants::CARD_LOCATION_EFFECT_Z;
                mShouldShowCardLocationIndicator = true;
                freeMovingCardThisFrame = true;
            }
        }
        else if (inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) &&
                 cursorInSceneObject &&
                 !otherHighlightedCardExists &&
                 currentCardSoWrapper->mState == CardSoState::HIGHLIGHTED &&
                 mRuleEngine->CanCardBePlayed(&currentCardSoWrapper->mCardData, i, game_constants::LOCAL_PLAYER_INDEX) &&
                 battleScene->FindSceneObject(strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(i))) != nullptr)
        {
            currentCardSoWrapper->mState = CardSoState::FREE_MOVING;
        }
        else if (!inputStateManager.VButtonPressed(input::Button::MAIN_BUTTON))
        {
            switch (currentCardSoWrapper->mState)
            {
                case CardSoState::FREE_MOVING:
                {
                    OnFreeMovingCardRelease(currentCardSoWrapper);
                } break;
                
                case CardSoState::IDLE:
                {
                    if (cursorInSceneObject && !otherHighlightedCardExists && mCanInteractWithAnyHeldCard)
                    {
                        candidateHighlightIndices.push_back(i);
                    }
                } break;
                    
                case CardSoState::HIGHLIGHTED:
                {
                    if (!cursorInSceneObject)
                    {
                        auto originalCardPosition = card_utils::CalculateHeldCardPosition(i, localPlayerCardCount, false, battleScene->GetCamera());
                        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::IGNORE_X_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){currentCardSoWrapper->mState = CardSoState::IDLE; });
                        currentCardSoWrapper->mState = CardSoState::MOVING_TO_SET_POSITION;
                        DestroyCardHighlighterAtIndex(i);
                        mSecsCardHighlighted = 0.0f;
                    }
                } break;
                    
                default: break;
            }
        }
#endif
    }
    
    // Select based candidate card to highlight based on distance from cursor
    std::sort(candidateHighlightIndices.begin(), candidateHighlightIndices.end(), [&](const int& lhs, const int& rhs)
    {
        return math::Abs(localPlayerCards[lhs]->mSceneObject->mPosition.x - worldTouchPos.x) <
               math::Abs(localPlayerCards[rhs]->mSceneObject->mPosition.x - worldTouchPos.x);
    });
    
    if (!candidateHighlightIndices.empty() && localPlayerCards.size() == mBoardState->GetPlayerStates()[1].mPlayerHeldCards.size())
    {
        auto currentCardSoWrapper = localPlayerCards[candidateHighlightIndices.front()];
        
        auto originalCardPosition = card_utils::CalculateHeldCardPosition(candidateHighlightIndices.front(), localPlayerCardCount, false, battleScene->GetCamera());
        originalCardPosition.y += game_constants::IN_GAME_BOT_PLAYER_SELECTED_CARD_Y_OFFSET;
        originalCardPosition.z = game_constants::IN_GAME_HIGHLIGHTED_CARD_Z;
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_HIGHLIGHT_ANIMATION_DURATION, animation_flags::IGNORE_X_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
        {
            CreateCardHighlighter();
        });
        
        currentCardSoWrapper->mState = CardSoState::HIGHLIGHTED;
        
    }
    
    // Check for turn pointer interaction
    bool freeMovingCardExists = std::find_if(localPlayerCards.begin(), localPlayerCards.end(), [&](const std::shared_ptr<CardSoWrapper>& cardSoWrapper){ return cardSoWrapper->mState == CardSoState::FREE_MOVING; }) != localPlayerCards.cend();
    if (!freeMovingCardExists && mBoardState->GetActivePlayerIndex() == 1)
    {
        auto turnPointerSo = battleScene->FindSceneObject(game_constants::TURN_POINTER_SCENE_OBJECT_NAME);
        auto turnPointerHighlighterSo = battleScene->FindSceneObject(game_constants::TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME);
        
        auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*turnPointerSo);
        bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
        
        if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON) && mCanIssueNextTurnInteraction)
        {
            animationManager.StartAnimation(std::make_unique<rendering::PulseAnimation>(turnPointerSo, TURN_POINTER_INTERACTOR_SCALE_FACTOR, TURN_POINTER_INTERACTION_PULSE_DURATION), [](){});
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(turnPointerHighlighterSo, 0.0f, game_constants::TURN_POINTER_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [](){});
            mActionEngine->AddGameAction(NEXT_PLAYER_ACTION_NAME);
            mCanIssueNextTurnInteraction = false;
        }
    }
    
    // Make sure that later pending cards appear in front of earlier ones
    if (mPendingCardsToBePlayed.size() > 1U)
    {
        for (auto i = 1U; i < mPendingCardsToBePlayed.size(); ++i)
        {
            mPendingCardsToBePlayed[i]->mSceneObject->mPosition.z = mPendingCardsToBePlayed.front()->mSceneObject->mPosition.z + i * 0.1f;
        }
    }
    
    // Additional constraints on showing the card location indicator
    mShouldShowCardLocationIndicator &= mActionEngine->GetActiveGameActionName() == IDLE_GAME_ACTION_NAME;
    mShouldShowCardLocationIndicator &= mBoardState->GetActivePlayerIndex() == 1;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::UpdateMiscSceneObjects(const float dtMillis)
{
    static float time = 0.0f;
    time += dtMillis * 0.001f;
    
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    const auto& inputStateManager = CoreSystemsEngine::GetInstance().GetInputStateManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    // Card Interactive Elements
    auto& localPlayerHeldCards = mPlayerHeldCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
    auto& localPlayerBoardCards = mPlayerBoardCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
    auto& remotePlayerBoardCards = mPlayerBoardCardSceneObjectWrappers[game_constants::REMOTE_PLAYER_INDEX];
    
    for (size_t i = 0; i < localPlayerHeldCards.size(); ++i)
    {
        auto& cardSoWrapper = localPlayerHeldCards[i];
        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
        
        if (mActionEngine->GetActiveGameActionName() != CARD_BUFFED_DEBUFFED_ANIMATION_GAME_ACTION_NAME &&
            mActionEngine->GetActiveGameActionName() != CARD_EFFECT_GAME_ACTION_NAME)
        {
            auto canCardBePlayed = mRuleEngine->CanCardBePlayed(&cardSoWrapper->mCardData, i, game_constants::LOCAL_PLAYER_INDEX);
            cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] = canCardBePlayed ? game_constants::CARD_INTERACTIVE_MODE_DEFAULT : game_constants::CARD_INTERACTIVE_MODE_NONINTERACTIVE;
            
            const auto& heldCardStatOverrides = mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHeldCardStatOverrides;
            int overriddenWeight = cardSoWrapper->mCardData.mCardWeight;
            if (heldCardStatOverrides.size() > i)
            {
                overriddenWeight = math::Max(0, heldCardStatOverrides[i].count(CardStatType::WEIGHT) ? heldCardStatOverrides[i].at(CardStatType::WEIGHT) : cardSoWrapper->mCardData.mCardWeight);
            }
            if (!cardSoWrapper->mCardData.IsSpell() && mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mBoardModifiers.mGlobalCardStatModifiers.count(CardStatType::WEIGHT))
            {
                overriddenWeight = math::Max(0, overriddenWeight + mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::WEIGHT));
            }
            
            if (canCardBePlayed && overriddenWeight < cardSoWrapper->mCardData.mCardWeight)
            {
                cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] = game_constants::CARD_INTERACTIVE_MODE_INTERACTIVE;
            }
        }
    }
    for (auto& cardSoWrapper: localPlayerBoardCards)
    {
        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
        if (cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] != game_constants::CARD_INTERACTIVE_MODE_INTERACTIVE)
        {
            cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] = game_constants::CARD_INTERACTIVE_MODE_DEFAULT;
        }
    }
    for (auto& cardSoWrapper: remotePlayerBoardCards)
    {
        cardSoWrapper->mSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
        if (cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] != game_constants::CARD_INTERACTIVE_MODE_INTERACTIVE)
        {
            cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] = game_constants::CARD_INTERACTIVE_MODE_DEFAULT;
        }
    }
    
    // Action Highlighters
    for (size_t i = 0U; i < localPlayerHeldCards.size(); ++i)
    {
        auto cardHighlighterObject = battleScene->FindSceneObject(strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
        if (cardHighlighterObject)
        {
            cardHighlighterObject->mInvisible = false;
            cardHighlighterObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
            cardHighlighterObject->mPosition = localPlayerHeldCards[i]->mSceneObject->mPosition;
            cardHighlighterObject->mPosition.z += game_constants::ACTION_HIGLIGHTER_Z_OFFSET;
            
            localPlayerHeldCards[i]->mSceneObject->mPosition.z -= game_constants::ACTION_HIGLIGHTER_Z_OFFSET;
        }
    }
    
    // Turn pointer highlighter
    auto turnPointerSo = battleScene->FindSceneObject(game_constants::TURN_POINTER_SCENE_OBJECT_NAME);
    auto turnPointerHighlighterSo = battleScene->FindSceneObject(game_constants::TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME);
    turnPointerHighlighterSo->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    turnPointerHighlighterSo->mShaderBoolUniformValues[game_constants::CARD_HIGHLIGHTER_INVALID_ACTION_UNIFORM_NAME] = false;
    turnPointerHighlighterSo->mPosition = turnPointerSo->mPosition;
    turnPointerHighlighterSo->mPosition.z += game_constants::ACTION_HIGLIGHTER_Z_OFFSET;
    
    // Lambda to make space/revert to original position board cards
    auto prospectiveMakeSpaceRevertToPositionLambda = [&](int prospectiveCardCount)
    {
        auto& boardCardSoWrappers = mPlayerBoardCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        
        const auto currentBoardCardCount = static_cast<int>(boardCardSoWrappers.size());
        
        for (int i = 0; i < currentBoardCardCount; ++i)
        {
            auto animationName = strutils::StringId(MAKE_SPACE_REVERT_TO_POSITION_ANIMATION_NAME_PREFIX + std::to_string(i));
            auto& currentCardSoWrapper = boardCardSoWrappers.at(i);
            auto originalCardPosition = card_utils::CalculateBoardCardPosition(i, prospectiveCardCount, false);
            animationManager.StopAnimation(animationName);
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){}, animationName);
        }
    };
    
    // Card Location
    auto cardLocationIndicatorSo = battleScene->FindSceneObject(CARD_LOCATION_INDICATOR_SCENE_OBJECT_NAME);
    auto currentSoWrapperIter = std::find_if(localPlayerHeldCards.begin(), localPlayerHeldCards.end(), [&](const std::shared_ptr<CardSoWrapper>& cardSoWrapper){ return cardSoWrapper->mState == CardSoState::FREE_MOVING; });
    
    if (mShouldShowCardLocationIndicator && currentSoWrapperIter != localPlayerHeldCards.end())
    {
        cardLocationIndicatorSo->mInvisible = false;
        cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
        
        auto distanceFromCardLocationSo = math::Distance2IgnoreZ((*currentSoWrapperIter)->mSceneObject->mPosition, cardLocationIndicatorSo->mPosition);
#if defined(MOBILE_FLOW)
        bool inBoardDropThreshold = distanceFromCardLocationSo <= MOBILE_DISTANCE_FROM_CARD_LOCATION_INDICATOR;
#else
        bool inBoardDropThreshold = distanceFromCardLocationSo <= DESKTOP_DISTANCE_FROM_CARD_LOCATION_INDICATOR;
#endif
        
        const auto& currentLocalPlayerBoardCards = mBoardState->GetPlayerStates().at(game_constants::LOCAL_PLAYER_INDEX).mPlayerBoardCards;
        const auto& currentLocalPlayerDeadBoardCardIndices = mBoardState->GetPlayerStates().at(game_constants::LOCAL_PLAYER_INDEX).mBoardCardIndicesToDestroy;
        const auto& currentLocalPlayerBoardCardCount = card_utils::CalculateNonDeadCardsCount(currentLocalPlayerBoardCards, currentLocalPlayerDeadBoardCardIndices);
        
        // If in drop threshold we lerp to max target location alpha
        if (inBoardDropThreshold)
        {
            cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] += dtMillis * CARD_LOCATION_EFFECT_ALPHA_SPEED;
            if (cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] >= CARD_LOCATION_EFFECT_MAX_TARGET_ALPHA)
            {
                cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = CARD_LOCATION_EFFECT_MAX_TARGET_ALPHA;
            }
            
            if (mPreviousProspectiveBoardCardsPushState == ProspectiveBoardCardsPushState::MAKE_SPACE_FOR_NEW_CARD)
            {
                prospectiveMakeSpaceRevertToPositionLambda(currentLocalPlayerBoardCardCount + 1);
            }
            mPreviousProspectiveBoardCardsPushState = ProspectiveBoardCardsPushState::MAKE_SPACE_FOR_NEW_CARD;
        }
        else
        {
            // Else if not, we constrain the alpha to the min target
            if (math::Abs(cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] - CARD_LOCATION_EFFECT_MIN_TARGET_ALPHA) > dtMillis * CARD_LOCATION_EFFECT_ALPHA_SPEED)
            {
                if (cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] > CARD_LOCATION_EFFECT_MIN_TARGET_ALPHA)
                {
                    cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] -= dtMillis * CARD_LOCATION_EFFECT_ALPHA_SPEED;
                }
                else
                {
                    cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] += dtMillis * CARD_LOCATION_EFFECT_ALPHA_SPEED;
                }
            }
            
            if (mPreviousProspectiveBoardCardsPushState != ProspectiveBoardCardsPushState::REVERT_TO_ORIGINAL_POSITION)
            {
                prospectiveMakeSpaceRevertToPositionLambda(currentLocalPlayerBoardCardCount);
            }
            mPreviousProspectiveBoardCardsPushState = ProspectiveBoardCardsPushState::REVERT_TO_ORIGINAL_POSITION;
        }
    }
    else
    {
        cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] -= dtMillis * CARD_LOCATION_EFFECT_ALPHA_SPEED;
        if (cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] <= 0.0f)
        {
            cardLocationIndicatorSo->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            cardLocationIndicatorSo->mInvisible = true;
        }
        
        mPreviousProspectiveBoardCardsPushState = ProspectiveBoardCardsPushState::NONE;
    }
    
    // Stat Crystal Values
    for (auto& statContainerEntry: mAnimatedStatContainers)
    {
        if (statContainerEntry.first)
        {
            statContainerEntry.first = statContainerEntry.second->Update(dtMillis) == AnimatedStatContainerUpdateResult::ONGOING;
        }
        
        statContainerEntry.second->GetSceneObjects().front()->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    }
    
    // Board side effects
    auto boardSideEffectTopSceneObject = battleScene->FindSceneObject(game_constants::BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME);
    boardSideEffectTopSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = fmod(time/10, 1.0f);
    
    auto boardSideEffectBotSceneObject = battleScene->FindSceneObject(game_constants::BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
    boardSideEffectBotSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = fmod(time/10, 1.0f);
    
    for (const auto& activePlayerIndividualCardEffectSceneObjects: mActiveIndividualCardBoardEffectSceneObjects)
    {
        for (auto& effectSceneObject: activePlayerIndividualCardEffectSceneObjects)
        {
            effectSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = 0.0f;
        }
    }
    
    // Card tooltip
    auto cardTooltipSceneObject = battleScene->FindSceneObject(CARD_TOOLTIP_SCENE_OBJECT_NAME);
    
    cardTooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] += dtMillis * CARD_TOOLTIP_REVEAL_SPEED;
    if (cardTooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] >= CARD_TOOLTIP_MAX_REVEAL_THRESHOLD)
    {
        cardTooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] = CARD_TOOLTIP_MAX_REVEAL_THRESHOLD;
        
        for (auto i = 0; i < game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT; ++i)
        {
            auto tooltipTextSceneObject = battleScene->FindSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[i]);
            tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = math::Min(1.0f, tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] +  dtMillis * CARD_TOOLTIP_TEXT_REVEAL_SPEED);
        }
    }
    
    // Check for opponent card inspection
    auto& localPlayerCards = mPlayerHeldCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
    bool freeMovingCardExists = std::find_if(localPlayerCards.begin(), localPlayerCards.end(), [&](const std::shared_ptr<CardSoWrapper>& cardSoWrapper){ return cardSoWrapper->mState == CardSoState::FREE_MOVING; }) != localPlayerCards.cend();
    if (!freeMovingCardExists)
    {
        auto worldTouchPos = inputStateManager.VGetPointingPosInWorldSpace(battleScene->GetCamera().GetViewMatrix(), battleScene->GetCamera().GetProjMatrix());
        for (auto& cardSoWrapper: mPlayerBoardCardSceneObjectWrappers[game_constants::REMOTE_PLAYER_INDEX])
        {
            auto cardBaseSceneObject = cardSoWrapper->mSceneObject;
            auto sceneObjectRect = scene_object_utils::GetSceneObjectBoundingRect(*cardBaseSceneObject);
            
            bool cursorInSceneObject = math::IsPointInsideRectangle(sceneObjectRect.bottomLeft, sceneObjectRect.topRight, worldTouchPos);
            
            if (cursorInSceneObject && inputStateManager.VButtonTapped(input::Button::MAIN_BUTTON))
            {
                DataRepository::GetInstance().SetNextInspectedCardId(cardSoWrapper->mCardData.mCardId);
                CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(battleScene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
                events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(CARD_INSPECTION_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                break;
            }
        }
    }
    
    // Animated buttons
    for (auto& animatedButton: mBattleSceneAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
    
    auto flawlessVictoryIndicator = mActiveScene->FindSceneObject(FLAWLESS_VICTORY_INDICATOR_SCENE_OBJECT_NAME);
    if (flawlessVictoryIndicator)
    {
        flawlessVictoryIndicator->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = -time;
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnFreeMovingCardRelease(std::shared_ptr<CardSoWrapper> cardSoWrapper)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto& localPlayerCards = mPlayerHeldCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
    auto cardIndex = std::find_if(localPlayerCards.begin(), localPlayerCards.end(), [=](const std::shared_ptr<CardSoWrapper>& otherCard)
    {
        return otherCard.get() == cardSoWrapper.get();
    }) - localPlayerCards.begin();
    
    DestroyCardHighlighterAtIndex(static_cast<int>(cardIndex));
    
    auto cardLocationIndicatorSo = battleScene->FindSceneObject(CARD_LOCATION_INDICATOR_SCENE_OBJECT_NAME);
    auto distanceFromCardLocationSo = math::Distance2IgnoreZ(cardSoWrapper->mSceneObject->mPosition, cardLocationIndicatorSo->mPosition);
    
#if defined(MOBILE_FLOW)
    bool inBoardDropThreshold = distanceFromCardLocationSo <= MOBILE_DISTANCE_FROM_CARD_LOCATION_INDICATOR;
#else
    bool inBoardDropThreshold = distanceFromCardLocationSo <= DESKTOP_DISTANCE_FROM_CARD_LOCATION_INDICATOR;
#endif
    
    if (inBoardDropThreshold &&
        (mActionEngine->GetActiveGameActionName() == IDLE_GAME_ACTION_NAME || mActionEngine->GetActiveGameActionName() == PLAY_CARD_ACTION_NAME) &&
        mBoardState->GetActivePlayerIndex() == 1 &&
        mRuleEngine->CanCardBePlayed(&cardSoWrapper->mCardData, cardIndex, game_constants::LOCAL_PLAYER_INDEX))
    {
        bool inPendingCardsToBePlayed = std::find(mPendingCardsToBePlayed.begin(), mPendingCardsToBePlayed.end(), cardSoWrapper) != mPendingCardsToBePlayed.end();
        if ((mCanPlayNextCard && !inPendingCardsToBePlayed) || mPendingCardReleasedThisFrame == cardSoWrapper)
        {
            mActionEngine->AddGameAction(PLAY_CARD_ACTION_NAME, {{PlayCardGameAction::LAST_PLAYED_CARD_INDEX_PARAM, std::to_string(cardIndex)}});
            mCanPlayNextCard = false;
        }
        else
        {
            if (!inPendingCardsToBePlayed)
            {
                mPendingCardsToBePlayed.push_back(cardSoWrapper);
            }
        }
    }
    else if (!inBoardDropThreshold || mCanPlayNextCard)
    {
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        auto originalCardPosition = card_utils::CalculateHeldCardPosition(static_cast<int>(cardIndex), static_cast<int>(localPlayerCards.size()), false, battleScene->GetCamera());
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, originalCardPosition, cardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){cardSoWrapper->mState = CardSoState::IDLE; });
        cardSoWrapper->mState = CardSoState::MOVING_TO_SET_POSITION;
    }

}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::CreateCardHighlighter()
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto& localPlayerCards = mPlayerHeldCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX];
    for (size_t i = 0U; i < localPlayerCards.size(); ++i)
    {
        battleScene->RemoveSceneObject(strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
    }

    auto highlightedCardIter = std::find_if(localPlayerCards.begin(), localPlayerCards.end(), [&](const std::shared_ptr<CardSoWrapper>& cardSoWrapper)
    {
#if defined(MOBILE_FLOW)
        return cardSoWrapper->mState == CardSoState::HIGHLIGHTED || cardSoWrapper->mState == CardSoState::FREE_MOVING;
#else
        return cardSoWrapper->mState == CardSoState::HIGHLIGHTED;
#endif
    });
    if (highlightedCardIter != localPlayerCards.cend())
    {
        auto cardIndex = highlightedCardIter - localPlayerCards.cbegin();
        auto cardHighlighterSo = battleScene->CreateSceneObject(strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(cardIndex)));
        
        cardHighlighterSo->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::ACTION_HIGHLIGHTER_SHADER_NAME);
        cardHighlighterSo->mShaderFloatUniformValues[game_constants::PERLIN_TIME_SPEED_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_TIME_SPEED;
        cardHighlighterSo->mShaderFloatUniformValues[game_constants::PERLIN_RESOLUTION_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_RESOLUTION;
        cardHighlighterSo->mShaderFloatUniformValues[game_constants::PERLIN_CLARITY_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_CLARITY;
        cardHighlighterSo->mShaderBoolUniformValues[game_constants::CARD_HIGHLIGHTER_INVALID_ACTION_UNIFORM_NAME] = !mRuleEngine->CanCardBePlayed(&(*highlightedCardIter)->mCardData, cardIndex, game_constants::LOCAL_PLAYER_INDEX);
        cardHighlighterSo->mPosition = (*highlightedCardIter)->mSceneObject->mPosition;
        cardHighlighterSo->mPosition.z += game_constants::ACTION_HIGLIGHTER_Z_OFFSET;
        cardHighlighterSo->mScale = game_constants::CARD_HIGHLIGHTER_SCALE;
        cardHighlighterSo->mInvisible = true;
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::CreateCardTooltip(const glm::vec3& cardOriginPostion, const std::string& tooltipText, const size_t cardIndex, std::shared_ptr<scene::Scene> scene)
{
    auto tooltipSceneObject = scene->FindSceneObject(CARD_TOOLTIP_SCENE_OBJECT_NAME);
    bool forHistoryScene = scene == CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(HISTORY_SCENE);
    bool shouldBeFlipped = cardIndex >= mPlayerHeldCardSceneObjectWrappers[game_constants::LOCAL_PLAYER_INDEX].size()/2 && cardIndex != 0;
    
    if (forHistoryScene)
    {
        tooltipSceneObject->mPosition = cardOriginPostion + CARD_TOOLTIP_HISTORY_OFFSET;
        tooltipSceneObject->mPosition.x += shouldBeFlipped ? CARD_TOOLTIP_FLIPPED_X_OFFSET : 0.046f;
    }
    else
    {
        tooltipSceneObject->mPosition = cardOriginPostion + CARD_TOOLTIP_OFFSET;
        tooltipSceneObject->mPosition.x += shouldBeFlipped ? CARD_TOOLTIP_FLIPPED_X_OFFSET : 0.0f;
    }
    
    tooltipSceneObject->mInvisible = false;
    tooltipSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    tooltipSceneObject->mShaderFloatUniformValues[CARD_TOOLTIP_REVEAL_THRESHOLD_UNIFORM_NAME] = 0.0f;
   
    if (forHistoryScene)
    {
        tooltipSceneObject->mScale.x = shouldBeFlipped ? -CARD_TOOLTIP_HISTORY_SCALE.x : CARD_TOOLTIP_HISTORY_SCALE.x;
    }
    else
    {
        tooltipSceneObject->mScale.x = shouldBeFlipped ? -CARD_TOOLTIP_SCALE.x : CARD_TOOLTIP_SCALE.x;
    }
    
    auto tooltipTextRows = strutils::StringSplit(tooltipText, '$');
    
    if (tooltipTextRows.size() == 1)
    {
        auto tooltipTextSceneObject = scene->FindSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[1]);
        tooltipTextSceneObject->mPosition = tooltipSceneObject->mPosition;
        tooltipTextSceneObject->mPosition += (forHistoryScene ? 2.0f : 1.0f) * CARD_TOOLTIP_TEXT_OFFSETS[1];
        tooltipTextSceneObject->mPosition.x += shouldBeFlipped ? ((forHistoryScene ? 2.0f : 1.0f) * CARD_TOOLTIP_TEXT_FLIPPED_X_OFFSET) : 0.0f;
        tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        std::get<scene::TextSceneObjectData>(tooltipTextSceneObject->mSceneObjectTypeData).mText = tooltipTextRows[0];
        tooltipTextSceneObject->mInvisible = false;
    }
    else
    {
        for (auto i = 0U; i < tooltipTextRows.size(); ++i)
        {
            assert(i < game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT);
            auto tooltipTextSceneObject = scene->FindSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[i]);
            tooltipTextSceneObject->mPosition = tooltipSceneObject->mPosition;
            tooltipTextSceneObject->mPosition += (forHistoryScene ? 2.0f : 1.0f) * CARD_TOOLTIP_TEXT_OFFSETS[i];
            tooltipTextSceneObject->mPosition.x += shouldBeFlipped ? ((forHistoryScene ? 2.0f : 1.0f) * CARD_TOOLTIP_TEXT_FLIPPED_X_OFFSET) : 0.0f;
            tooltipTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            std::get<scene::TextSceneObjectData>(tooltipTextSceneObject->mSceneObjectTypeData).mText = tooltipTextRows[i];
            tooltipTextSceneObject->mInvisible = false;
        }
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::DestroyCardHighlighterAtIndex(const int index)
{
    mSecsCardHighlighted = 0.0f;
    
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto cardHighlighterName = strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(index));
    battleScene->RemoveSceneObject(cardHighlighterName);
    
    DestroyCardTooltip(battleScene);
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::DestroyCardTooltip(std::shared_ptr<scene::Scene> scene)
{
    auto tooltipSceneObject = scene->FindSceneObject(CARD_TOOLTIP_SCENE_OBJECT_NAME);
    tooltipSceneObject->mInvisible = true;
    
    for (auto i = 0; i < game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT; ++i)
    {
        auto tooltipTextSceneObject = scene->FindSceneObject(CARD_TOOLTIP_TEXT_SCENE_OBJECT_NAMES[i]);
        tooltipTextSceneObject->mInvisible = true;
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::RegisterForEvents()
{
    auto& eventSystem = events::EventSystem::GetInstance();
    
    eventSystem.RegisterForEvent<events::ApplicationMovedToBackgroundEvent>(this, &BattleSceneLogicManager::OnApplicationMovedToBackground);
    eventSystem.RegisterForEvent<events::WindowResizeEvent>(this, &BattleSceneLogicManager::OnWindowResize);
    eventSystem.RegisterForEvent<events::LocalPlayerTurnStarted>(this, &BattleSceneLogicManager::OnLocalPlayerTurnStarted);
    eventSystem.RegisterForEvent<events::EndOfTurnCardDestructionEvent>(this, &BattleSceneLogicManager::OnEndOfTurnCardDestruction);
    eventSystem.RegisterForEvent<events::ImmediateCardDestructionWithRepositionEvent>(this, &BattleSceneLogicManager::OnImmediateCardDestructionWithReposition);
    eventSystem.RegisterForEvent<events::SingleUseHeldCardCopyDestructionWithRepositionEvent>(this, &BattleSceneLogicManager::OnSingleUseHeldCardCopyDestructionWithReposition);
    eventSystem.RegisterForEvent<events::CardCreationEvent>(this, &BattleSceneLogicManager::OnCardCreation);
    eventSystem.RegisterForEvent<events::CardBuffedDebuffedEvent>(this, &BattleSceneLogicManager::OnCardBuffedDebuffed);
    eventSystem.RegisterForEvent<events::HeldCardSwapEvent>(this, &BattleSceneLogicManager::OnHeldCardSwap);
    eventSystem.RegisterForEvent<events::BlockInteractionWithHeldCardsEvent>(this, &BattleSceneLogicManager::OnBlockInteractionWithHeldCards);
    eventSystem.RegisterForEvent<events::ZeroCostTimeEvent>(this, &BattleSceneLogicManager::OnZeroCostTimeEvent);
    eventSystem.RegisterForEvent<events::CardSummoningEvent>(this, &BattleSceneLogicManager::OnCardSummoning);
    eventSystem.RegisterForEvent<events::NewBoardCardCreatedEvent>(this, &BattleSceneLogicManager::OnNewBoardCardCreated);
    eventSystem.RegisterForEvent<events::HeroCardCreatedEvent>(this, &BattleSceneLogicManager::OnHeroCardCreated);
    eventSystem.RegisterForEvent<events::LastCardPlayedFinalizedEvent>(this, &BattleSceneLogicManager::OnLastCardPlayedFinalized);
    eventSystem.RegisterForEvent<events::EmptyDeckCardTokenPlayedEvent>(this, &BattleSceneLogicManager::OnEmptyDeckCardTokenPlayed);
    eventSystem.RegisterForEvent<events::HealthChangeAnimationTriggerEvent>(this, &BattleSceneLogicManager::OnHealthChangeAnimationTrigger);
    eventSystem.RegisterForEvent<events::WeightChangeAnimationTriggerEvent>(this, &BattleSceneLogicManager::OnWeightChangeAnimationTrigger);
    eventSystem.RegisterForEvent<events::BoardSideCardEffectTriggeredEvent>(this, &BattleSceneLogicManager::OnBoardSideCardEffectTriggered);
    eventSystem.RegisterForEvent<events::BoardSideCardEffectEndedEvent>(this, &BattleSceneLogicManager::OnBoardSideCardEffectEnded);
    eventSystem.RegisterForEvent<events::ForceSendCardBackToPositionEvent>(this, &BattleSceneLogicManager::OnForceSendCardBackToPosition);
    eventSystem.RegisterForEvent<events::PoisonStackChangeChangeAnimationTriggerEvent>(this, &BattleSceneLogicManager::OnPoisonStackChangeChangeAnimationTrigger);
    eventSystem.RegisterForEvent<events::ArmorChangeChangeAnimationTriggerEvent>(this, &BattleSceneLogicManager::OnArmorChangeAnimationTrigger);
    eventSystem.RegisterForEvent<events::CardHistoryEntryAdditionEvent>(this, &BattleSceneLogicManager::OnCardHistoryEntryAddition);
    eventSystem.RegisterForEvent<events::StoryBattleWonEvent>(this, &BattleSceneLogicManager::OnStoryBattleWon);
    eventSystem.RegisterForEvent<events::FlawlessVictoryTriggerEvent>(this, &BattleSceneLogicManager::OnFlawlessVictoryTriggered);
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnApplicationMovedToBackground(const events::ApplicationMovedToBackgroundEvent&)
{
    if (!DataRepository::GetInstance().GetNextStoryOpponentName().empty() && !DataRepository::GetInstance().GetQuickPlayData())
    {
        DataRepository::GetInstance().SetNextBattleControlType(mCurrentBattleControlType);
        mBattleSerializer->FlushStateToFile();
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnWindowResize(const events::WindowResizeEvent&)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    // Correct position of held cards
    for (auto j = 0U; j < mPlayerHeldCardSceneObjectWrappers.size(); ++j)
    {
        for (auto i = 0U; i < mPlayerHeldCardSceneObjectWrappers[j].size(); ++i)
        {
            auto cardSoWrapper = mPlayerHeldCardSceneObjectWrappers[j][i];
            if (cardSoWrapper->mState == CardSoState::IDLE)
            {
                cardSoWrapper->mSceneObject->mPosition = card_utils::CalculateHeldCardPosition(i, static_cast<int>(mPlayerHeldCardSceneObjectWrappers[j].size()), j == game_constants::REMOTE_PLAYER_INDEX, battleScene->GetCamera());
            }
        }
    }
    
    // Correct position of other snap to edge scene objects
    battleScene->RecalculatePositionOfEdgeSnappingSceneObjects();
    
    // Fix position of child->parent objects
    auto turnPointerSo = battleScene->FindSceneObject(game_constants::TURN_POINTER_SCENE_OBJECT_NAME);
    auto turnPointerHighlighterSo = battleScene->FindSceneObject(game_constants::TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME);
    turnPointerHighlighterSo->mPosition = turnPointerSo->mPosition;
    turnPointerHighlighterSo->mPosition.z += game_constants::ACTION_HIGLIGHTER_Z_OFFSET;
    
    if (mGuiManager)
    {
        mGuiManager->OnWindowResize();
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnLocalPlayerTurnStarted(const events::LocalPlayerTurnStarted&)
{
    mCanIssueNextTurnInteraction = true;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnEndOfTurnCardDestruction(const events::EndOfTurnCardDestructionEvent& event)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto& cardSoWrappers = event.mIsBoardCard ?
        mPlayerBoardCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)]:
        mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
    
    std::vector<std::shared_ptr<CardSoWrapper>> remainingCards;
    for (int i = 0; i < static_cast<int>(cardSoWrappers.size()); ++i)
    {
        if (std::find_if(event.mCardIndices.cbegin(), event.mCardIndices.cend(), [=](const std::string& index){ return std::stoi(index) == i; }) == event.mCardIndices.end())
        {
            remainingCards.emplace_back(cardSoWrappers[i]);
        }
        else
        {
            DestroyCardHighlighterAtIndex(i);
            battleScene->RemoveSceneObject(cardSoWrappers[i]->mSceneObject->mName);
        }
    }
    
    if (event.mIsBoardCard)
    {
        mPlayerBoardCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)] = remainingCards;
    }
    else
    {
        mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)] = remainingCards;
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnImmediateCardDestructionWithReposition(const events::ImmediateCardDestructionWithRepositionEvent& event)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    const auto& cards = event.mIsBoardCard ?
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mPlayerBoardCards:
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mPlayerHeldCards;
    
    if (!event.mIsBoardCard)
    {
        DestroyCardHighlighterAtIndex(event.mCardIndex);
        
        for (int i = 0; i < cards.size(); ++i)
        {
            auto cardHighlighterName = strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(i));
            auto existingCardHighlighter = battleScene->FindSceneObject(cardHighlighterName);
            
            if (existingCardHighlighter)
            {
                if (event.mCardIndex < i)
                {
                    existingCardHighlighter->mName = strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(i - 1));
                }
            }
        }
    }
    
    const auto& cardIndicesToDestroy = event.mIsBoardCard ?
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mBoardCardIndicesToDestroy:
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mHeldCardIndicesToDestroy;
    auto currentCardCount = card_utils::CalculateNonDeadCardsCount(cards, cardIndicesToDestroy);
    
    auto& cardSoWrappers = event.mIsBoardCard ?
        mPlayerBoardCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)]:
        mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
    battleScene->RemoveSceneObject(cardSoWrappers[event.mCardIndex]->mSceneObject->mName);
    
    
    cardSoWrappers.erase(cardSoWrappers.begin() + event.mCardIndex);
    
    // Animate rest of the cards to position.
    for (int i = 0; i < currentCardCount; ++i)
    {
        auto& currentCardSoWrapper = cardSoWrappers.at(i);
        
        currentCardSoWrapper->mSceneObject->mName = event.mIsBoardCard ?
            strutils::StringId((mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX) + std::to_string(i)):
            strutils::StringId((mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_HELD_CARD_SO_NAME_PREFIX) + std::to_string(i));
        
        auto originalCardPosition = event.mIsBoardCard ?
            card_utils::CalculateBoardCardPosition(i, currentCardCount, event.mForRemotePlayer) :
            card_utils::CalculateHeldCardPosition(i, currentCardCount, event.mForRemotePlayer, battleScene->GetCamera());
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnSingleUseHeldCardCopyDestructionWithReposition(const events::SingleUseHeldCardCopyDestructionWithRepositionEvent& event)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& cardSoWrappers = mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
        
    std::vector<int> indicesToDestroy;
    for (const auto& stringIndex: event.mHeldCardIndicesToDestroy)
    {
        indicesToDestroy.push_back(std::stoi(stringIndex));
    }
    std::sort(indicesToDestroy.begin(), indicesToDestroy.end());
    
    for (int i = static_cast<int>(indicesToDestroy.size() - 1); i >= 0; i--)
    {
        battleScene->RemoveSceneObject(cardSoWrappers[indicesToDestroy[i]]->mSceneObject->mName);
        cardSoWrappers.erase(cardSoWrappers.begin() + indicesToDestroy[i]);
    }
    
    // Animate rest of the cards to position.
    for (int i = 0; i < cardSoWrappers.size(); ++i)
    {
        auto& currentCardSoWrapper = cardSoWrappers.at(i);
        
        currentCardSoWrapper->mSceneObject->mName = strutils::StringId((mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_HELD_CARD_SO_NAME_PREFIX) + std::to_string(i));
        
        auto originalCardPosition = card_utils::CalculateHeldCardPosition(i, static_cast<int>(cardSoWrappers.size()), event.mForRemotePlayer, battleScene->GetCamera());
        
        animationManager.StopAllAnimationsPlayingForSceneObject(currentCardSoWrapper->mSceneObject->mName);
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnCardCreation(const events::CardCreationEvent& event)
{
    mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].push_back(event.mCardSoWrapper);
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnCardBuffedDebuffed(const events::CardBuffedDebuffedEvent& event)
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    if (event.mBoardCard)
    {
        auto& boardSceneObjectWrappers = mPlayerBoardCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
        auto& cardSceneObjectWrapper = boardSceneObjectWrappers[event.mCardIndex];
        auto previousScale = cardSceneObjectWrapper->mSceneObject->mScale;
        auto& playerState = mBoardState->GetPlayerStates()[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX];
        
        battleScene->RemoveSceneObject(cardSceneObjectWrapper->mSceneObject->mName);
        
        boardSceneObjectWrappers[event.mCardIndex] = card_utils::CreateCardSoWrapper
        (
            &cardSceneObjectWrapper->mCardData,
            cardSceneObjectWrapper->mSceneObject->mPosition,
            (event.mForRemotePlayer ? game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX) + std::to_string(event.mCardIndex),
            CardOrientation::FRONT_FACE,
            card_utils::GetCardRarity(cardSceneObjectWrapper->mCardData.mCardId, event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX, *mBoardState),
            true,
            event.mForRemotePlayer,
            true,
            (static_cast<int>(playerState.mPlayerBoardCardStatOverrides.size()) > event.mCardIndex ? playerState.mPlayerBoardCardStatOverrides.at(event.mCardIndex) : CardStatOverrides()),
            playerState.mBoardModifiers.mGlobalCardStatModifiers,
            *battleScene
        );
        boardSceneObjectWrappers[event.mCardIndex]->mSceneObject->mScale = previousScale;
    }
    else
    {
        auto& heldSceneObjectWrappers = mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
        auto& cardSceneObjectWrapper = heldSceneObjectWrappers[event.mCardIndex];
        auto previousScale = cardSceneObjectWrapper->mSceneObject->mScale;
        auto& playerState = mBoardState->GetPlayerStates()[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX];
        
        battleScene->RemoveSceneObject(cardSceneObjectWrapper->mSceneObject->mName);
        
        heldSceneObjectWrappers[event.mCardIndex] = card_utils::CreateCardSoWrapper
        (
            &cardSceneObjectWrapper->mCardData,
            cardSceneObjectWrapper->mSceneObject->mPosition,
            (event.mForRemotePlayer ? game_constants::TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_HELD_CARD_SO_NAME_PREFIX) + std::to_string(event.mCardIndex),
            CardOrientation::FRONT_FACE,
            card_utils::GetCardRarity(cardSceneObjectWrapper->mCardData.mCardId, event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX, *mBoardState),
            false,
            event.mForRemotePlayer,
            mRuleEngine->CanCardBePlayed(&heldSceneObjectWrappers[event.mCardIndex]->mCardData, event.mCardIndex, game_constants::LOCAL_PLAYER_INDEX),
            (static_cast<int>(playerState.mPlayerHeldCardStatOverrides.size()) > event.mCardIndex ? playerState.mPlayerHeldCardStatOverrides.at(event.mCardIndex) : CardStatOverrides()),
            playerState.mBoardModifiers.mGlobalCardStatModifiers,
            *battleScene
        );
        heldSceneObjectWrappers[event.mCardIndex]->mSceneObject->mScale = previousScale;
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnHeldCardSwap(const events::HeldCardSwapEvent& event)
{
    mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)][event.mCardIndex] = event.mCardSoWrapper;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnBlockInteractionWithHeldCards(const events::BlockInteractionWithHeldCardsEvent&)
{
    mCanInteractWithAnyHeldCard = false;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnZeroCostTimeEvent(const events::ZeroCostTimeEvent& event)
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    auto& playerState = mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
    auto& heldSceneObjectWrappers = mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
    
    for (auto i = 0; i < heldSceneObjectWrappers.size(); ++i)
    {
        auto& cardSceneObjectWrapper = heldSceneObjectWrappers[i];
        auto previousScale = cardSceneObjectWrapper->mSceneObject->mScale;
        
        battleScene->RemoveSceneObject(cardSceneObjectWrapper->mSceneObject->mName);
        
        heldSceneObjectWrappers[i] = card_utils::CreateCardSoWrapper
        (
            &cardSceneObjectWrapper->mCardData,
            cardSceneObjectWrapper->mSceneObject->mPosition,
            (event.mForRemotePlayer ? game_constants::TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_HELD_CARD_SO_NAME_PREFIX) + std::to_string(i),
            (event.mForRemotePlayer ? CardOrientation::BACK_FACE : CardOrientation::FRONT_FACE),
            card_utils::GetCardRarity(cardSceneObjectWrapper->mCardData.mCardId, event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX, *mBoardState),
            false,
            event.mForRemotePlayer,
            event.mZeroCostTimeEnabled ? true : mRuleEngine->CanCardBePlayed(&heldSceneObjectWrappers[i]->mCardData, i, event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX),
            (static_cast<int>(playerState.mPlayerHeldCardStatOverrides.size()) > i ? playerState.mPlayerHeldCardStatOverrides.at(i) : CardStatOverrides()),
            playerState.mBoardModifiers.mGlobalCardStatModifiers,
            *battleScene
        );
        heldSceneObjectWrappers[i]->mSceneObject->mScale = previousScale;
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnCardSummoning(const events::CardSummoningEvent& event)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    const auto& boardCards = mBoardState->GetActivePlayerState().mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetActivePlayerState().mBoardCardIndicesToDestroy;
    const auto& boardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    auto& playerBoardCardSoWrappers = mPlayerBoardCardSceneObjectWrappers[mBoardState->GetActivePlayerIndex()];
    
    for (int i = 0; i < boardCardCount - static_cast<int>(event.mCardSoWrappers.size()); ++i)
    {
        auto& currentCardSoWrapper = playerBoardCardSoWrappers.at(i);
        auto originalCardPosition = card_utils::CalculateBoardCardPosition(i, boardCardCount, mBoardState->GetActivePlayerIndex() == 0);
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    }
    
    for (int i = 0; i < event.mCardSoWrappers.size(); ++i)
    {
        playerBoardCardSoWrappers.push_back(event.mCardSoWrappers[i]);
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnNewBoardCardCreated(const events::NewBoardCardCreatedEvent& event)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& playerBoardCardSoWrappers = mPlayerBoardCardSceneObjectWrappers[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX];
    
    playerBoardCardSoWrappers.push_back(event.mCardSoWrapper);
    
    const auto& boardCards = mBoardState->GetPlayerStates().at(mBoardState->GetActivePlayerIndex()).mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetPlayerStates().at(mBoardState->GetActivePlayerIndex()).mBoardCardIndicesToDestroy;
    const auto& boardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    
    // Animate and rename board cards to position. Last one will be animated externally
    for (int i = 0; i < boardCardCount; ++i)
    {
        auto& currentCardSoWrapper = playerBoardCardSoWrappers.at(i);
        if (i !=  boardCardCount - 1)
        {
            auto originalCardPosition = card_utils::CalculateBoardCardPosition(i, boardCardCount, mBoardState->GetActivePlayerIndex() == 0);
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
        }
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnHeroCardCreated(const events::HeroCardCreatedEvent& event)
{
    mBoardState->GetPlayerStates()[game_constants::REMOTE_PLAYER_INDEX].mHasHeroCard = true;
    auto& playerBoardCardSoWrappers = mPlayerBoardCardSceneObjectWrappers[game_constants::REMOTE_PLAYER_INDEX];
    playerBoardCardSoWrappers.push_back(event.mCardSoWrapper);
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnLastCardPlayedFinalized(const events::LastCardPlayedFinalizedEvent& event)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    battleScene->RemoveSceneObject(strutils::StringId(CARD_HIGHLIGHTER_SCENE_OBJECT_NAME_PREFIX + std::to_string(event.mCardIndex)));
    
    auto& playerHeldCardSoWrappers = mPlayerHeldCardSceneObjectWrappers[mBoardState->GetActivePlayerIndex()];
    auto& playerBoardCardSoWrappers = mPlayerBoardCardSceneObjectWrappers[mBoardState->GetActivePlayerIndex()];
    
    playerBoardCardSoWrappers.push_back(playerHeldCardSoWrappers[event.mCardIndex]);
    playerHeldCardSoWrappers.erase(playerHeldCardSoWrappers.begin() + event.mCardIndex);
    
    const auto currentPlayerHeldCardCount = static_cast<int>(playerHeldCardSoWrappers.size());
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    for (int i = 0; i < currentPlayerHeldCardCount; ++i)
    {
        auto& currentCardSoWrapper = playerHeldCardSoWrappers.at(i);
        
        // Rename held cards for different indices
        currentCardSoWrapper->mSceneObject->mName = strutils::StringId((mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_HELD_CARD_SO_NAME_PREFIX) + std::to_string(i));
        
        // Reposition held cards for different indices
        if (currentCardSoWrapper->mState != CardSoState::FREE_MOVING)
        {
            auto originalCardPosition = card_utils::CalculateHeldCardPosition(i, currentPlayerHeldCardCount, mBoardState->GetActivePlayerIndex() == 0, battleScene->GetCamera());
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){ currentCardSoWrapper->mState = CardSoState::IDLE; });
            currentCardSoWrapper->mState = CardSoState::MOVING_TO_SET_POSITION;
        }
    }
    
    const auto& boardCards = mBoardState->GetPlayerStates().at(mBoardState->GetActivePlayerIndex()).mPlayerBoardCards;
    const auto& deadBoardCardIndices = mBoardState->GetPlayerStates().at(mBoardState->GetActivePlayerIndex()).mBoardCardIndicesToDestroy;
    const auto& boardCardCount = card_utils::CalculateNonDeadCardsCount(boardCards, deadBoardCardIndices);
    
    // Animate and rename board cards to position. Last one will be animated externally
    for (int i = 0; i < boardCardCount; ++i)
    {
        auto& currentCardSoWrapper = playerBoardCardSoWrappers.at(i);
        currentCardSoWrapper->mSceneObject->mName = strutils::StringId((mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX) + std::to_string(i));
        
        if (i !=  boardCardCount - 1)
        {
            auto originalCardPosition = card_utils::CalculateBoardCardPosition(i, boardCardCount, mBoardState->GetActivePlayerIndex() == 0);
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(currentCardSoWrapper->mSceneObject, originalCardPosition, currentCardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
        }
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnEmptyDeckCardTokenPlayed(const events::EmptyDeckCardTokenPlayedEvent&)
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    const auto& newCardId = mBoardState->GetActivePlayerState().mPlayerBoardCards.back();
    const auto& newCardData = CardDataRepository::GetInstance().GetCardData(newCardId, mBoardState->GetActivePlayerIndex());
    auto& boardSceneObjectWrappers = mPlayerBoardCardSceneObjectWrappers[mBoardState->GetActivePlayerIndex()];
    const auto newCardIndex = boardSceneObjectWrappers.size() - 1;
    auto newCardPosition = boardSceneObjectWrappers.back()->mSceneObject->mPosition;
    auto newCardScale = boardSceneObjectWrappers.back()->mSceneObject->mScale;
    newCardPosition.z -= 0.001f;
    
    boardSceneObjectWrappers.push_back(card_utils::CreateCardSoWrapper
    (
        &newCardData,
        newCardPosition,
        (mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX ? game_constants::TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX : game_constants::BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX) + std::to_string(newCardIndex),
        CardOrientation::FRONT_FACE,
        card_utils::GetCardRarity(newCardId, mBoardState->GetActivePlayerIndex(), *mBoardState),
        true,
        mBoardState->GetActivePlayerIndex() == game_constants::REMOTE_PLAYER_INDEX,
        true,
        {},
        mBoardState->GetActivePlayerState().mBoardModifiers.mGlobalCardStatModifiers,
        *battleScene
    ));
    
    boardSceneObjectWrappers.back()->mSceneObject->mScale = newCardScale/2.0f;
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(boardSceneObjectWrappers.back()->mSceneObject, boardSceneObjectWrappers.back()->mSceneObject->mPosition, newCardScale, EMPTY_DECK_CARD_TOKEN_NEW_CARD_SCALE_IN_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnHealthChangeAnimationTrigger(const events::HealthChangeAnimationTriggerEvent& event)
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    auto& playerState = mBoardState->GetPlayerStates()[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX];
    auto& animatedStateContainer = mAnimatedStatContainers[event.mForRemotePlayer ? 0 : 1];
    animatedStateContainer.first = true;
    
    if (math::Abs(animatedStateContainer.second->GetDisplayedValue() - playerState.mPlayerHealth) <= 0 || mAnimatedStatContainers[0].second->GetDisplayedValue() <= 0)
    {
        return;
    }
    
    auto existingHealthChangeTextSceneObjectCount = battleScene->FindSceneObjectsWhoseNameStartsWith(event.mForRemotePlayer ? HEALTH_CHANGE_TEXT_TOP_SCENE_OBJECT_NAME_PREFIX : HEALTH_CHANGE_TEXT_BOT_SCENE_OBJECT_NAME_PREFIX).size();
    
    scene::TextSceneObjectData healthChangeTextData;
    healthChangeTextData.mFontName = game_constants::DEFAULT_FONT_NAME;
    healthChangeTextData.mText = std::to_string(math::Abs(animatedStateContainer.second->GetDisplayedValue() - playerState.mPlayerHealth));
    healthChangeTextData.mText = animatedStateContainer.second->GetDisplayedValue() > playerState.mPlayerHealth ? "-" + healthChangeTextData.mText : "+" + healthChangeTextData.mText;
    
    auto healthChangeTextSceneObject = battleScene->CreateSceneObject(strutils::StringId(event.mForRemotePlayer ? HEALTH_CHANGE_TEXT_TOP_SCENE_OBJECT_NAME_PREFIX : HEALTH_CHANGE_TEXT_BOT_SCENE_OBJECT_NAME_PREFIX + std::to_string(existingHealthChangeTextSceneObjectCount)));
    healthChangeTextSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::BASIC_CUSTOM_COLOR_SHADER_FILE_NAME);
    healthChangeTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
    healthChangeTextSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = healthChangeTextData.mText.front() == '+' ? HEALTH_CHANGE_TEXT_COLOR_GAIN : HEALTH_CHANGE_TEXT_COLOR_LOSS;
    healthChangeTextSceneObject->mScale = HEALTH_CHANGE_TEXT_SCALE;
    healthChangeTextSceneObject->mPosition = event.mForRemotePlayer ? game_constants::HEALTH_CRYSTAL_TOP_POSITION : game_constants::HEALTH_CRYSTAL_BOT_POSITION;
    healthChangeTextSceneObject->mPosition += HEALTH_CHANGE_TEXT_OFFSET;
    healthChangeTextSceneObject->mSceneObjectTypeData = std::move(healthChangeTextData);
        
    auto targetPosition = healthChangeTextSceneObject->mPosition;
    targetPosition.y += HEALTH_CHANGE_TARGET_Y_OFFSET;
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(healthChangeTextSceneObject, 0.0f, HEALTH_CHANGE_TEXT_ANIMATION_DURATION_SECS, animation_flags::NONE, HEALTH_CHANGE_TEXT_ANIMATION_DELAY_SECS), [](){});
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(healthChangeTextSceneObject, targetPosition, healthChangeTextSceneObject->mScale, HEALTH_CHANGE_TEXT_ANIMATION_DURATION_SECS, animation_flags::NONE, HEALTH_CHANGE_TEXT_ANIMATION_DELAY_SECS), [=]()
    {
        battleScene->RemoveSceneObject(healthChangeTextSceneObject->mName);
    });
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnWeightChangeAnimationTrigger(const events::WeightChangeAnimationTriggerEvent& event)
{
    mAnimatedStatContainers[event.mForRemotePlayer ? 2 : 3].first = true;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnBoardSideCardEffectTriggered(const events::BoardSideCardEffectTriggeredEvent& event)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& animationManager = systemsEngine.GetAnimationManager();
    const auto& sceneManager = systemsEngine.GetSceneManager();
    
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    std::shared_ptr<scene::SceneObject> sideEffectSceneObject = nullptr;
    if (event.mEffectBoardModifierMask == effects::board_modifier_masks::BOARD_SIDE_DEBUFF)
    {
        sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
    }
    else
    {
        sideEffectSceneObject = nullptr;
        if (event.mEffectBoardModifierMask == effects::board_modifier_masks::KILL_NEXT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DEMON_KILL_NEXT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::DEMON_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DEMON_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::SPELL_KILL_NEXT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::SPELL_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::SPELL_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DUPLICATE_NEXT_INSECT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::INSECT_DUPLICATION_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::INSECT_DUPLICATION_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::NEXT_DINO_DAMAGE_DOUBLING_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::NEXT_DINO_DAMAGE_DOUBLING_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::NEXT_DINO_HEAL_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::NEXT_DINO_HEAL_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DOUBLE_POISON_ATTACKS)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::DOUBLE_POISON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DOUBLE_POISON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::INSECT_VIRUS)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::INSECT_VIRUS_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::INSECT_VIRUS_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DIG_NO_FAIL)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::DIG_NO_FAIL_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DIG_NO_FAIL_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::RODENT_LIFESTEAL)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        
        assert(sideEffectSceneObject);
        
        if (!sideEffectSceneObject->mInvisible)
        {
            // effect already showing (and not additive like the the side stat modifier above)
            return;
        }
        
        sideEffectSceneObject->mScale = game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_SCALE;
        sideEffectSceneObject->mRotation = glm::vec3(0.0f);
        animationManager.StartAnimation(std::make_unique<rendering::PulseAnimation>(sideEffectSceneObject, game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_SCALE_UP_FACTOR, game_constants::INDIVIDUAL_CARD_BOARD_EFFECT_PULSE_ANIMATION_PULSE_DURATION_SECS, animation_flags::ANIMATE_CONTINUOUSLY), [](){});
        
        auto& activeEffects = mActiveIndividualCardBoardEffectSceneObjects[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX];
        if (std::find(activeEffects.cbegin(), activeEffects.cend(), sideEffectSceneObject) == activeEffects.cend())
        {
            activeEffects.push_back(sideEffectSceneObject);
            
            for (auto i = 0U; i < activeEffects.size(); ++i)
            {
                const auto& targetPosition = CalculateBoardEffectPosition(i, activeEffects.size(), event.mForRemotePlayer);
                animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(activeEffects[i], targetPosition, activeEffects[i]->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
            }
        }
    }
    
    sideEffectSceneObject->mInvisible = false;
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sideEffectSceneObject, BOARD_EFFECT_MAX_ALPHA, BOARD_SIDE_EFFECT_SHOWING_HIDING_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [](){});
    
    
    // Update text specifically for board side stat modifier
    if (event.mEffectBoardModifierMask == effects::board_modifier_masks::BOARD_SIDE_DEBUFF)
    {
        for (int i = 0; i < game_constants::BOARD_SIDE_EFFECT_VALUE_SO_COUNT; ++i)
        {
            auto boardSideEffectValueSceneObject = battleScene->FindSceneObject(strutils::StringId((event.mForRemotePlayer ? game_constants::BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME_PRE_FIX : game_constants::BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME_PRE_FIX) + std::to_string(i)));
            boardSideEffectValueSceneObject->mInvisible = false;
            
            std::get<scene::TextSceneObjectData>(boardSideEffectValueSceneObject->mSceneObjectTypeData).mText = std::to_string(mBoardState->GetPlayerStates()[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX].mBoardModifiers.mGlobalCardStatModifiers.at(CardStatType::DAMAGE));
            
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(boardSideEffectValueSceneObject, BOARD_EFFECT_MAX_ALPHA * 2, BOARD_SIDE_EFFECT_SHOWING_HIDING_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [](){});
        }
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnBoardSideCardEffectEnded(const events::BoardSideCardEffectEndedEvent& event)
{
    auto& systemsEngine = CoreSystemsEngine::GetInstance();
    auto& animationManager = systemsEngine.GetAnimationManager();
    const auto& sceneManager = systemsEngine.GetSceneManager();
    
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    
    std::shared_ptr<scene::SceneObject> sideEffectSceneObject = nullptr;
    if (event.mEffectBoardModifierMask == effects::board_modifier_masks::BOARD_SIDE_DEBUFF)
    {
        sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
    }
    else
    {
        sideEffectSceneObject = nullptr;
        if (event.mEffectBoardModifierMask == effects::board_modifier_masks::KILL_NEXT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DEMON_KILL_NEXT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::DEMON_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DEMON_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::SPELL_KILL_NEXT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::SPELL_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::SPELL_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DUPLICATE_NEXT_INSECT)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::INSECT_DUPLICATION_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::INSECT_DUPLICATION_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DOUBLE_NEXT_DINO_DAMAGE)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::NEXT_DINO_DAMAGE_DOUBLING_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::NEXT_DINO_DAMAGE_DOUBLING_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::HEAL_NEXT_DINO_DAMAGE)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::NEXT_DINO_HEAL_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::NEXT_DINO_HEAL_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DOUBLE_POISON_ATTACKS)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::DOUBLE_POISON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DOUBLE_POISON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::INSECT_VIRUS)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::INSECT_VIRUS_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::INSECT_VIRUS_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::DIG_NO_FAIL)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::DIG_NO_FAIL_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::DIG_NO_FAIL_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::RODENT_LIFESTEAL)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::PERMANENT_CONTINUAL_WEIGHT_REDUCTION)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        else if (event.mEffectBoardModifierMask == effects::board_modifier_masks::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST)
        {
            sideEffectSceneObject = battleScene->FindSceneObject(event.mForRemotePlayer ? game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_TOP_SCENE_OBJECT_NAME : game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_BOT_SCENE_OBJECT_NAME);
        }
        
        assert(sideEffectSceneObject);
        
        auto& activeEffects = mActiveIndividualCardBoardEffectSceneObjects[event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX];
        if (!activeEffects.empty())
        {
            auto foundIter = std::find(activeEffects.cbegin(), activeEffects.cend(), sideEffectSceneObject);
            if (foundIter != activeEffects.end())
            {
                activeEffects.erase(std::find(activeEffects.cbegin(), activeEffects.cend(), sideEffectSceneObject));
            }
            
            for (auto i = 0U; i < activeEffects.size(); ++i)
            {
                if (!event.mMassClear || (activeEffects[i]->mName == game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_TOP_SCENE_OBJECT_NAME || activeEffects[i]->mName == game_constants::PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_BOT_SCENE_OBJECT_NAME || activeEffects[i]->mName == game_constants::INSECT_VIRUS_EFFECT_TOP_SCENE_OBJECT_NAME || activeEffects[i]->mName == game_constants::INSECT_VIRUS_EFFECT_BOT_SCENE_OBJECT_NAME || activeEffects[i]->mName == game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_TOP_SCENE_OBJECT_NAME || activeEffects[i]->mName == game_constants::EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_BOT_SCENE_OBJECT_NAME))
                {
                    const auto& targetPosition = CalculateBoardEffectPosition(i, activeEffects.size(), event.mForRemotePlayer);
                    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(activeEffects[i], targetPosition, activeEffects[i]->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
                }
            }
        }
    }
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sideEffectSceneObject, 0.0f, BOARD_SIDE_EFFECT_SHOWING_HIDING_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [=]()
    {
        sideEffectSceneObject->mInvisible = true;
    });
    
    if (event.mEffectBoardModifierMask == effects::board_modifier_masks::BOARD_SIDE_DEBUFF)
    {
        for (int i = 0; i < game_constants::BOARD_SIDE_EFFECT_VALUE_SO_COUNT; ++i)
        {
            auto boardSideEffectValueSceneObject = battleScene->FindSceneObject(strutils::StringId((event.mForRemotePlayer ? game_constants::BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME_PRE_FIX : game_constants::BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME_PRE_FIX) + std::to_string(i)));
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(boardSideEffectValueSceneObject, 0.0f, BOARD_SIDE_EFFECT_SHOWING_HIDING_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [=]()
            {
                boardSideEffectValueSceneObject->mInvisible = true;
            });
        }
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnForceSendCardBackToPosition(const events::ForceSendCardBackToPositionEvent& event)
{
    const auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    auto battleScene = sceneManager.FindScene(game_constants::BATTLE_SCENE);
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    const auto& cards = event.mBoardCard ?
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mPlayerBoardCards:
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mPlayerHeldCards;
    const auto& cardIndicesToDestroy = event.mBoardCard ?
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mBoardCardIndicesToDestroy:
        mBoardState->GetPlayerStates()[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)].mHeldCardIndicesToDestroy;
    auto currentCardCount = card_utils::CalculateNonDeadCardsCount(cards, cardIndicesToDestroy);
    
    auto& cardSoWrappers = event.mBoardCard ?
        mPlayerBoardCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)]:
        mPlayerHeldCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)];
    
    auto& cardSoWrapper = cardSoWrappers[event.mCardIdex];
    
    cardSoWrapper->mState = CardSoState::IDLE;
    
    auto originalCardPosition = event.mBoardCard ?
        card_utils::CalculateBoardCardPosition(event.mCardIdex, currentCardCount, event.mForRemotePlayer) :
        card_utils::CalculateHeldCardPosition(event.mCardIdex, currentCardCount, event.mForRemotePlayer, battleScene->GetCamera());
    animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(cardSoWrapper->mSceneObject, originalCardPosition, cardSoWrapper->mSceneObject->mScale, CARD_SELECTION_ANIMATION_DURATION, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=](){});
    
    DestroyCardHighlighterAtIndex(event.mCardIdex);
    
    mCanInteractWithAnyHeldCard = false;
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnPoisonStackChangeChangeAnimationTrigger(const events::PoisonStackChangeChangeAnimationTriggerEvent& event)
{
    auto& affectedContainerEntry = mAnimatedStatContainers[event.mForRemotePlayer ? 4 : 5];
    affectedContainerEntry.first = true;
    auto newPoisonStackValue = event.mNewPoisonStackValue;
    
    for (auto sceneObject: affectedContainerEntry.second->GetSceneObjects())
    {
        if (newPoisonStackValue != 0)
        {
            sceneObject->mInvisible = false;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, newPoisonStackValue == 0 ? 0.0f : 1.0f, game_constants::POISON_STACK_SHOW_HIDE_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [=]()
        {
            if (newPoisonStackValue == 0)
            {
                sceneObject->mInvisible = true;
            }
        });
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnArmorChangeAnimationTrigger(const events::ArmorChangeChangeAnimationTriggerEvent& event)
{
    auto& affectedContainerEntry = mAnimatedStatContainers[event.mForRemotePlayer ? 6 : 7];
    affectedContainerEntry.first = true;
    auto newArmorValue = event.mNewArmorValue;
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    for (auto sceneObject: affectedContainerEntry.second->GetSceneObjects())
    {
        if (newArmorValue != 0)
        {
            if (animationManager.GetAnimationCountPlayingForSceneObject(sceneObject->mName) != 0)
            {
                animationManager.StopAllAnimationsPlayingForSceneObject(sceneObject->mName);
            }
            sceneObject->mInvisible = false;
        }
        
        animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, newArmorValue == 0 ? 0.0f : 1.0f, game_constants::ARMOR_SHOW_HIDE_ANIMATION_DURATION_SECS, animation_flags::NONE, 0.0f, math::LinearFunction, math::TweeningMode::EASE_IN), [=]()
        {
            if (newArmorValue == 0)
            {
                sceneObject->mInvisible = true;
            }
        });
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnCardHistoryEntryAddition(const events::CardHistoryEntryAdditionEvent& event)
{
    auto& sceneManager = CoreSystemsEngine::GetInstance().GetSceneManager();
    
    auto historyEntrySceneObject = sceneManager.FindScene(HISTORY_SCENE)->CreateSceneObject();
    
    if (event.mIsTurnCounter)
    {
        historyEntrySceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + TURN_COUNTER_HISTORY_ENTRY_SHADER_FILE_NAME);
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_X_UNIFORM_NAME] = CARD_HISTORY_CONTAINER_BOUNDS.bottomLeft.x;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_X_UNIFORM_NAME] = CARD_HISTORY_CONTAINER_BOUNDS.topRight.x;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        historyEntrySceneObject->mShaderBoolUniformValues[game_constants::CARD_HIGHLIGHTER_INVALID_ACTION_UNIFORM_NAME] = event.mForRemotePlayer;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::PERLIN_TIME_SPEED_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_TIME_SPEED;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::PERLIN_RESOLUTION_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_RESOLUTION;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::PERLIN_CLARITY_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_CLARITY;
        historyEntrySceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + HISTORY_ENTRY_TURN_COUNTER_MASK_TEXTURE_FILE_NAME);
        historyEntrySceneObject->mScale = CARD_HISTORY_TURN_COUNTER_ENTRY_SCALE;
        historyEntrySceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + TURN_COUNTER_HISTORY_ENTRY_TEXTURE_FILE_NAME);
        historyEntrySceneObject->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
        historyEntrySceneObject->mInvisible = true;
        
        auto turnCounterStringSceneObject = sceneManager.FindScene(HISTORY_SCENE)->CreateSceneObject();
        scene::TextSceneObjectData turnCounterTextData;
        turnCounterTextData.mFontName = game_constants::DEFAULT_FONT_BLACK_NAME;
        turnCounterTextData.mText = "Turn " + std::to_string(mBoardState->GetTurnCounter() + 1);
        turnCounterStringSceneObject->mSceneObjectTypeData = std::move(turnCounterTextData);
        turnCounterStringSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + TURN_COUNTER_STRING_HISTORY_ENTRY_SHADER_FILE_NAME);
        turnCounterStringSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        turnCounterStringSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_X_UNIFORM_NAME] = CARD_HISTORY_CONTAINER_BOUNDS.bottomLeft.x;
        turnCounterStringSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_X_UNIFORM_NAME] = CARD_HISTORY_CONTAINER_BOUNDS.topRight.x;
        turnCounterStringSceneObject->mPosition += CARD_HISTORY_TURN_COUNTER_TEXT_OFFSET; // Offset to be considered by SwipeableContainer
        turnCounterStringSceneObject->mScale = glm::vec3(CARD_TOOLTIP_TEXT_FONT_SIZE * 1.8f);
        turnCounterStringSceneObject->mInvisible = true;
        mCardHistoryContainer->AddItem({{historyEntrySceneObject, turnCounterStringSceneObject}, 0, false, true}, EntryAdditionStrategy::ADD_IN_FRONT);
    }
    else
    {
        auto cardSoWrapper = mPlayerBoardCardSceneObjectWrappers[(event.mForRemotePlayer ? game_constants::REMOTE_PLAYER_INDEX : game_constants::LOCAL_PLAYER_INDEX)][event.mCardIndex];
        historyEntrySceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + CARD_HISTORY_ENTRY_SHADER_FILE_NAME);
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_X_UNIFORM_NAME] = CARD_HISTORY_CONTAINER_BOUNDS.bottomLeft.x;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_X_UNIFORM_NAME] = CARD_HISTORY_CONTAINER_BOUNDS.topRight.x;
        historyEntrySceneObject->mShaderBoolUniformValues[game_constants::CARD_HIGHLIGHTER_INVALID_ACTION_UNIFORM_NAME] = event.mForRemotePlayer;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::PERLIN_TIME_SPEED_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_TIME_SPEED;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::PERLIN_RESOLUTION_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_RESOLUTION;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::PERLIN_CLARITY_UNIFORM_NAME] = game_constants::ACTION_HIGLIGHTER_PERLIN_CLARITY;
        historyEntrySceneObject->mShaderIntUniformValues[game_constants::CARD_DAMAGE_INTERACTIVE_MODE_UNIFORM_NAME] = cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_DAMAGE_INTERACTIVE_MODE_UNIFORM_NAME];
        historyEntrySceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME] = cardSoWrapper->mSceneObject->mShaderIntUniformValues[game_constants::CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME];
        historyEntrySceneObject->mScale = CARD_HISTORY_ENTRY_SCALE;
        historyEntrySceneObject->mTextureResourceId = cardSoWrapper->mSceneObject->mTextureResourceId;
        historyEntrySceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (cardSoWrapper->mCardData.IsSpell() ? game_constants::GOLDEN_SPELL_CARD_FLAKES_MASK_TEXTURE_FILE_NAME : game_constants::GOLDEN_CARD_FLAKES_MASK_TEXTURE_FILE_NAME));
        historyEntrySceneObject->mEffectTextureResourceIds[1] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + (cardSoWrapper->mCardData.IsSpell() ? HISTORY_ENTRY_SPELL_MASK_TEXTURE_FILE_NAME : HISTORY_ENTRY_MASK_TEXTURE_FILE_NAME));
        historyEntrySceneObject->mEffectTextureResourceIds[2] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + event.mEntryTypeTextureFileName);
        historyEntrySceneObject->mBoundingRectMultiplier.x = game_constants::CARD_BOUNDING_RECT_X_MULTIPLIER;
        historyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        historyEntrySceneObject->mInvisible = true;
        mCardHistoryContainer->AddItem({{historyEntrySceneObject}, cardSoWrapper->mCardData.mCardId, event.mForRemotePlayer, event.mIsTurnCounter}, EntryAdditionStrategy::ADD_IN_FRONT);
    }
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnStoryBattleWon(const events::StoryBattleWonEvent&)
{
    auto healthReward = DataRepository::GetInstance().GetNextStoryOpponentDamage();
    auto battleCoinRewards = DataRepository::GetInstance().GetNextBattleTopPlayerHealth();
    bool flawlessVictoryCase = mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth >= DataRepository::GetInstance().StoryCurrentHealth().GetValue();
    
    if (flawlessVictoryCase)
    {
        battleCoinRewards *= 5;
    }
    
    if (DataRepository::GetInstance().GetNextStoryOpponentName() == game_constants::EMERALD_DRAGON_NAME.GetString())
    {
        battleCoinRewards *= 5;
    }
    
    if (DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD && DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP)
    {
        battleCoinRewards *= 7;
    }
    
    if (DataRepository::GetInstance().GetCurrentStoryMapNodeType() == StoryMap::NodeType::BOSS_ENCOUNTER)
    {
        battleCoinRewards *= 10;
        healthReward = 0;
    }
    
    auto greedyGoblinCount = DataRepository::GetInstance().GetStoryArtifactCount(artifacts::GREEDY_GOBLIN);
    if (greedyGoblinCount > 0)
    {
        battleCoinRewards *= 2 * greedyGoblinCount;
    }
    
    if (DataRepository::GetInstance().DoesCurrentStoryHaveMutation(game_constants::MUTATION_HALF_COINS))
    {
        battleCoinRewards /= 2;
    }
    
    // Add victory coin loot indicator and animate coins to gui
    auto coinsLootIndicatorSceneObject = mActiveScene->FindSceneObject(COINS_LOOT_INDICATOR_SCENE_OBJECT_NAME);
    coinsLootIndicatorSceneObject->mInvisible = false;
    std::get<scene::TextSceneObjectData>(coinsLootIndicatorSceneObject->mSceneObjectTypeData).mText = "+" + std::to_string(battleCoinRewards) + " coins";
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(coinsLootIndicatorSceneObject, 1.0f, 0.5f), [=](){});
    events::EventSystem::GetInstance().DispatchEvent<events::CoinRewardEvent>(battleCoinRewards, mPlayerBoardCardSceneObjectWrappers[game_constants::REMOTE_PLAYER_INDEX][0]->mSceneObject->mPosition);
    
    // Commit health values
    DataRepository::GetInstance().StoryCurrentHealth().SetValue(mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth);
    DataRepository::GetInstance().StoryCurrentHealth().SetDisplayedValue(mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth);
    mGuiManager->ForceSetStoryHealthValue(mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mPlayerHealth);
    mAnimatedStatContainers[1].second->ChangeTrackedValue(&DataRepository::GetInstance().StoryCurrentHealth().GetDisplayedValue());
    
    // Emerald Dragon Event also adds an artifact
    if (DataRepository::GetInstance().GetNextStoryOpponentName() == game_constants::EMERALD_DRAGON_NAME.GetString())
    {
        auto rareItemProductNames = ProductRepository::GetInstance().GetRareItemProductNames();
        for (auto iter = rareItemProductNames.begin(); iter != rareItemProductNames.end();)
        {
            if (ProductRepository::GetInstance().GetProductDefinition(*iter).mUnique &&
                DataRepository::GetInstance().GetStoryArtifactCount(*iter) > 0)
            {
                iter = rareItemProductNames.erase(iter);
            }
            else
            {
                iter++;
            }
        }
        
        auto rareItemReward = rareItemProductNames[math::ControlledRandomInt() % rareItemProductNames.size()];
        const auto& rareItemDefinition = ProductRepository::GetInstance().GetProductDefinition(rareItemReward);
        
        auto rareItemSceneObject = mActiveScene->CreateSceneObject();
        rareItemSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + RARE_ITEM_SHADER);
        rareItemSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + std::get<std::string>(rareItemDefinition.mProductTexturePathOrCardId));
        rareItemSceneObject->mPosition.z += RARE_ITEM_Z_OFFSET;
        rareItemSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        rareItemSceneObject->mScale = RARE_ITEM_INIT_SCALE;
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(rareItemSceneObject, rareItemSceneObject->mPosition, RARE_ITEM_TARGET_SCALE, RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS), [=]()
        {
            events::EventSystem::GetInstance().DispatchEvent<events::RareItemCollectedEvent>(rareItemReward, rareItemSceneObject);
        });
    }
    
    // Commit Artifact changes
    if (mBoardState->GetPlayerStates()[game_constants::LOCAL_PLAYER_INDEX].mHasResurrectionActive)
    {
        DataRepository::GetInstance().SetStoryArtifactCount(artifacts::GUARDIAN_ANGEL, 1);
    }
    else
    {
        auto currentStoryArtifacts = DataRepository::GetInstance().GetCurrentStoryArtifacts();
        currentStoryArtifacts.erase(std::remove_if(currentStoryArtifacts.begin(), currentStoryArtifacts.end(), [](const std::pair<strutils::StringId, int>& artifactEntry)
        {
            return artifactEntry.first == artifacts::GUARDIAN_ANGEL;
        }), currentStoryArtifacts.end());
        DataRepository::GetInstance().SetCurrentStoryArtifacts(currentStoryArtifacts);
    }
    
    auto eligibleHealthPointsAdded = 0;
    while (DataRepository::GetInstance().StoryCurrentHealth().GetValue() < DataRepository::GetInstance().GetStoryMaxHealth() && eligibleHealthPointsAdded < healthReward)
    {
        DataRepository::GetInstance().StoryCurrentHealth().SetValue(DataRepository::GetInstance().StoryCurrentHealth().GetValue() + 1);
        eligibleHealthPointsAdded++;
    }
    
    if (eligibleHealthPointsAdded > 0)
    {
        // Add victory health loot indicator and animate health to gui
        auto healthLootIndicatorSceneObject = mActiveScene->FindSceneObject(HEALTH_LOOT_INDICATOR_SCENE_OBJECT_NAME);
        healthLootIndicatorSceneObject->mInvisible = false;
        std::get<scene::TextSceneObjectData>(healthLootIndicatorSceneObject->mSceneObjectTypeData).mText = "+" + std::to_string(eligibleHealthPointsAdded) + " health";
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(healthLootIndicatorSceneObject, 1.0f, 0.5f), [=](){});
        
        events::EventSystem::GetInstance().DispatchEvent<events::HealthRefillRewardEvent>(eligibleHealthPointsAdded, mPlayerBoardCardSceneObjectWrappers[game_constants::REMOTE_PLAYER_INDEX][0]->mSceneObject->mPosition, true);
    }

    // Set next scenes accordingly
    DataRepository::GetInstance().SetCurrentStoryMapNodeSeed(math::GetControlSeed());
    
    auto isTutorialMiniBoss = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::TUTORIAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::TUTORIAL_MAP_BOSS_COORD;
    auto isStoryFinalBoss = DataRepository::GetInstance().GetCurrentStoryMapType() == StoryMapType::NORMAL_MAP && DataRepository::GetInstance().GetCurrentStoryMapNodeCoord() == game_constants::STORY_MAP_BOSS_COORD;
    
    if (isStoryFinalBoss)
    {
        DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::STORY_VICTORY);
    }
    else
    {
        DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::CARD_SELECTION);
    }
    
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
        
        DataRepository::GetInstance().SetCurrentBattleSubSceneType(BattleSubSceneType::WHEEL);
    }
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnFlawlessVictoryTriggered(const events::FlawlessVictoryTriggerEvent&)
{
    CoreSystemsEngine::GetInstance().GetSoundManager().PreloadSfx(FIREWORKS_SFX);
    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(FIREWORKS_SFX);
    
    auto flawlessVictoryIndicatorSceneObject = mActiveScene->FindSceneObject(FLAWLESS_VICTORY_INDICATOR_SCENE_OBJECT_NAME);
    flawlessVictoryIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = -1.0f;
    flawlessVictoryIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = 1.0f;
    flawlessVictoryIndicatorSceneObject->mInvisible = false;
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(flawlessVictoryIndicatorSceneObject, 1.0f, 0.5f), [=](){});
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::OnHistoryButtonPressed()
{
    auto battleScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(battleScene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    
    battleScene->RemoveAllParticleEffects();
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(HISTORY_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

void BattleSceneLogicManager::FakeSettingsButtonPressed()
{
    auto battleScene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::BATTLE_SCENE);
    
    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(battleScene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
    
    battleScene->RemoveAllParticleEffects();
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::SETTINGS_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
}

///------------------------------------------------------------------------------------------------

glm::vec3 BattleSceneLogicManager::CalculateBoardEffectPosition(const size_t effectIndex, const size_t effectsCount, bool forRemotePlayer)
{
    float cardBlockWidth = game_constants::IN_GAME_CARD_ON_BOARD_WIDTH * effectsCount;
    float cardStartX = -cardBlockWidth/2.0f;
    
    auto targetX = cardStartX + effectIndex * game_constants::IN_GAME_CARD_ON_BOARD_WIDTH + game_constants::IN_GAME_CARD_ON_BOARD_WIDTH/2;
    if (effectsCount > game_constants::IN_GAME_CARD_PUSH_THRESHOLD)
    {
        float pushX = (effectsCount - game_constants::IN_GAME_CARD_PUSH_THRESHOLD) * game_constants::IN_GAME_CARD_PUSH_VALUE * (math::Abs(static_cast<int>(effectIndex - effectsCount/2)));
        bool oddCardCount = effectsCount % 2 != 0;
        if ((oddCardCount && effectIndex != effectsCount/2) || !oddCardCount)
        {
            targetX += (effectIndex < effectsCount/2) ? pushX : -pushX;
        }
    }
    
    return glm::vec3(targetX, forRemotePlayer ? BOARD_SIDE_EFFECT_TOP_POSITION.y : BOARD_SIDE_EFFECT_BOT_POSITION.y, INDIVIDUAL_CARD_BOARD_EFFECT_BASE_Z + effectIndex * INDIVIDUAL_CARD_BOARD_EFFECT_Z_INCREMENT);
}

///------------------------------------------------------------------------------------------------
