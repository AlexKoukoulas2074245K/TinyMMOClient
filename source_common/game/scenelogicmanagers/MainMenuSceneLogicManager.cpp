///------------------------------------------------------------------------------------------------
///  MainMenuSceneLogicManager.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 12/12/2023
///------------------------------------------------------------------------------------------------

#include <engine/CoreSystemsEngine.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/rendering/AnimationManager.h>
#include <engine/resloading/ShaderResource.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/Date.h>
#include <engine/utils/Logging.h>
#include <engine/scene/SceneManager.h>
#include <engine/sound/SoundManager.h>
#include <fstream>
#include <game/AchievementManager.h>
#include <game/AnimatedButton.h>
#include <game/Cards.h>
#include <game/GameSymbolicGlyphNames.h>
#include <game/scenelogicmanagers/MainMenuSceneLogicManager.h>
#include <game/DataRepository.h>
#include <game/IAPProductIds.h>
#include <game/ProductRepository.h>
#include <game/TutorialManager.h>
#include <game/utils/GiftingUtils.h>
#include <SDL_events.h>
#if defined(MACOS) || defined(MOBILE_FLOW)
#include <platform_utilities/AppleUtils.h>
#include <platform_utilities/CloudKitUtils.h>
#elif defined(WINDOWS)
#include <platform_utilities/WindowsUtils.h>
#endif

///------------------------------------------------------------------------------------------------

static constexpr int AVAILABLE_STORY_DECKS_COUNT = 3;
static constexpr int MIN_DECK_ENTRIES_TO_SCROLL = 4;

static const std::string MUTATION_CHANGES_TEXT_SCENE_OBJECT_NAME_PREFIX = "mutation_changes_text_";
static const std::string SELECTABLE_BUTTON_SHADER_FILE_NAME = "basic_custom_color.vs";
static const std::string DECK_ENTRY_SHADER = "card_family_selection_swipe_entry.vs";
static const std::string DECK_ENTRY_MASK_TEXTURE_FILE_NAME = "trap_mask.png";
static const std::string BRAZIER_TEXTURE_FILE_NAME = "brazier.png";
static const std::string MUTATION_FIRE_SHADER_FILE_NAME = "fire.vs";
static const std::string MUTATION_FIRE_TEXTURE_FILE_NAME = "fire.png";
static const std::string MUTATION_TEXTURE_FILE_NAME = "virus.png";
static const std::string PLUS_BUTTON_TEXTURE_FILE_NAME = "plus_button.png";
static const std::string MINUS_BUTTON_TEXTURE_FILE_NAME = "minus_button.png";
static const std::string MUTATION_MESH_FILE_NAME = "virus.obj";
static const std::string MUTATION_SHADER_FILE_NAME = "virus.vs";
static const std::string MAIN_MENU_THEME_MUSIC = "main_menu_theme";
static const std::string LOCK_ICON_TEXTURE_FILE_NAME = "lock.png";
static const std::string NEW_CARD_INDICATOR_SHADER_FILE_NAME = "new_indicator.vs";
static const std::string STORY_DECK_NAMES[AVAILABLE_STORY_DECKS_COUNT] =
{
    "Dinosaurs",
    "Gnawers",
    "Insects"
};

static const strutils::StringId FIRE_ALPHA_ANIMATION_NAME = strutils::StringId("fire_alpha_animation");
static const strutils::StringId FIRE_RED_COLOR_ANIMATION_NAME = strutils::StringId("fire_red_color_animation");
static const strutils::StringId FIRE_GREEN_COLOR_ANIMATION_NAME = strutils::StringId("fire_green_color_animation");
static const strutils::StringId MUTATION_PULSE_ANIMATION_NAME = strutils::StringId("mutation_pulse_animation");
static const strutils::StringId BRAZIER_SCENE_OBJECT_NAME = strutils::StringId("brazier");
static const strutils::StringId MUTATION_FIRE_SCENE_OBJECT_NAME = strutils::StringId("mutation_fire");
static const strutils::StringId MUTATION_SCENE_OBJECT_NAME = strutils::StringId("mutation");
static const strutils::StringId PRIVACY_POLICY_SCENE = strutils::StringId("privacy_policy_scene");
static const strutils::StringId STATS_SCENE = strutils::StringId("stats_scene");
static const strutils::StringId CREDITS_SCENE = strutils::StringId("credits_scene");
static const strutils::StringId RELEASE_NOTES_SCENE = strutils::StringId("release_notes_scene");
static const strutils::StringId GIFT_CODE_CLAIM_SCENE = strutils::StringId("gift_code_claim_scene");
static const strutils::StringId BOARD_SCENE_OBJECT_NAME = strutils::StringId("board");
static const strutils::StringId GAME_VERSION_SCENE_OBJECT_NAME = strutils::StringId("version");
static const strutils::StringId STORY_MODE_BUTTON_NAME = strutils::StringId("story_mode_button");
static const strutils::StringId CARD_LIBRARY_BUTTON_NAME = strutils::StringId("card_library_button");
static const strutils::StringId SHOP_BUTTON_NAME = strutils::StringId("shop_button");
static const strutils::StringId CONTINUE_STORY_BUTTON_NAME = strutils::StringId("continue_story_button");
static const strutils::StringId NEW_STORY_BUTTON_NAME = strutils::StringId("new_story_button");
static const strutils::StringId EXTRAS_BUTTON_NAME = strutils::StringId("extras_button");
static const strutils::StringId OPTIONS_BUTTON_NAME = strutils::StringId("options_button");
static const strutils::StringId QUIT_BUTTON_NAME = strutils::StringId("quit_button");
static const strutils::StringId NORMAL_BATTLE_MODE_BUTTON_NAME = strutils::StringId("normal_battle_mode_button");
static const strutils::StringId AI_DEMO_BATTLE_MODE_BUTTON_NAME = strutils::StringId("ai_demo_battle_mode_button");
static const strutils::StringId REPLAY_BATTLE_MODE_BUTTON_NAME = strutils::StringId("replay_battle_mode_button");
static const strutils::StringId STATS_BUTTON_NAME = strutils::StringId("stats_button");
static const strutils::StringId ACHIEVEMENTS_BUTTON_NAME = strutils::StringId("achievements_button");
static const strutils::StringId ENTER_GIFT_CODE_BUTTON_NAME = strutils::StringId("enter_gift_code_button");
static const strutils::StringId PRIVACY_POLICY_BUTTON_NAME = strutils::StringId("privacy_policy_button");
static const strutils::StringId RELEASE_NOTES_BUTTON_NAME = strutils::StringId("release_notes_button");
static const strutils::StringId CREDITS_BUTTON_NAME = strutils::StringId("credits_button");
static const strutils::StringId BACK_BUTTON_NAME = strutils::StringId("back_button");
static const strutils::StringId MUTATION_PLUS_BUTTON_NAME = strutils::StringId("mutation_plus");
static const strutils::StringId MUTATION_MINUS_BUTTON_NAME = strutils::StringId("mutation_minus");
static const strutils::StringId TITLE_SCENE_OBJECT_NAME = strutils::StringId("predators_title");
static const strutils::StringId TOP_DECK_TEXT_SCENE_OBJECT_NAME = strutils::StringId("top_deck_text");
static const strutils::StringId BOT_DECK_TEXT_SCENE_OBJECT_NAME = strutils::StringId("bot_deck_text");
static const strutils::StringId LOCK_LIBRARY_SCENE_OBJECT_NAME = strutils::StringId("library_lock");
static const strutils::StringId LOCK_SHOP_SCENE_OBJECT_NAME = strutils::StringId("shop_lock");
static const strutils::StringId UNLOCKED_LIBRARY_SCENE_OBJECT_NAME = strutils::StringId("library_unlocked");
static const strutils::StringId UNLOCKED_SHOP_SCENE_OBJECT_NAME = strutils::StringId("shop_unlocked");
static const strutils::StringId STORY_DECK_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("story_deck_container");
static const strutils::StringId TOP_DECK_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("top_deck_container");
static const strutils::StringId BOT_DECK_CONTAINER_SCENE_OBJECT_NAME = strutils::StringId("bot_deck_container");
static const strutils::StringId NEW_STORY_CONFIRMATION_BUTTON_NAME = strutils::StringId("new_story_confirmation");
static const strutils::StringId NEW_STORY_CANCELLATION_BUTTON_NAME = strutils::StringId("new_story_cancellation");
static const strutils::StringId NEW_STORY_CONFIRMATION_TEXT_TOP_NAME = strutils::StringId("new_story_confirmation_text_top");
static const strutils::StringId NEW_STORY_CONFIRMATION_TEXT_MIDDLE_NAME = strutils::StringId("new_story_confirmation_text_middle");
static const strutils::StringId NEW_STORY_CONFIRMATION_TEXT_BOT_NAME = strutils::StringId("new_story_confirmation_text_bot");
static const strutils::StringId STORY_DECK_SELECTION_PROMPT_SCENE_OBJECT_NAME = strutils::StringId("story_deck_selection_prompt");
static const strutils::StringId MUTATION_SELECTION_PROMPT_SCENE_OBJECT_NAME = strutils::StringId("mutation_selection_prompt");
static const strutils::StringId MUTATION_VALUE_SCENE_OBJECT_NAME = strutils::StringId("mutation_value");
static const strutils::StringId START_NEW_STORY_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("start_new_story_button");
static const strutils::StringId STORY_HEALTH_REFILL_PRODUCT_NAME = strutils::StringId("story_health_refill");
static const strutils::StringId NORMAL_PACK_PRODUCT_NAME = strutils::StringId("normal_card_pack");
static const strutils::StringId GOLDEN_PACK_PRODUCT_NAME = strutils::StringId("golden_card_pack");
static const strutils::StringId COINS_S_PRODUCT_NAME = strutils::StringId("coins_ss");
static const strutils::StringId COINS_M_PRODUCT_NAME = strutils::StringId("coins_mm");
static const strutils::StringId COINS_L_PRODUCT_NAME = strutils::StringId("coins_ll");
static const strutils::StringId POINT_LIGHT_POSITION_UNIFORM_NAME = strutils::StringId("point_light_position");
static const strutils::StringId DIFFUSE_COLOR_UNIFORM_NAME = strutils::StringId("mat_diffuse");
static const strutils::StringId AMBIENT_COLOR_UNIFORM_NAME = strutils::StringId("mat_ambient");
static const strutils::StringId SPEC_COLOR_UNIFORM_NAME = strutils::StringId("mat_spec");
static const strutils::StringId POINT_LIGHT_POWER_UNIFORM_NAME = strutils::StringId("point_light_power");
static const strutils::StringId AFFECTED_BY_LIGHT_UNIFORM_NAME = strutils::StringId("affected_by_light");
static const strutils::StringId COLOR_FACTOR_R_UNIFORM_NAME = strutils::StringId("color_factor_r");
static const strutils::StringId COLOR_FACTOR_G_UNIFORM_NAME = strutils::StringId("color_factor_g");
static const strutils::StringId COLOR_FACTOR_B_UNIFORM_NAME = strutils::StringId("color_factor_b");
static const strutils::StringId STORY_DECK_SCENE_OBJECT_NAMES[AVAILABLE_STORY_DECKS_COUNT] =
{
    strutils::StringId("selected_deck_dinosaurs"),
    strutils::StringId("selected_deck_rodents"),
    strutils::StringId("selected_deck_insects")
};

static const glm::vec2 STORY_DECK_ENTRY_CUTOFF_VALUES = {-0.25f, 0.15f};
static const glm::vec2 STORY_DECK_SELECTION_CONTAINER_CUTOFF_VALUES = {-0.1f, 0.1f};

static const glm::vec3 BUTTON_SCALE = {0.0005f, 0.0005f, 0.0005f};
static const glm::vec3 NEW_INDICATOR_SCALE = {0.00035f, 0.00035f, 0.00035f};
static const glm::vec3 LOCK_SCALE = {0.05f, 0.05f, 0.05f};
static const glm::vec3 LOCK_POSITION_OFFSET = {-0.04f, -0.003f, 0.0f};
static const glm::vec3 UNLOCKED_TEXT_POSITION_OFFSET = {-0.09f, -0.003f, 0.0f};
static const glm::vec3 LOCKED_BUTTON_COLOR = {0.5f, 0.5f, 0.5f};
static const glm::vec3 MUTATION_CHANGE_TEXT_SCALE = {0.0002f, 0.0002f, 0.0002f};
static const glm::vec3 MUTATION_CHANGE_TEXT_INIT_POSITION = {0.05, 0.088f, 1.0f};
static const glm::vec3 PLUS_BUTTON_SCALE = {0.075f, 0.075f, 0.075f};
static const glm::vec3 MINUS_BUTTON_SCALE = {0.075f, 0.075f, 0.075f};
static const glm::vec3 STORY_DECK_NAME_SCALES = {0.000325f, 0.000325f, 0.000325f};
static const glm::vec3 CONTINUE_STORY_BUTTON_POSITION = {-0.142f, 0.09f, 0.1f};
static const glm::vec3 NO_PROGRESS_NEW_STORY_BUTTON_POSITION = {-0.091f, 0.06f, 0.1f};
static const glm::vec3 NEW_STORY_BUTTON_POSITION = {-0.091f, 0.00f, 0.1f};
static const glm::vec3 STORY_MODE_BUTTON_POSITION = {0.0f, 0.11f, 0.1f};
static const glm::vec3 CARD_LIBRARY_BUTTON_POSITION = {0.0f, 0.05f, 0.1f};
static const glm::vec3 SHOP_BUTTON_POSITION = {0.0f, -0.01f, 0.1f};
static const glm::vec3 EXTRAS_BUTTON_POSITION = {0.0f, -0.06f, 0.1f};
static const glm::vec3 OPTIONS_BUTTON_POSITION = {0.0f, -0.120f, 0.1f};
static const glm::vec3 MUTATION_PLUS_BUTTON_POSITION = {-0.198f, -0.083f, 0.1f};
static const glm::vec3 MUTATION_MINUS_BUTTON_POSITION = {-0.106f, -0.083f, 0.1f};
static const glm::vec3 QUIT_BUTTON_POSITION = {0.0f, -0.180f, 0.1f};
static const glm::vec3 STATS_BUTTON_POSITION = {-0.076f, 0.105f, 0.1f};
static const glm::vec3 ACHIEVEMENTS_BUTTON_POSITION = {-0.125f, 0.045f, 0.1f};
//static const glm::vec3 ENTER_GIFT_CODE_BUTTON_POSITION = {-0.135f, 0.045f, 0.1f};
static const glm::vec3 PRIVACY_POLICY_BUTTON_POSITION = {-0.125f, -0.015f, 0.1f};
static const glm::vec3 RELEASE_NOTES_BUTTON_POSITION = {-0.125f, -0.075f, 0.1f};
static const glm::vec3 CREDITS_BUTTON_POSITION = {-0.052f, -0.135f, 0.1f};
static const glm::vec3 POINT_LIGHT_POSITION = { -1.0f, 0.0f, -1.0f };
static const glm::vec3 DIFFUSE_COLOR = { 1.0f, 1.0f, 1.0f };
static const glm::vec3 SPEC_COLOR = { 1.0f, 1.0f, 1.0f };
static const glm::vec3 AMB_COLOR = { 1.0f, 0.0f, 0.0f };
static const glm::vec3 BRAZIER_POSITION = { -0.149f, -0.030f, 1.5f };
static const glm::vec3 BRAZIER_SCALE = { 0.1f, 0.057f, 0.1f };
static const glm::vec3 MUTATION_POSITION = { -0.04f, -0.0f, 1.0f };
static const glm::vec3 MUTATION_SCALE = { 0.05f, 0.05f, 0.05f };
static const glm::vec3 MUTATION_FIRE_POSITION = { -0.145f, 0.044f, 1.0f };
static const glm::vec3 MUTATION_VALUE_POSITION = { -0.133f, 0.02f, 2.0f };
static const glm::vec3 MUTATION_FIRE_SCALE = { 0.104f, 0.127f, 0.05f };
static const glm::vec3 BACK_BUTTON_POSITION = {0.148f, -0.148f, 0.1f};
static const glm::vec3 SELECT_MUTATION_BACK_BUTTON_POSITION = {0.148f, -0.18f, 23.1f};
static const glm::vec3 DESELECTED_BUTTON_COLOR = { 1.0f, 1.0f, 1.0f};
static const glm::vec3 SELECTED_BUTTON_COLOR = {0.0f, 0.66f, 0.66f};
static const glm::vec3 NEW_STORY_CONFIRMATION_BUTTON_POSITION = {-0.132f, -0.103f, 23.1f};
static const glm::vec3 NEW_STORY_CANCELLATION_BUTTON_POSITION = {0.036f, -0.103f, 23.1f};
static const glm::vec3 NEW_STORY_CONFIRMATION_TEXT_TOP_POSITION = {-0.267f, 0.09f, 23.1f};
static const glm::vec3 NEW_STORY_CONFIRMATION_TEXT_MIDDLE_POSITION = {-0.282f, 0.039f, 23.1f};
static const glm::vec3 NEW_STORY_CONFIRMATION_TEXT_BOT_POSITION = {-0.205f, -0.012f, 23.1f};
static const glm::vec3 NEW_STORY_DECK_SELECTION_TEXT_POSITION = {-0.169f, 0.115f, 0.1f};
static const glm::vec3 MUTATION_SELECTION_TEXT_POSITION = {-0.179f, 0.135f, 0.1f};
static const glm::vec3 START_NEW_STORY_BUTTON_POSITION = {-0.049f, -0.145f, 23.1f};
static const glm::vec3 SELECT_DECK_BUTTON_POSITION = {-0.065f, -0.145f, 23.1f};
static const glm::vec3 SELECT_MUTATION_START_BUTTON_POSITION = {-0.055f, -0.18f, 23.1f};
static const glm::vec3 STORY_DECK_NAME_POSITIONS[AVAILABLE_STORY_DECKS_COUNT] =
{
    { -0.202f, 0.054f, 0.1f},
    { -0.072f, 0.054f, 0.1f},
    { 0.054f, 0.054f, 0.1f}
};

static const float SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS = 0.25f;
static const float DECK_SWIPEABLE_ENTRY_SCALE = 0.075f;
static const float STORY_DECK_SELECTION_ENTRY_SCALE = 0.115f;
static const float DECK_ENTRY_ALPHA = 0.5f;
static const float DECK_ENTRY_Z = 0.1f;
static const float STAGGERED_ITEM_ALPHA_DELAY_SECS = 0.1f;
static const float DECK_SELECTED_MAX_SCALE_FACTOR = 1.15f;
static const float DECK_SELECTED_MIN_SCALE_FACTOR = 0.65f;
static const float DECK_SELECTION_ANIMATION_DURATION_SECS = 0.4f;
static const float MUTATION_ROTATION_SPEED = 1.0f/1000.0f;
static const float POINT_LIGHT_POWER = 8.0f;
static const float FIRE_COLOR_R_INCREMENTS = 0.1f;
static const float FIRE_COLOR_G_INCREMENTS = 0.1f;

static const math::Rectangle STORY_DECK_SELECTION_CONTAINER_TOP_BOUNDS = {{-0.25f, -0.08f}, {0.2f, 0.01f}};

static const std::vector<strutils::StringId> APPLICABLE_SCENE_NAMES =
{
    game_constants::MAIN_MENU_SCENE
};

static const std::unordered_set<strutils::StringId, strutils::StringIdHasher> STATIC_SCENE_ELEMENTS =
{
    TITLE_SCENE_OBJECT_NAME,
    BOARD_SCENE_OBJECT_NAME,
    GAME_VERSION_SCENE_OBJECT_NAME,
    MUTATION_SCENE_OBJECT_NAME
};

static const std::unordered_map<strutils::StringId, BattleControlType, strutils::StringIdHasher> BATTLE_MODE_BUTTON_NAMES_TO_BATTLE_CONTROL_TYPE =
{
    { NORMAL_BATTLE_MODE_BUTTON_NAME, BattleControlType::AI_TOP_ONLY },
    { AI_DEMO_BATTLE_MODE_BUTTON_NAME, BattleControlType::AI_TOP_BOT },
    { REPLAY_BATTLE_MODE_BUTTON_NAME, BattleControlType::REPLAY }
};

static const std::unordered_map<StoryMapSceneType, strutils::StringId> STORY_MAP_SCENE_TYPE_TO_SCENE_NAME =
{
    { StoryMapSceneType::STORY_MAP, game_constants::STORY_MAP_SCENE },
    { StoryMapSceneType::EVENT, game_constants::EVENT_SCENE },
    { StoryMapSceneType::BATTLE, game_constants::BATTLE_SCENE },
    { StoryMapSceneType::SHOP, game_constants::SHOP_SCENE }
};

///------------------------------------------------------------------------------------------------

static bool sEmptyProgression = false;
void CheckForEmptyProgression()
{
    serial::BaseDataFileDeserializer persistentDataFileChecker("persistent", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::WarnOnFileNotFoundBehavior::DO_NOT_WARN, serial::CheckSumValidationBehavior::VALIDATE_CHECKSUM);
    sEmptyProgression = persistentDataFileChecker.GetState().empty();
}

///------------------------------------------------------------------------------------------------

#if defined(MACOS) || defined(MOBILE_FLOW)
void OnCloudQueryCompleted(cloudkit_utils::QueryResultData resultData)
{
    if (!resultData.mSuccessfullyQueriedAtLeastOneFileField)
    {
        return;
    }
    
    auto writeDataStringToTempFile = [](const std::string& tempFileNameWithoutExtension, const std::string& data)
    {
        if (!data.empty())
        {
            std::string dataFileExtension = ".json";
            
            auto filePath = apple_utils::GetPersistentDataDirectoryPath() + tempFileNameWithoutExtension + dataFileExtension;
            std::remove(filePath.c_str());
            
            std::ofstream file(filePath);
            if (file.is_open())
            {
                file << data;
                file.close();
            }
        }
    };
    
    auto localDeviceId = apple_utils::GetDeviceId();
    
    auto checkForDeviceIdInconsistency = [=](const std::string& targetDataFileNameWithoutExtension, const serial::BaseDataFileDeserializer& dataFileDeserializer)
    {
        if
        (
            targetDataFileNameWithoutExtension == "persistent" &&
            dataFileDeserializer.GetState().count("device_id") &&
            dataFileDeserializer.GetState().count("device_name") &&
            dataFileDeserializer.GetState().count("timestamp")
        )
        {
            auto deviceId = dataFileDeserializer.GetState().at("device_id").get<std::string>();
            
            using namespace date;
            std::stringstream s;
            s << std::chrono::system_clock::time_point(std::chrono::seconds(dataFileDeserializer.GetState().at("timestamp").get<long>()));
            
            const auto& localSuccessfulTransactions = DataRepository::GetInstance().GetSuccessfulTransactionIds();
            std::vector<std::string> cloudSuccessfulTransactions;
            if (dataFileDeserializer.GetState().count("successful_transaction_ids"))
            {
                cloudSuccessfulTransactions = dataFileDeserializer.GetState()["successful_transaction_ids"].get<std::vector<std::string>>();
            }
            
            // If local progression is ahead in terms of transactions we decline the cloud data
            if (localSuccessfulTransactions.size() > cloudSuccessfulTransactions.size())
            {
                DataRepository::GetInstance().SetForeignProgressionDataFound(ForeignCloudDataFoundType::NONE);
            }
            // If cloud progression is ahead in terms of transactions we mandate the cloud data
            else if (localSuccessfulTransactions.size() < cloudSuccessfulTransactions.size())
            {
                DataRepository::GetInstance().SetForeignProgressionDataFound(ForeignCloudDataFoundType::MANDATORY);
            }
            // Otherwise if the transaction vectors are the same let the user choose
            else
            {
                if (deviceId != localDeviceId)
                {
                    DataRepository::GetInstance().SetForeignProgressionDataFound(ForeignCloudDataFoundType::OPTIONAL);
                }
                else
                {
                    DataRepository::GetInstance().SetForeignProgressionDataFound(ForeignCloudDataFoundType::NONE);
                }
            }
            
            DataRepository::GetInstance().SetCloudDataDeviceNameAndTime("(From " + dataFileDeserializer.GetState().at("device_name").get<std::string>() + " at " + strutils::StringSplit(s.str(), '.')[0] + ")");
            
        }
    };
    
    writeDataStringToTempFile("cloud_persistent", resultData.mPersistentProgressRawString);
    writeDataStringToTempFile("cloud_story", resultData.mStoryProgressRawString);
    writeDataStringToTempFile("cloud_last_battle", resultData.mLastBattleRawString);
    
    checkForDeviceIdInconsistency("persistent", serial::BaseDataFileDeserializer("cloud_persistent", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::WarnOnFileNotFoundBehavior::DO_NOT_WARN, serial::CheckSumValidationBehavior::VALIDATE_CHECKSUM));
    checkForDeviceIdInconsistency("story", serial::BaseDataFileDeserializer("cloud_story", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::WarnOnFileNotFoundBehavior::DO_NOT_WARN, serial::CheckSumValidationBehavior::VALIDATE_CHECKSUM));
    checkForDeviceIdInconsistency("last_battle", serial::BaseDataFileDeserializer("cloud_last_battle", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::WarnOnFileNotFoundBehavior::DO_NOT_WARN, serial::CheckSumValidationBehavior::VALIDATE_CHECKSUM));
}
#endif

///------------------------------------------------------------------------------------------------

const std::vector<strutils::StringId>& MainMenuSceneLogicManager::VGetApplicableSceneNames() const
{
    return APPLICABLE_SCENE_NAMES;
}

///------------------------------------------------------------------------------------------------

MainMenuSceneLogicManager::MainMenuSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

MainMenuSceneLogicManager::~MainMenuSceneLogicManager(){}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::VInitSceneCamera(std::shared_ptr<scene::Scene>)
{
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::VInitScene(std::shared_ptr<scene::Scene> scene)
{
    CheckForEmptyProgression();
#if defined(MACOS) || defined(MOBILE_FLOW)
    cloudkit_utils::QueryPlayerProgress([=](cloudkit_utils::QueryResultData resultData){ OnCloudQueryCompleted(resultData); });
    apple_utils::LoadStoreProducts({ iap_product_ids::STORY_HEALTH_REFILL, iap_product_ids::COINS_S, iap_product_ids::COINS_M, iap_product_ids::COINS_L });
#endif
    CoreSystemsEngine::GetInstance().GetSoundManager().PlaySound(MAIN_MENU_THEME_MUSIC);
    
    auto& eventSystem = events::EventSystem::GetInstance();
    eventSystem.RegisterForEvent<events::WindowResizeEvent>(this, &MainMenuSceneLogicManager::OnWindowResize);
    
    DataRepository::GetInstance().SetQuickPlayData(nullptr);
    DataRepository::GetInstance().SetIsCurrentlyPlayingStoryMode(false);
    
    mQuickPlayData = std::make_unique<QuickPlayData>();
    mQuickPlayData->mMutationLevel = math::Min(game_constants::MAX_MUTATION_LEVEL, DataRepository::GetInstance().GetMaxMutationLevelWithAtLeastOneVictory() + 1);
    
    CardDataRepository::GetInstance().LoadCardData(true);
    mPreviousSubSceneStack = std::stack<SubSceneType>();
    mActiveSubScene = SubSceneType::NONE;
    mTransitioningToSubScene = false;
    mNeedToSetBoardPositionAndZoomFactor = true;
    mShouldPushToPreviousSceneStack = true;
    CreateMutationObject(scene);
    InitSubScene(SubSceneType::MAIN, scene);
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::VUpdate(const float dtMillis, std::shared_ptr<scene::Scene> scene)
{
    static float time = 0.0f;
    time += dtMillis/1000.0f;
    
    auto mutationFireSceneObject = scene->FindSceneObject(MUTATION_FIRE_SCENE_OBJECT_NAME);
    if (mutationFireSceneObject)
    {
        mutationFireSceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    }
    
    auto versionSceneObject = scene->FindSceneObject(GAME_VERSION_SCENE_OBJECT_NAME);
    if (versionSceneObject)
    {
#if defined(MACOS) || defined(MOBILE_FLOW)
        std::get<scene::TextSceneObjectData>(versionSceneObject->mSceneObjectTypeData).mText = "Game Version " + apple_utils::GetAppVersion();
#endif
    }
    
    auto mutationSceneObject = scene->FindSceneObject(MUTATION_SCENE_OBJECT_NAME);
    if (mutationSceneObject)
    {
        mutationSceneObject->mRotation.y += dtMillis * MUTATION_ROTATION_SPEED;
    }
    
    if (mTransitioningToSubScene || DataRepository::GetInstance().GetForeignProgressionDataFound() != ForeignCloudDataFoundType::NONE)
    {
        return;
    }
    
    if (mNeedToSetBoardPositionAndZoomFactor)
    {
        auto boardSceneObject = scene->FindSceneObject(BOARD_SCENE_OBJECT_NAME);
        
        boardSceneObject->mPosition = game_constants::GAME_BOARD_INIT_POSITION;
        boardSceneObject->mRotation = game_constants::GAME_BOARD_INIT_ROTATION;
        
        mNeedToSetBoardPositionAndZoomFactor = false;
    }
    
    // Animated buttons
    for (auto& animatedButton: mAnimatedButtons)
    {
        animatedButton->Update(dtMillis);
    }
    
    if (mCardFamilyContainerTop)
    {
        auto containerUpdateResult = mCardFamilyContainerTop->Update(dtMillis);
        if (containerUpdateResult.mInteractedElementIndex != -1 && mQuickPlayData->mBattleControlType != BattleControlType::REPLAY)
        {
            DeckSelected(containerUpdateResult.mInteractedElementIndex, true, scene);
        }
    }
    
    if (mCardFamilyContainerBot)
    {
        auto containerUpdateResult = mCardFamilyContainerBot->Update(dtMillis);
        if (containerUpdateResult.mInteractedElementIndex != -1 && (mQuickPlayData->mBattleControlType != BattleControlType::REPLAY || mActiveSubScene == SubSceneType::NEW_STORY_DECK_SELECTION))
        {
            DeckSelected(containerUpdateResult.mInteractedElementIndex, false, scene);
        }
    }
    
    auto unlockedCardLibrarySceneObject = scene->FindSceneObject(UNLOCKED_LIBRARY_SCENE_OBJECT_NAME);
    if (unlockedCardLibrarySceneObject)
    {
        unlockedCardLibrarySceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    }
    
    auto unlockedShopLibrarySceneObject = scene->FindSceneObject(UNLOCKED_SHOP_SCENE_OBJECT_NAME);
    if (unlockedShopLibrarySceneObject)
    {
        unlockedShopLibrarySceneObject->mShaderFloatUniformValues[game_constants::TIME_UNIFORM_NAME] = time;
    }
    
    CheckForCardCompletion();
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::VDestroyScene(std::shared_ptr<scene::Scene>)
{
    CoreSystemsEngine::GetInstance().GetResourceLoadingService().UnloadAllDynamicallyCreatedTextures();
    events::EventSystem::GetInstance().UnregisterAllEventsForListener(this);
}

///------------------------------------------------------------------------------------------------

std::shared_ptr<GuiObjectManager> MainMenuSceneLogicManager::VGetGuiObjectManager()
{
    return nullptr;
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::InitSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    if (mActiveSubScene == subSceneType)
    {
        return;
    }
    
    DataRepository::GetInstance().SetCanSurfaceCloudDataScene(subSceneType == SubSceneType::MAIN || subSceneType == SubSceneType::NONE);
    
    if (!mShouldPushToPreviousSceneStack)
    {
        mShouldPushToPreviousSceneStack = true;
    }
    else
    {
        mPreviousSubSceneStack.push(mActiveSubScene);
    }
    
    mActiveSubScene = subSceneType;
    
    scene->RemoveAllSceneObjectsButTheOnesNamed(STATIC_SCENE_ELEMENTS);
    mAnimatedButtons.clear();
    mDeckSelectionSceneObjects.clear();
    mCardFamilyContainerTop = nullptr;
    mCardFamilyContainerBot = nullptr;
    
    switch (subSceneType)
    {
        case SubSceneType::MAIN:
        {
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                STORY_MODE_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Story Mode",
                STORY_MODE_BUTTON_NAME,
                [=](){ TransitionToSubScene(SubSceneType::STORY_MODE, scene); },
                *scene
            ));
            
            if (DataRepository::GetInstance().GetGamesFinishedCount() == 0)
            {
                auto lockSceneObject = scene->CreateSceneObject(LOCK_LIBRARY_SCENE_OBJECT_NAME);
                lockSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + LOCK_ICON_TEXTURE_FILE_NAME);
                lockSceneObject->mScale = LOCK_SCALE;
                lockSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            }
            else if (!DataRepository::GetInstance().HasSeenTutorial(tutorials::CARD_LIBRARY_TUTORIAL))
            {
                scene::TextSceneObjectData textNewIndicatorData;
                textNewIndicatorData.mText = "NEW";
                textNewIndicatorData.mFontName = game_constants::DEFAULT_FONT_NAME;
                
                auto newLibraryIndicatorSceneObject = scene->CreateSceneObject(UNLOCKED_LIBRARY_SCENE_OBJECT_NAME);
                newLibraryIndicatorSceneObject->mSceneObjectTypeData = std::move(textNewIndicatorData);
                newLibraryIndicatorSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + NEW_CARD_INDICATOR_SHADER_FILE_NAME);
                newLibraryIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = -1.0f;
                newLibraryIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = 1.0f;
                newLibraryIndicatorSceneObject->mScale = NEW_INDICATOR_SCALE;
            }
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                CARD_LIBRARY_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Card Library",
                CARD_LIBRARY_BUTTON_NAME,
                [=]()
                {
                    if (DataRepository::GetInstance().GetGamesFinishedCount() == 0)
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::FIRST_GAME_LOCK_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                    }
                    else
                    {
                        DataRepository::GetInstance().SetCurrentCardLibraryBehaviorType(CardLibraryBehaviorType::CARD_LIBRARY);
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::CARD_LIBRARY_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                        
                        auto unlockedLibrarySceneObject = scene->FindSceneObject(UNLOCKED_LIBRARY_SCENE_OBJECT_NAME);
                        if (unlockedLibrarySceneObject)
                        {
                            CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(unlockedLibrarySceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [](){});
                        }
                    }
                },
                *scene
            ));
            
            if (DataRepository::GetInstance().GetGamesFinishedCount() == 0)
            {
                auto lockSceneObject = scene->CreateSceneObject(LOCK_SHOP_SCENE_OBJECT_NAME);
                lockSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + LOCK_ICON_TEXTURE_FILE_NAME);
                lockSceneObject->mScale = LOCK_SCALE;
                lockSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            }
            else if (!DataRepository::GetInstance().HasSeenTutorial(tutorials::PERMA_SHOP_TUTORIAL))
            {
                scene::TextSceneObjectData textNewIndicatorData;
                textNewIndicatorData.mText = "NEW";
                textNewIndicatorData.mFontName = game_constants::DEFAULT_FONT_NAME;
                
                auto newShopIndicatorSceneObject = scene->CreateSceneObject(UNLOCKED_SHOP_SCENE_OBJECT_NAME);
                newShopIndicatorSceneObject->mSceneObjectTypeData = std::move(textNewIndicatorData);
                newShopIndicatorSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + NEW_CARD_INDICATOR_SHADER_FILE_NAME);
                newShopIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_Y_UNIFORM_NAME] = -1.0f;
                newShopIndicatorSceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_Y_UNIFORM_NAME] = 1.0f;
                newShopIndicatorSceneObject->mScale = NEW_INDICATOR_SCALE;
            }
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                SHOP_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Shop",
                SHOP_BUTTON_NAME,
                [=]()
                {
                    if (IsDisconnected())
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::DISCONNECTED_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                    }
                    else if (DataRepository::GetInstance().GetGamesFinishedCount() == 0)
                    {
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::FIRST_GAME_LOCK_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                    }
                    else
                    {
                        DataRepository::GetInstance().SetCurrentShopBehaviorType(ShopBehaviorType::PERMA_SHOP);
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::SHOP_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
                    }
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                EXTRAS_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Extras",
                EXTRAS_BUTTON_NAME,
                [=](){ TransitionToSubScene(SubSceneType::EXTRAS, scene); },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                OPTIONS_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Options",
                OPTIONS_BUTTON_NAME,
                [=]()
                {
                    CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenValueAnimation>(scene->GetUpdateTimeSpeedFactor(), 0.0f, game_constants::SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS), [](){}, game_constants::SCENE_SPEED_DILATION_ANIMATION_NAME);
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::SETTINGS_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                },
                *scene
            ));
            
        #if defined(MOBILE_FLOW)
            (void)QUIT_BUTTON_NAME;
            (void)QUIT_BUTTON_POSITION;
        #else
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                QUIT_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Quit",
                QUIT_BUTTON_NAME,
                []()
                {
                    SDL_Event e;
                    e.type = SDL_QUIT;
                    SDL_PushEvent(&e);
                },
                *scene
            ));
        #endif
            
            for (auto& animatedButton: mAnimatedButtons)
            {
                auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*animatedButton->GetSceneObject());
                auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
                animatedButton->GetSceneObject()->mPosition.x -= textLength/2.0f;
            }
            
            auto libraryLockSceneObject = scene->FindSceneObject(LOCK_LIBRARY_SCENE_OBJECT_NAME);
            if (libraryLockSceneObject)
            {
                auto cardLibrarySceneObject = scene->FindSceneObject(CARD_LIBRARY_BUTTON_NAME);
                cardLibrarySceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::BASIC_CUSTOM_COLOR_SHADER_FILE_NAME);
                cardLibrarySceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = LOCKED_BUTTON_COLOR;
                libraryLockSceneObject->mPosition = cardLibrarySceneObject->mPosition + LOCK_POSITION_OFFSET;
            }
            
            auto shopLockSceneObject = scene->FindSceneObject(LOCK_SHOP_SCENE_OBJECT_NAME);
            if (shopLockSceneObject)
            {
                auto shopSceneObject = scene->FindSceneObject(SHOP_BUTTON_NAME);
                shopSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + game_constants::BASIC_CUSTOM_COLOR_SHADER_FILE_NAME);
                shopSceneObject->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = LOCKED_BUTTON_COLOR;
                shopLockSceneObject->mPosition = shopSceneObject->mPosition + LOCK_POSITION_OFFSET;
            }
            
            auto unlockedLibrarySceneObject = scene->FindSceneObject(UNLOCKED_LIBRARY_SCENE_OBJECT_NAME);
            if (unlockedLibrarySceneObject)
            {
                auto cardLibrarySceneObject = scene->FindSceneObject(CARD_LIBRARY_BUTTON_NAME);
                unlockedLibrarySceneObject->mPosition = cardLibrarySceneObject->mPosition + UNLOCKED_TEXT_POSITION_OFFSET;
            }
            
            auto unlockedShopSceneObject = scene->FindSceneObject(UNLOCKED_SHOP_SCENE_OBJECT_NAME);
            if (unlockedShopSceneObject)
            {
                auto shopSceneObject = scene->FindSceneObject(SHOP_BUTTON_NAME);
                unlockedShopSceneObject->mPosition = shopSceneObject->mPosition + UNLOCKED_TEXT_POSITION_OFFSET;
            }
        } break;
           
        case SubSceneType::STORY_MODE:
        {
            bool progressExists = DataRepository::GetInstance().GetStoryMapGenerationSeed() != 0;
            if (progressExists)
            {
                mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
                (
                    CONTINUE_STORY_BUTTON_POSITION,
                    BUTTON_SCALE,
                    game_constants::DEFAULT_FONT_NAME,
                    "Continue Story",
                    CONTINUE_STORY_BUTTON_NAME,
                    [=](){
                        DataRepository::GetInstance().SetIsCurrentlyPlayingStoryMode(true);
                        DataRepository::GetInstance().SetCurrentShopBehaviorType(ShopBehaviorType::STORY_SHOP);
                        events::EventSystem::GetInstance().DispatchEvent<events::SendPlayMessageEvent>();
                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(STORY_MAP_SCENE_TYPE_TO_SCENE_NAME.at(DataRepository::GetInstance().GetCurrentStoryMapSceneType()), SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE); },
                    *scene
                ));
                
                mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
                (
                    NEW_STORY_BUTTON_POSITION,
                    BUTTON_SCALE,
                    game_constants::DEFAULT_FONT_NAME,
                    "New Story",
                    NEW_STORY_BUTTON_NAME,
                    [=](){ TransitionToSubScene(SubSceneType::NEW_STORY_CONFIRMATION, scene); },
                    *scene
                ));
            }
            else
            {
                mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
                (
                    NO_PROGRESS_NEW_STORY_BUTTON_POSITION,
                    BUTTON_SCALE,
                    game_constants::DEFAULT_FONT_NAME,
                    "New Story",
                    NEW_STORY_BUTTON_NAME,
                    [=]()
                    {
                        DataRepository::GetInstance().ResetStoryData();
                        DataRepository::GetInstance().FlushStateToFile();
                        TransitionToSubScene(SubSceneType::NEW_STORY_DECK_SELECTION, scene);
                    },
                    *scene
                ));
            }
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                BACK_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Back",
                BACK_BUTTON_NAME,
                [=](){ GoToPreviousSubScene(scene); },
                *scene
            ));
        } break;
            
        case SubSceneType::NEW_STORY_CONFIRMATION:
        {
            scene::TextSceneObjectData textDataNewStoryTop;
            textDataNewStoryTop.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataNewStoryTop.mText = "Are you sure you want to start";
            auto textNewStoryTopSceneObject = scene->CreateSceneObject(NEW_STORY_CONFIRMATION_TEXT_TOP_NAME);
            textNewStoryTopSceneObject->mSceneObjectTypeData = std::move(textDataNewStoryTop);
            textNewStoryTopSceneObject->mPosition = NEW_STORY_CONFIRMATION_TEXT_TOP_POSITION;
            textNewStoryTopSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataNewStoryMid;
            textDataNewStoryMid.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataNewStoryMid.mText = "a new story? Your active story";
            auto textNewStoryMidSceneObject = scene->CreateSceneObject(NEW_STORY_CONFIRMATION_TEXT_MIDDLE_NAME);
            textNewStoryMidSceneObject->mSceneObjectTypeData = std::move(textDataNewStoryMid);
            textNewStoryMidSceneObject->mPosition = NEW_STORY_CONFIRMATION_TEXT_MIDDLE_POSITION;
            textNewStoryMidSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textDataNewStoryBot;
            textDataNewStoryBot.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataNewStoryBot.mText = " progress will be lost.";
            auto textNewStoryBotSceneObject = scene->CreateSceneObject(NEW_STORY_CONFIRMATION_TEXT_BOT_NAME);
            textNewStoryBotSceneObject->mSceneObjectTypeData = std::move(textDataNewStoryBot);
            textNewStoryBotSceneObject->mPosition = NEW_STORY_CONFIRMATION_TEXT_BOT_POSITION;
            textNewStoryBotSceneObject->mScale = BUTTON_SCALE;
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                NEW_STORY_CONFIRMATION_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Yes",
                NEW_STORY_CONFIRMATION_BUTTON_NAME,
                [=]()
                {
                    DataRepository::GetInstance().ResetStoryData();
                    DataRepository::GetInstance().FlushStateToFile();
                    TransitionToSubScene(SubSceneType::NEW_STORY_DECK_SELECTION, scene);
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                NEW_STORY_CANCELLATION_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Cancel",
                NEW_STORY_CANCELLATION_BUTTON_NAME,
                [=]() { GoToPreviousSubScene(scene); },
                *scene
            ));
        } break;
        
        case SubSceneType::NEW_STORY_DECK_SELECTION:
        {
            scene::TextSceneObjectData textDataDeckSelectionPrompt;
            textDataDeckSelectionPrompt.mFontName = game_constants::DEFAULT_FONT_NAME;
            textDataDeckSelectionPrompt.mText = "Select Story Deck";
            auto deckSelectionTextSceneObject = scene->CreateSceneObject(STORY_DECK_SELECTION_PROMPT_SCENE_OBJECT_NAME);
            deckSelectionTextSceneObject->mSceneObjectTypeData = std::move(textDataDeckSelectionPrompt);
            deckSelectionTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            deckSelectionTextSceneObject->mPosition = NEW_STORY_DECK_SELECTION_TEXT_POSITION;
            deckSelectionTextSceneObject->mScale = BUTTON_SCALE;
            
            for (int i = 0; i < AVAILABLE_STORY_DECKS_COUNT; ++i)
            {
                scene::TextSceneObjectData textDataDeckSelectionPrompt;
                textDataDeckSelectionPrompt.mFontName = game_constants::DEFAULT_FONT_NAME;
                textDataDeckSelectionPrompt.mText = STORY_DECK_NAMES[i];
                auto deckSelectionTextSceneObject = scene->CreateSceneObject(STORY_DECK_SCENE_OBJECT_NAMES[i]);
                deckSelectionTextSceneObject->mSceneObjectTypeData = std::move(textDataDeckSelectionPrompt);
                deckSelectionTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
                deckSelectionTextSceneObject->mPosition = STORY_DECK_NAME_POSITIONS[i];
                deckSelectionTextSceneObject->mScale = STORY_DECK_NAME_SCALES;
            }
            
            mCardFamilyContainerBot = std::make_unique<SwipeableContainer<CardFamilyEntry>>
            (
                ContainerType::HORIZONTAL_LINE,
                glm::vec3(STORY_DECK_SELECTION_ENTRY_SCALE * 2),
                STORY_DECK_SELECTION_CONTAINER_TOP_BOUNDS,
                STORY_DECK_SELECTION_CONTAINER_CUTOFF_VALUES,
                STORY_DECK_CONTAINER_SCENE_OBJECT_NAME,
                DECK_ENTRY_Z,
                *scene,
                MIN_DECK_ENTRIES_TO_SCROLL
            );
            
            for (const auto& cardFamilyEntry: game_constants::CARD_FAMILY_NAMES_TO_TEXTURES)
            {
                {
                    auto cardFamilyEntrySceneObject = scene->CreateSceneObject();
                    cardFamilyEntrySceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + DECK_ENTRY_SHADER);
                    cardFamilyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MIN_X_UNIFORM_NAME] = STORY_DECK_ENTRY_CUTOFF_VALUES.s;
                    cardFamilyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUTOFF_MAX_X_UNIFORM_NAME] = STORY_DECK_ENTRY_CUTOFF_VALUES.t;
                    cardFamilyEntrySceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = DECK_ENTRY_ALPHA;
                    cardFamilyEntrySceneObject->mEffectTextureResourceIds[0] = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + DECK_ENTRY_MASK_TEXTURE_FILE_NAME);
                    cardFamilyEntrySceneObject->mScale = glm::vec3(STORY_DECK_SELECTION_ENTRY_SCALE);
                    cardFamilyEntrySceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + cardFamilyEntry.second);
                    
                    CardFamilyEntry cardEntry;
                    cardEntry.mCardFamilyName = cardFamilyEntry.first;
                    cardEntry.mSceneObjects.emplace_back(cardFamilyEntrySceneObject);
                    mCardFamilyContainerBot->AddItem(std::move(cardEntry), EntryAdditionStrategy::ADD_ON_THE_BACK);
                }
            }
            
            if (DataRepository::GetInstance().GetMutationLevelVictories(0) > 0)
            {
                mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
                (
                    SELECT_DECK_BUTTON_POSITION,
                    BUTTON_SCALE,
                    game_constants::DEFAULT_FONT_NAME,
                    "Select",
                    START_NEW_STORY_BUTTON_SCENE_OBJECT_NAME,
                    [=]()
                    {
                        TransitionToSubScene(SubSceneType::MUTATION_SELECTION, scene);
                    },
                    *scene
                ));
            }
            else
            {
                mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
                (
                    START_NEW_STORY_BUTTON_POSITION,
                    BUTTON_SCALE,
                    game_constants::DEFAULT_FONT_NAME,
                    "Start",
                    START_NEW_STORY_BUTTON_SCENE_OBJECT_NAME,
                    [=]()
                    {
                        DataRepository::GetInstance().SetCurrentStoryMutationLevel(0);
                        DataRepository::GetInstance().SetCurrentStoryPlayerDeck(mQuickPlayData->mBotPlayerDeck);
                        DataRepository::GetInstance().FlushStateToFile();
                        StartNewStory();
                    },
                    *scene
                ));
            }
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                BACK_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Back",
                BACK_BUTTON_NAME,
                [=](){ TransitionToSubScene(SubSceneType::MAIN, scene); },
                *scene
            ));
            
            DeckSelected(0, false, scene);
        } break;
            
        case SubSceneType::MUTATION_SELECTION:
        {
            scene::TextSceneObjectData textMutationSelectionPrompt;
            textMutationSelectionPrompt.mFontName = game_constants::DEFAULT_FONT_NAME;
            textMutationSelectionPrompt.mText = "Select Mutation Level";
            auto mutationSelectionTextSceneObject = scene->CreateSceneObject(MUTATION_SELECTION_PROMPT_SCENE_OBJECT_NAME);
            mutationSelectionTextSceneObject->mSceneObjectTypeData = std::move(textMutationSelectionPrompt);
            mutationSelectionTextSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            mutationSelectionTextSceneObject->mPosition = MUTATION_SELECTION_TEXT_POSITION;
            mutationSelectionTextSceneObject->mScale = BUTTON_SCALE;
            
            scene::TextSceneObjectData textMutationValue;
            textMutationValue.mFontName = game_constants::DEFAULT_FONT_NAME;
            textMutationValue.mText = "0";
            auto mutationValueSceneObject = scene->CreateSceneObject(MUTATION_VALUE_SCENE_OBJECT_NAME);
            mutationValueSceneObject->mSceneObjectTypeData = std::move(textMutationValue);
            mutationValueSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            mutationValueSceneObject->mPosition = MUTATION_VALUE_POSITION;
            mutationValueSceneObject->mScale = BUTTON_SCALE;
            
            auto mutationFireSceneObject = scene->CreateSceneObject(MUTATION_FIRE_SCENE_OBJECT_NAME);
            mutationFireSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + MUTATION_FIRE_SHADER_FILE_NAME);
            mutationFireSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + MUTATION_FIRE_TEXTURE_FILE_NAME);
            mutationFireSceneObject->mPosition = MUTATION_FIRE_POSITION;
            mutationFireSceneObject->mScale = MUTATION_FIRE_SCALE;
            mutationFireSceneObject->mShaderFloatUniformValues[COLOR_FACTOR_R_UNIFORM_NAME] = 1.0f;
            mutationFireSceneObject->mShaderFloatUniformValues[COLOR_FACTOR_G_UNIFORM_NAME] = 1.0f;
            mutationFireSceneObject->mShaderFloatUniformValues[COLOR_FACTOR_B_UNIFORM_NAME] = 1.0f;
            mutationFireSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            
            auto brazierSceneObject = scene->CreateSceneObject(BRAZIER_SCENE_OBJECT_NAME);
            brazierSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + BRAZIER_TEXTURE_FILE_NAME);
            brazierSceneObject->mPosition = BRAZIER_POSITION;
            brazierSceneObject->mScale = BRAZIER_SCALE;
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                MUTATION_PLUS_BUTTON_POSITION,
                PLUS_BUTTON_SCALE,
                PLUS_BUTTON_TEXTURE_FILE_NAME,
                MUTATION_PLUS_BUTTON_NAME,
                [=]()
                {
                    auto maxMutationLevelAllowed = math::Min(game_constants::MAX_MUTATION_LEVEL, DataRepository::GetInstance().GetMaxMutationLevelWithAtLeastOneVictory() + 1);
                    SetMutationLevel(math::Min(maxMutationLevelAllowed, mQuickPlayData->mMutationLevel + 1), scene);
                                     
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                 MUTATION_MINUS_BUTTON_POSITION,
                 MINUS_BUTTON_SCALE,
                 MINUS_BUTTON_TEXTURE_FILE_NAME,
                 MUTATION_MINUS_BUTTON_NAME,
                 [=](){ SetMutationLevel(math::Max(0, mQuickPlayData->mMutationLevel - 1), scene); },
                 *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                SELECT_MUTATION_START_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Start",
                START_NEW_STORY_BUTTON_SCENE_OBJECT_NAME,
                [=]()
                {
                    DataRepository::GetInstance().SetCurrentStoryMutationLevel(mQuickPlayData->mMutationLevel);
                    DataRepository::GetInstance().SetCurrentStoryPlayerDeck(mQuickPlayData->mBotPlayerDeck);
                    DataRepository::GetInstance().FlushStateToFile();
                    StartNewStory();
                },
                *scene
            ));
        
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                SELECT_MUTATION_BACK_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Back",
                BACK_BUTTON_NAME,
                [=](){ GoToPreviousSubScene(scene); },
                *scene
            ));
            
            SetMutationLevel(mQuickPlayData->mMutationLevel, scene);
        } break;
        
        case SubSceneType::EXTRAS:
        {
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                STATS_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Statistics",
                STATS_BUTTON_NAME,
                [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(STATS_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                ACHIEVEMENTS_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Achievements",
                ACHIEVEMENTS_BUTTON_NAME,
                [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::ACHIEVEMENTS_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                },
                *scene
            ));
            
//            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
//            (
//                ENTER_GIFT_CODE_BUTTON_POSITION,
//                BUTTON_SCALE,
//                game_constants::DEFAULT_FONT_NAME,
//                "Enter Gift Code",
//                ENTER_GIFT_CODE_BUTTON_NAME,
//                [=]()
//                {
//                    if (IsDisconnected())
//                    {
//                        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::DISCONNECTED_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
//                    }
//                    else
//                    {
//                        OnEnterGiftCodeButtonPressed();
//                    }
//                },
//                *scene
//            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                PRIVACY_POLICY_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Privacy Policy",
                PRIVACY_POLICY_BUTTON_NAME,
                [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(PRIVACY_POLICY_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                RELEASE_NOTES_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Release Notes",
                RELEASE_NOTES_BUTTON_NAME,
                [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(RELEASE_NOTES_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                CREDITS_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Credits",
                CREDITS_BUTTON_NAME,
                [=]()
                {
                    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(CREDITS_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
                },
                *scene
            ));
            
            mAnimatedButtons.emplace_back(std::make_unique<AnimatedButton>
            (
                BACK_BUTTON_POSITION,
                BUTTON_SCALE,
                game_constants::DEFAULT_FONT_NAME,
                "Back",
                BACK_BUTTON_NAME,
                [=](){ GoToPreviousSubScene(scene); },
                *scene
            ));
        } break;
            
        default: break;
    }
    
    size_t sceneObjectIndex = 0;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == MUTATION_SCENE_OBJECT_NAME)
        {
            if (mActiveSubScene != SubSceneType::MUTATION_SELECTION)
            {
                continue;
            }
            else
            {
                sceneObject->mInvisible = false;
            }
        }
        else if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            continue;
        }
        
        if (sceneObject->mName == STORY_DECK_SCENE_OBJECT_NAMES[0] ||
            sceneObject->mName == STORY_DECK_SCENE_OBJECT_NAMES[1] ||
            sceneObject->mName == STORY_DECK_SCENE_OBJECT_NAMES[2] ||
            sceneObject->mName == MUTATION_FIRE_SCENE_OBJECT_NAME)
        {
            continue;
        }
        
        sceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS, animation_flags::NONE, sceneObjectIndex++ * STAGGERED_ITEM_ALPHA_DELAY_SECS), [=]()
        {
            if (mActiveSubScene == SubSceneType::NEW_STORY_DECK_SELECTION)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::SELECT_DECK_1_TUTORIAL);
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::SELECT_DECK_2_TUTORIAL);
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::SELECT_DECK_3_TUTORIAL);
            }
            else if (mActiveSubScene == SubSceneType::MUTATION_SELECTION)
            {
                events::EventSystem::GetInstance().DispatchEvent<events::TutorialTriggerEvent>(tutorials::MUTATIONS_TUTORIAL);
            }
            mTransitioningToSubScene = false;
        });
    }
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::TransitionToSubScene(const SubSceneType subSceneType, std::shared_ptr<scene::Scene> scene)
{
    mTransitioningToSubScene = true;
    for (auto sceneObject: scene->GetSceneObjects())
    {
        if (sceneObject->mName == MUTATION_SCENE_OBJECT_NAME)
        {
            if (mActiveSubScene != SubSceneType::MUTATION_SELECTION)
            {
                continue;
            }
        }
        else if (STATIC_SCENE_ELEMENTS.count(sceneObject->mName))
        {
            continue;
        }
        
        CoreSystemsEngine::GetInstance().GetAnimationManager().StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(sceneObject, 0.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [=]()
        {
            InitSubScene(subSceneType, scene);
            OnWindowResize(events::WindowResizeEvent());
        });
    }
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::BattleModeSelected(const strutils::StringId& buttonName)
{
    auto& resourceLoadingService = CoreSystemsEngine::GetInstance().GetResourceLoadingService();
    auto scene = CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::MAIN_MENU_SCENE);
    
    scene->FindSceneObject(NORMAL_BATTLE_MODE_BUTTON_NAME)->mShaderResourceId = resourceLoadingService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SELECTABLE_BUTTON_SHADER_FILE_NAME);
    scene->FindSceneObject(REPLAY_BATTLE_MODE_BUTTON_NAME)->mShaderResourceId = resourceLoadingService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SELECTABLE_BUTTON_SHADER_FILE_NAME);
    scene->FindSceneObject(AI_DEMO_BATTLE_MODE_BUTTON_NAME)->mShaderResourceId = resourceLoadingService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + SELECTABLE_BUTTON_SHADER_FILE_NAME);
    
    scene->FindSceneObject(NORMAL_BATTLE_MODE_BUTTON_NAME)->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = DESELECTED_BUTTON_COLOR;
    scene->FindSceneObject(REPLAY_BATTLE_MODE_BUTTON_NAME)->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = DESELECTED_BUTTON_COLOR;
    scene->FindSceneObject(AI_DEMO_BATTLE_MODE_BUTTON_NAME)->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = DESELECTED_BUTTON_COLOR;
    
    if (!buttonName.isEmpty())
    {
        scene->FindSceneObject(buttonName)->mShaderVec3UniformValues[game_constants::CUSTOM_COLOR_UNIFORM_NAME] = SELECTED_BUTTON_COLOR;
        mQuickPlayData->mBattleControlType = BATTLE_MODE_BUTTON_NAMES_TO_BATTLE_CONTROL_TYPE.at(buttonName);
        
        auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
        for (auto& deckSelectionSceneObject: mDeckSelectionSceneObjects)
        {
            animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(deckSelectionSceneObject, buttonName == REPLAY_BATTLE_MODE_BUTTON_NAME ? 0.0f : 1.0f, SUBSCENE_ITEM_FADE_IN_OUT_DURATION_SECS), [](){});
        }
    }
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::DeckSelected(const int selectedDeckIndex, const bool forTopPlayer, std::shared_ptr<scene::Scene> scene)
{
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    auto& respectiveDeckContainer = forTopPlayer ? mCardFamilyContainerTop : mCardFamilyContainerBot;
    
    for (auto i = 0; i < static_cast<int>(respectiveDeckContainer->GetItems().size()); ++i)
    {
        auto& sceneObject = respectiveDeckContainer->GetItems()[i].mSceneObjects.front();
        auto targetScale = glm::vec3((mActiveSubScene == SubSceneType::NEW_STORY_DECK_SELECTION ? STORY_DECK_SELECTION_ENTRY_SCALE : DECK_SWIPEABLE_ENTRY_SCALE) * (selectedDeckIndex == i ? DECK_SELECTED_MAX_SCALE_FACTOR : DECK_SELECTED_MIN_SCALE_FACTOR));
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(sceneObject, sceneObject->mPosition, targetScale, DECK_SELECTION_ANIMATION_DURATION_SECS, animation_flags::IGNORE_X_COMPONENT, 0.0f, math::ElasticFunction, math::TweeningMode::EASE_IN), [=](){});
    }
    
    if (mActiveSubScene == SubSceneType::NEW_STORY_DECK_SELECTION)
    {
        mQuickPlayData->mBotPlayerDeck = CardDataRepository::GetInstance().GetStoryStartingFamilyCards(respectiveDeckContainer->GetItems()[selectedDeckIndex].mCardFamilyName);
        
        for (auto i = 0; i < AVAILABLE_STORY_DECKS_COUNT; ++i)
        {
            animationManager.StopAllAnimationsPlayingForSceneObject(STORY_DECK_SCENE_OBJECT_NAMES[i]);
            
            if (i == selectedDeckIndex)
            {
                animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(scene->FindSceneObject(STORY_DECK_SCENE_OBJECT_NAMES[i]), 1.0f, DECK_SELECTION_ANIMATION_DURATION_SECS), [=](){});
            }
            else
            {
                scene->FindSceneObject(STORY_DECK_SCENE_OBJECT_NAMES[i])->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 0.0f;
            }
        }
    }
    else
    {
        if (forTopPlayer)
        {
            mQuickPlayData->mTopPlayerDeck = CardDataRepository::GetInstance().GetCardIdsByFamily(respectiveDeckContainer->GetItems()[selectedDeckIndex].mCardFamilyName);
        }
        else
        {
            mQuickPlayData->mBotPlayerDeck = CardDataRepository::GetInstance().GetCardIdsByFamily(respectiveDeckContainer->GetItems()[selectedDeckIndex].mCardFamilyName);
        }
    }
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::GoToPreviousSubScene(std::shared_ptr<scene::Scene> mainScene)
{
    auto previousSubScene = mPreviousSubSceneStack.top();
    mPreviousSubSceneStack.pop();
    mShouldPushToPreviousSceneStack = false;
    TransitionToSubScene(previousSubScene, mainScene);
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::StartNewStory()
{
    DataRepository::GetInstance().SetIsCurrentlyPlayingStoryMode(true);
    DataRepository::GetInstance().SetStoryStartingGold(DataRepository::GetInstance().CurrencyCoins().GetValue());
    events::EventSystem::GetInstance().DispatchEvent<events::SendPlayMessageEvent>();
    events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(game_constants::STORY_MAP_SCENE, SceneChangeType::CONCRETE_SCENE_ASYNC_LOADING, PreviousSceneDestructionType::DESTROY_PREVIOUS_SCENE);
    DataRepository::GetInstance().FlushStateToFile();
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::OnEnterGiftCodeButtonPressed()
{
#if defined(MACOS) || defined(MOBILE_FLOW)
    apple_utils::GetMessageBoxTextInput([](const std::string& giftCodeEntered)
    {
        strutils::StringId resultProductName;
        gift_utils::ClaimGiftCode(giftCodeEntered, resultProductName);
        
        if (DataRepository::GetInstance().GetCurrentGiftCodeClaimedResultType() == GiftCodeClaimedResultType::SUCCESS)
        {
            const auto& productDefinition = ProductRepository::GetInstance().GetProductDefinition(resultProductName);
            if (resultProductName == STORY_HEALTH_REFILL_PRODUCT_NAME)
            {
                DataRepository::GetInstance().StoryCurrentHealth().SetValue(DataRepository::GetInstance().GetStoryMaxHealth());
            }
            else if (resultProductName == NORMAL_PACK_PRODUCT_NAME)
            {
                DataRepository::GetInstance().AddPendingCardPack(CardPackType::NORMAL);
            }
            else if (resultProductName == GOLDEN_PACK_PRODUCT_NAME)
            {
                DataRepository::GetInstance().AddPendingCardPack(CardPackType::GOLDEN);
            }
            else if (resultProductName == COINS_S_PRODUCT_NAME || resultProductName == COINS_M_PRODUCT_NAME || resultProductName == COINS_L_PRODUCT_NAME)
            {
                DataRepository::GetInstance().CurrencyCoins().SetValue(DataRepository::GetInstance().CurrencyCoins().GetValue() + productDefinition.mPrice);
            }
            
            DataRepository::GetInstance().FlushStateToFile();
        }
        
        events::EventSystem::GetInstance().DispatchEvent<events::SceneChangeEvent>(GIFT_CODE_CLAIM_SCENE, SceneChangeType::MODAL_SCENE, PreviousSceneDestructionType::RETAIN_PREVIOUS_SCENE);
    });
#endif
}

///------------------------------------------------------------------------------------------------

bool MainMenuSceneLogicManager::IsDisconnected() const
{
#if defined(MACOS) || defined(MOBILE_FLOW)
    return !apple_utils::IsConnectedToTheInternet();
#else
    return !window_utils::IsConnectedToTheInternet();
#endif
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::CreateMutationObject(std::shared_ptr<scene::Scene> scene)
{
    auto mutationSceneObject = scene->CreateSceneObject(MUTATION_SCENE_OBJECT_NAME);
    mutationSceneObject->mTextureResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + MUTATION_TEXTURE_FILE_NAME);
    mutationSceneObject->mMeshResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_MESHES_ROOT + MUTATION_MESH_FILE_NAME);
    mutationSceneObject->mShaderResourceId = CoreSystemsEngine::GetInstance().GetResourceLoadingService().LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + MUTATION_SHADER_FILE_NAME);
    mutationSceneObject->mShaderVec3UniformValues[POINT_LIGHT_POSITION_UNIFORM_NAME] = POINT_LIGHT_POSITION;
    mutationSceneObject->mShaderVec3UniformValues[DIFFUSE_COLOR_UNIFORM_NAME] = DIFFUSE_COLOR;
    mutationSceneObject->mShaderVec3UniformValues[SPEC_COLOR_UNIFORM_NAME] = SPEC_COLOR;
    mutationSceneObject->mShaderVec3UniformValues[AMBIENT_COLOR_UNIFORM_NAME] = AMB_COLOR;
    mutationSceneObject->mShaderFloatUniformValues[POINT_LIGHT_POWER_UNIFORM_NAME] = POINT_LIGHT_POWER;
    mutationSceneObject->mShaderBoolUniformValues[AFFECTED_BY_LIGHT_UNIFORM_NAME] = true;

    mutationSceneObject->mPosition = MUTATION_POSITION;
    mutationSceneObject->mScale = MUTATION_SCALE;
    mutationSceneObject->mInvisible = true;
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::SetMutationLevel(const int mutationLevel, std::shared_ptr<scene::Scene> scene)
{
    int valueDelta = mutationLevel - mQuickPlayData->mMutationLevel;
    mQuickPlayData->mMutationLevel = mutationLevel;
    auto mutationValueSceneObject = scene->FindSceneObject(MUTATION_VALUE_SCENE_OBJECT_NAME);
    std::get<scene::TextSceneObjectData>(mutationValueSceneObject->mSceneObjectTypeData).mText = std::to_string(mutationLevel);
    
    auto boundingRect = scene_object_utils::GetSceneObjectBoundingRect(*mutationValueSceneObject);
    auto textLength = boundingRect.topRight.x - boundingRect.bottomLeft.x;
    mutationValueSceneObject->mPosition.x = MUTATION_VALUE_POSITION.x - textLength/2.0f;
    
    auto fireSceneObject = scene->FindSceneObject(MUTATION_FIRE_SCENE_OBJECT_NAME);
    auto mutationSceneObject = scene->FindSceneObject(MUTATION_SCENE_OBJECT_NAME);
    
    auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
    
    animationManager.StopAnimation(FIRE_ALPHA_ANIMATION_NAME);
    animationManager.StopAnimation(FIRE_RED_COLOR_ANIMATION_NAME);
    animationManager.StopAnimation(FIRE_GREEN_COLOR_ANIMATION_NAME);
    animationManager.StopAnimation(MUTATION_PULSE_ANIMATION_NAME);
    
    animationManager.StartAnimation(std::make_unique<rendering::TweenAlphaAnimation>(fireSceneObject, mQuickPlayData->mMutationLevel ? 1.0f : 0.0f, 0.5f), [](){}, FIRE_ALPHA_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(fireSceneObject->mShaderFloatUniformValues[COLOR_FACTOR_R_UNIFORM_NAME], 1.0f - mQuickPlayData->mMutationLevel * FIRE_COLOR_R_INCREMENTS, 0.5f), [](){}, FIRE_RED_COLOR_ANIMATION_NAME);
    animationManager.StartAnimation(std::make_unique<rendering::TweenValueAnimation>(fireSceneObject->mShaderFloatUniformValues[COLOR_FACTOR_G_UNIFORM_NAME], 1.0f + mQuickPlayData->mMutationLevel * FIRE_COLOR_G_INCREMENTS, 0.5f), [](){}, FIRE_GREEN_COLOR_ANIMATION_NAME);
    
    if (valueDelta != 0)
    {
        animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(mutationSceneObject, mutationSceneObject->mPosition, MUTATION_SCALE * (valueDelta > 0 ? 1.25f : 0.8f), 0.1f, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
        {
            auto& animationManager = CoreSystemsEngine::GetInstance().GetAnimationManager();
            animationManager.StartAnimation(std::make_unique<rendering::TweenPositionScaleAnimation>(mutationSceneObject, mutationSceneObject->mPosition, MUTATION_SCALE, 0.1f, animation_flags::IGNORE_X_COMPONENT | animation_flags::IGNORE_Y_COMPONENT, 0.0f, math::LinearFunction, math::TweeningMode::EASE_OUT), [=]()
            {
                mutationSceneObject->mScale = MUTATION_SCALE;
            }, MUTATION_PULSE_ANIMATION_NAME);
        }, MUTATION_PULSE_ANIMATION_NAME);
    }
    
    for (auto i = 0; i < game_constants::MAX_MUTATION_LEVEL; ++i)
    {
        scene->RemoveSceneObject(strutils::StringId(MUTATION_CHANGES_TEXT_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
    }
    
    if (mQuickPlayData->mMutationLevel == 0)
    {
        int i = 3;
        
        scene::TextSceneObjectData textMutationChange;
        textMutationChange.mFontName = game_constants::DEFAULT_FONT_NAME;
        textMutationChange.mText = "No Mutations";
        
        auto mutationChangeSceneObject = scene->CreateSceneObject(strutils::StringId(MUTATION_CHANGES_TEXT_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
        mutationChangeSceneObject->mSceneObjectTypeData = std::move(textMutationChange);
        mutationChangeSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
        mutationChangeSceneObject->mPosition = MUTATION_CHANGE_TEXT_INIT_POSITION - glm::vec3(0.0f, i * 0.025f, 0.0f);
        mutationChangeSceneObject->mScale = MUTATION_CHANGE_TEXT_SCALE * 2.0f;
    }
    else
    {
        for (auto i = 0; i < mQuickPlayData->mMutationLevel; ++i)
        {
            scene::TextSceneObjectData textMutationChange;
            textMutationChange.mFontName = game_constants::DEFAULT_FONT_NAME;
            
            auto text = game_constants::MUTATION_TEXTS[i];
            for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
            {
                strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), text);
            }
            
            textMutationChange.mText = std::string(1, symbolic_glyph_names::SYMBOLIC_NAMES.at(symbolic_glyph_names::SKULL)) + text;
            auto mutationChangeSceneObject = scene->CreateSceneObject(strutils::StringId(MUTATION_CHANGES_TEXT_SCENE_OBJECT_NAME_PREFIX + std::to_string(i)));
            mutationChangeSceneObject->mSceneObjectTypeData = std::move(textMutationChange);
            mutationChangeSceneObject->mShaderFloatUniformValues[game_constants::CUSTOM_ALPHA_UNIFORM_NAME] = 1.0f;
            mutationChangeSceneObject->mPosition = MUTATION_CHANGE_TEXT_INIT_POSITION - glm::vec3(0.0f, i * 0.025f, 0.0f);
            mutationChangeSceneObject->mScale = MUTATION_CHANGE_TEXT_SCALE;
        }
    }
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::OnWindowResize(const events::WindowResizeEvent& event)
{
    CoreSystemsEngine::GetInstance().GetSceneManager().FindScene(game_constants::MAIN_MENU_SCENE)->RecalculatePositionOfEdgeSnappingSceneObjects();
}

///------------------------------------------------------------------------------------------------

void MainMenuSceneLogicManager::CheckForCardCompletion()
{
    auto totalCardPoolSize = DataRepository::GetInstance().GetUnlockedCardIds().size() + CardDataRepository::GetInstance().GetCardPackLockedCardRewardsPool().size();
    auto percentageCollection = static_cast<int>((DataRepository::GetInstance().GetUnlockedCardIds().size() * 100.0f)/static_cast<float>(totalCardPoolSize));
    
    if (percentageCollection == 100)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::NORMAL_COLLECTOR);
    }
    
    auto goldenPercentageCollection = static_cast<int>((DataRepository::GetInstance().GetGoldenCardIdMap().size() * 100.0f)/static_cast<float>(totalCardPoolSize));
    if (goldenPercentageCollection == 100)
    {
        events::EventSystem::GetInstance().DispatchEvent<events::AchievementUnlockedTriggerEvent>(achievements::GOLDEN_COLLECTOR);
    }
}

///------------------------------------------------------------------------------------------------
