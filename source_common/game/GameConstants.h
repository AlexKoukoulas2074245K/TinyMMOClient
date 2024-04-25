///------------------------------------------------------------------------------------------------
///  GameConstants.h                                                                                          
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 09/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameConstants_h
#define GameConstants_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

namespace game_constants
{
    // Resources
    inline const std::string DEFAULT_MESH_NAME = "quad.obj";
    inline const std::string DEFAULT_SHADER_NAME = "basic.vs";
    inline const std::string ACTION_HIGHLIGHTER_SHADER_NAME = "action_highlighter_perlin_noise.vs";
    inline const std::string BOARD_CARD_LOCATION_SHADER_NAME = "card_board_location_perlin_noise.vs";
    inline const std::string DEFAULT_TEXTURE_NAME = "debug.png";
    inline const std::string CARD_LOCATION_MASK_TEXTURE_NAME =  "card_location_mask.png";
    inline const std::string GOLDEN_CARD_FLAKES_MASK_TEXTURE_FILE_NAME = "golden_card_flakes_mask.png";
    inline const std::string GOLDEN_SPELL_CARD_FLAKES_MASK_TEXTURE_FILE_NAME = "golden_spell_card_flakes_mask.png";
    inline const std::string GOLDEN_CARD_TEXTURE_FILE_NAME = "card_frame_golden.png";
    inline const std::string CARD_FRAME_NORMAL_TEXTURE_FILE_NAME = "card_frame_normal.png";
    inline const std::string CARD_FRAME_SPELL_TEXTURE_FILE_NAME = "card_frame_spell.png";
    inline const std::string BASIC_CUSTOM_COLOR_SHADER_FILE_NAME = "basic_custom_color.vs";

    // SO Name Prefixes/Postfixes
    inline const std::string TOP_PLAYER_HELD_CARD_SO_NAME_PREFIX = "top_player_held_card_";
    inline const std::string BOT_PLAYER_HELD_CARD_SO_NAME_PREFIX = "bot_player_held_card_";
    inline const std::string TOP_PLAYER_BOARD_CARD_SO_NAME_PREFIX = "top_player_board_card_";
    inline const std::string BOT_PLAYER_BOARD_CARD_SO_NAME_PREFIX = "bot_player_board_card_";
    inline const std::string CARD_FREE_MOVING_ANIMATION_NAME_PRE_FIX = "free_moving_card_";
    inline const std::string BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME_PRE_FIX = "board_side_effect_top_value_";
    inline const std::string BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME_PRE_FIX = "board_side_effect_bot_value_";
    inline const std::string STORY_MAP_NODE_PORTRAIT_SO_NAME_POST_FIX = "_portrait";
    inline const std::string STORY_MAP_NODE_TEXT_SO_NAME_POST_FIX = "_text";
    inline const std::string STORY_MAP_NODE_SECONDARY_TEXT_SO_NAME_POST_FIX = "_text_secondary";
    inline const std::string STORY_MAP_NODE_HEALTH_ICON_SO_NAME_POST_FIX = "_health_icon";
    inline const std::string STORY_MAP_NODE_HEALTH_TEXT_SO_NAME_POST_FIX = "_health_text";
    inline const std::string STORY_MAP_NODE_DAMAGE_ICON_SO_NAME_POST_FIX = "_damage_icon";
    inline const std::string STORY_MAP_NODE_DAMAGE_TEXT_SO_NAME_POST_FIX = "_damage_text";
    inline const std::string STORY_MAP_NODE_WEIGHT_ICON_SO_NAME_POST_FIX = "_weight_icon";
    inline const std::string STORY_MAP_NODE_WEIGHT_TEXT_SO_NAME_POST_FIX = "_weight_text";

    // SO Names
    inline const strutils::StringId TURN_POINTER_SCENE_OBJECT_NAME = strutils::StringId("turn_pointer");
    inline const strutils::StringId TURN_POINTER_HIGHLIGHTER_SCENE_OBJECT_NAME = strutils::StringId("turn_pointer_highlighter");
    inline const strutils::StringId BOARD_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("board_side_effect_top");
    inline const strutils::StringId BOARD_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("board_side_effect_bot");
    inline const strutils::StringId KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("kill_side_effect_top");
    inline const strutils::StringId KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("kill_side_effect_bot");
    inline const strutils::StringId DEMON_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("demon_kill_side_effect_top");
    inline const strutils::StringId DEMON_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("demon_kill_side_effect_bot");
    inline const strutils::StringId SPELL_KILL_SIDE_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("spell_kill_side_effect_top");
    inline const strutils::StringId SPELL_KILL_SIDE_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("spell_kill_side_effect_bot");
    inline const strutils::StringId INSECT_DUPLICATION_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("insect_duplication_effect_top");
    inline const strutils::StringId INSECT_DUPLICATION_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("insect_duplication_effect_bot");
    inline const strutils::StringId NEXT_DINO_DAMAGE_DOUBLING_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("next_dino_damage_doubling_effect_top");
    inline const strutils::StringId NEXT_DINO_DAMAGE_DOUBLING_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("next_dino_damage_doubling_effect_bot");
    inline const strutils::StringId NEXT_DINO_HEAL_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("next_dino_heal_effect_top");
    inline const strutils::StringId NEXT_DINO_HEAL_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("next_dino_heal_effect_bot");
    inline const strutils::StringId RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("rodent_lifesteal_on_attacks_effect_top");
    inline const strutils::StringId RODENT_LIFESTEAL_ON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("rodent_lifesteal_on_attacks_effect_bot");
    inline const strutils::StringId DOUBLE_POISON_ATTACKS_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("double_poison_attacks_effect_top");
    inline const strutils::StringId DOUBLE_POISON_ATTACKS_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("double_poison_attacks_effect_bot");
    inline const strutils::StringId INSECT_VIRUS_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("insect_virus_effect_top");
    inline const strutils::StringId INSECT_VIRUS_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("insect_virus_effect_bot");
    inline const strutils::StringId DIG_NO_FAIL_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("dig_no_fail_effect_top");
    inline const strutils::StringId DIG_NO_FAIL_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("dig_no_fail_effect_bot");
    inline const strutils::StringId PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("permanent_continual_weight_reduction_effect_top");
    inline const strutils::StringId PERMANENT_CONTINUAL_WEIGHT_REDUCTION_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("permanent_continual_weight_reduction_effect_bot");
    inline const strutils::StringId EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_TOP_SCENE_OBJECT_NAME = strutils::StringId("every_third_card_played_has_zero_cost_effect_top");
    inline const strutils::StringId EVERY_THIRD_CARD_PLAYED_HAS_ZERO_COST_EFFECT_BOT_SCENE_OBJECT_NAME = strutils::StringId("every_third_card_played_has_zero_cost_effect_bot");
    inline const strutils::StringId OVERLAY_SCENE_OBJECT_NAME = strutils::StringId("overlay");
    inline const strutils::StringId GUI_SETTINGS_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("settings_button");
    inline const strutils::StringId GUI_STORY_CARDS_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("story_cards_button");
    inline const strutils::StringId GUI_INVENTORY_BUTTON_SCENE_OBJECT_NAME = strutils::StringId("inventory_button");
    inline const strutils::StringId GUI_COIN_STACK_SCENE_OBJECT_NAME = strutils::StringId("coin_stack");
    inline const strutils::StringId GUI_COIN_VALUE_TEXT_SCENE_OBJECT_NAME = strutils::StringId("coin_value_text");
    
    // Animation Names
    inline const strutils::StringId GOLDEN_CARD_LIGHT_RAY_ANIMATION_NAME = strutils::StringId("golden_card_light_ray_animation");
    inline const strutils::StringId SCENE_SPEED_DILATION_ANIMATION_NAME = strutils::StringId("scene_speed_dilation_animation");
    inline const strutils::StringId STAT_PARTICLE_FLYING_ANIMATION_NAME = strutils::StringId("coin_flying_animation");
    inline const strutils::StringId OVERLAY_DARKENING_ANIMATION_NAME = strutils::StringId("overlay_darkening_animation");

    // Fonts
    inline const strutils::StringId DEFAULT_FONT_NAME = strutils::StringId("font");
    inline const strutils::StringId DEFAULT_FONT_BLACK_NAME = strutils::StringId("font_black");
    inline const strutils::StringId FONT_PLACEHOLDER_DAMAGE_NAME = strutils::StringId("font_placeholder_damage");
    inline const strutils::StringId FONT_PLACEHOLDER_WEIGHT_NAME = strutils::StringId("font_placeholder_weight");

    // Uniforms
    inline const strutils::StringId CARD_HIGHLIGHTER_INVALID_ACTION_UNIFORM_NAME = strutils::StringId("invalid_action");
    inline const strutils::StringId CUSTOM_ALPHA_UNIFORM_NAME = strutils::StringId("custom_alpha");
    inline const strutils::StringId TIME_UNIFORM_NAME = strutils::StringId("time");
    inline const strutils::StringId PERLIN_TIME_SPEED_UNIFORM_NAME = strutils::StringId("time_speed");
    inline const strutils::StringId PERLIN_RESOLUTION_UNIFORM_NAME = strutils::StringId("perlin_resolution");
    inline const strutils::StringId PERLIN_CLARITY_UNIFORM_NAME = strutils::StringId("perlin_clarity");
    inline const strutils::StringId CARD_WEIGHT_INTERACTIVE_MODE_UNIFORM_NAME = strutils::StringId("weight_interactive_mode");
    inline const strutils::StringId CARD_DAMAGE_INTERACTIVE_MODE_UNIFORM_NAME = strutils::StringId("damage_interactive_mode");
    inline const strutils::StringId LIGHT_POS_X_UNIFORM_NAME = strutils::StringId("light_pos_x");
    inline const strutils::StringId IS_GOLDEN_CARD_UNIFORM_NAME = strutils::StringId("golden_card");
    inline const strutils::StringId IS_HELD_CARD_UNIFORM_NAME = strutils::StringId("held_card");
    inline const strutils::StringId DORMANT_CARD_VALUE_UNIFORM_NAME = strutils::StringId("dormant_value");
    inline const strutils::StringId CUTOFF_MIN_X_UNIFORM_NAME = strutils::StringId("cutoff_min_x");
    inline const strutils::StringId CUTOFF_MAX_X_UNIFORM_NAME = strutils::StringId("cutoff_max_x");
    inline const strutils::StringId CUTOFF_MIN_Y_UNIFORM_NAME = strutils::StringId("cutoff_min_y");
    inline const strutils::StringId CUTOFF_MAX_Y_UNIFORM_NAME = strutils::StringId("cutoff_max_y");
    inline const strutils::StringId CUSTOM_COLOR_UNIFORM_NAME = strutils::StringId("custom_color");
    inline const strutils::StringId METALLIC_STAT_CONTAINER_UNIFORM_NAME = strutils::StringId("metallic_container");

    // Card Family Names
    inline const strutils::StringId INSECTS_FAMILY_NAME = strutils::StringId("insects");
    inline const strutils::StringId RODENTS_FAMILY_NAME = strutils::StringId("rodents");
    inline const strutils::StringId DINOSAURS_FAMILY_NAME = strutils::StringId("dinosaurs");
    inline const strutils::StringId DEMONS_GENERIC_FAMILY_NAME = strutils::StringId("demons");
    inline const strutils::StringId DEMONS_NORMAL_FAMILY_NAME = strutils::StringId("demons_normal");
    inline const strutils::StringId DEMONS_MEDIUM_FAMILY_NAME = strutils::StringId("demons_medium");
    inline const strutils::StringId DEMONS_HARD_FAMILY_NAME = strutils::StringId("demons_hard");
    inline const strutils::StringId DEMONS_BOSS_FAMILY_NAME = strutils::StringId("demons_boss");
    inline const strutils::StringId DRAGON_FAMILY_NAME = strutils::StringId("dragon");

    // Card Names
    inline const strutils::StringId EMPTY_DECK_TOKEN_CARD_NAME = strutils::StringId("Card Token");
    inline const strutils::StringId EMERALD_DRAGON_NAME = strutils::StringId("Emerald Dragon");

    // Scenes
    inline const strutils::StringId BATTLE_SCENE = strutils::StringId("battle_scene");
    inline const strutils::StringId MAIN_MENU_SCENE = strutils::StringId("main_menu_scene");
    inline const strutils::StringId STORY_MAP_SCENE = strutils::StringId("story_map_scene");
    inline const strutils::StringId EVENT_SCENE = strutils::StringId("event_scene");
    inline const strutils::StringId SHOP_SCENE = strutils::StringId("shop_scene");
    inline const strutils::StringId DISCONNECTED_SCENE = strutils::StringId("disconnected_scene");
    inline const strutils::StringId SETTINGS_SCENE = strutils::StringId("settings_scene");
    inline const strutils::StringId CARD_LIBRARY_SCENE = strutils::StringId("card_library_scene");
    inline const strutils::StringId ACHIEVEMENTS_SCENE = strutils::StringId("achievements_scene");
    inline const strutils::StringId INVENTORY_SCENE = strutils::StringId("inventory_scene");
    inline const strutils::StringId WHEEL_OF_FORTUNE_SCENE = strutils::StringId("wheel_of_fortune_scene");
    inline const strutils::StringId CLOUD_DATA_CONFIRMATION_SCENE = strutils::StringId("cloud_data_confirmation_scene");
    inline const strutils::StringId FIRST_GAME_LOCK_SCENE = strutils::StringId("first_game_lock_scene");
    inline const strutils::StringId LOADING_SCENE = strutils::StringId("loading_scene");
    inline const strutils::StringId CARD_PACK_REWARD_SCENE = strutils::StringId("card_pack_reward_scene");
    inline const strutils::StringId TUTORIAL_SCENE = strutils::StringId("tutorial_scene");
    inline const strutils::StringId ACHIEVEMENT_UNLOCKED_SCENE = strutils::StringId("achievement_unlocked_scene");
    inline const strutils::StringId CARD_SELECTION_REWARD_SCENE_NAME = strutils::StringId("card_selection_reward_scene");

    // General Game Constants
    inline constexpr int REMOTE_PLAYER_INDEX = 0;
    inline constexpr int LOCAL_PLAYER_INDEX = 1;
    inline constexpr int MAX_BOARD_CARDS = 4;
    inline constexpr int CARD_INTERACTIVE_MODE_DEFAULT = 0;
    inline constexpr int CARD_INTERACTIVE_MODE_INTERACTIVE = 1;
    inline constexpr int CARD_INTERACTIVE_MODE_NONINTERACTIVE = 2;
    inline constexpr int IN_GAME_CARD_PUSH_THRESHOLD = 4;
    inline constexpr int BOARD_SIDE_EFFECT_VALUE_SO_COUNT = 2;
    inline constexpr int CARD_TOOLTIP_TEXT_ROWS_COUNT = 4;
    inline constexpr int TOP_PLAYER_DEFAULT_HEALTH = 33;
    inline constexpr int BOT_PLAYER_DEFAULT_HEALTH = 35;
    inline constexpr int TOP_PLAYER_DEFAULT_WEIGHT = 3;
    inline constexpr int BOT_PLAYER_DEFAULT_WEIGHT = 6;
    inline constexpr int TOP_PLAYER_DEFAULT_WEIGHT_LIMIT = 10;
    inline constexpr int BOT_PLAYER_DEFAULT_WEIGHT_LIMIT = 10;
    inline constexpr int STORY_DEFAULT_MAX_HEALTH = 80;
    inline constexpr int ZERO_COST_TIME_WEIGHT_VALUE = 100;
    inline constexpr int MAX_MUTATION_LEVEL = 10;
    inline constexpr int MUTATION_HALF_COINS = 1;
    inline constexpr int MUTATION_INCREASED_SHOP_PRICES = 2;
    inline constexpr int MUTATION_INCREASED_STARTING_WEIGHT_FOR_OPPONENTS = 3;
    inline constexpr int MUTATION_INCREASED_STARTING_HEALTH_FOR_OPPONENTS = 4;
    inline constexpr int MUTATION_REDUCED_NORMAL_CARD_DAMAGE = 5;
    inline constexpr int MUTATION_INCREASED_CARD_WEIGHT = 6;
    inline constexpr int MUTATION_INCREASED_STARTING_DAMAGE_FOR_OPPONENTS = 7;
    inline constexpr int MUTATION_NO_HEAL_AFTER_FIRST_BOSS = 8;
    inline constexpr int MUTATION_ALL_NORMAL_FIGHTS_BECOME_ELITE = 9;
    inline constexpr int MUTATION_FINAL_BOSS_REVIVES = 10;

    inline const float CARD_COMPONENT_Z_OFFSET = 0.1f;
    inline const float CARD_BOUNDING_RECT_X_MULTIPLIER = 0.5f;
    inline const float IN_GAME_PLAYED_CARD_Z = 0.1f;
    inline const float IN_GAME_HELD_CARD_Z = 2.3f;
    inline const float IN_GAME_CARD_BASE_SCALE = 0.1f;
    inline const float IN_GAME_CARD_PORTRAIT_SCALE = 0.025f;
    inline const float IN_GAME_CARD_PORTRAIT_Y_OFFSET = 0.008f;
    inline const float IN_GAME_CARD_PROPERTY_ICON_SCALE = 0.04f;
    inline const float IN_GAME_CARD_SPELL_PROPERTY_ICON_X_OFFSET = 0.000f;
    inline const float IN_GAME_CARD_SPELL_PROPERTY_ICON_Y_OFFSET = 0.035f;
    inline const float IN_GAME_CARD_PROPERTY_ICON_X_OFFSET = -0.015f;
    inline const float IN_GAME_CARD_PROPERTY_ICON_Y_OFFSET = 0.03f;
    inline const float IN_GAME_CARD_PROPERTY_X_OFFSET = -0.0158f;
    inline const float IN_GAME_CARD_PROPERTY_SCALE = 0.00015f;
    inline const float IN_GAME_CARD_PROPERTY_Y_OFFSET = 0.0285f;
    inline const float IN_GAME_CARD_NAME_X_OFFSET = 0.002f;
    inline const float IN_GAME_CARD_NAME_Y_OFFSET = -0.0145f;
    inline const float IN_GAME_CARD_NAME_SPELL_Y_OFFSET = -0.0125f;
    inline const float IN_GAME_CARD_NAME_SPELL_EFFECT_Y_OFFSET = -0.0205f;
    inline const float IN_GAME_CARD_NAME_SCALE = 0.00012f;
    inline const float IN_GAME_CARD_WIDTH = 0.055f;
    inline const float IN_GAME_CARD_ON_BOARD_WIDTH = 0.045f;
    inline const float IN_GAME_CARD_PUSH_VALUE = 0.003f;
    inline const float IN_GAME_TOP_PLAYER_HELD_CARD_Y = 0.132f;
    inline const float IN_GAME_BOT_PLAYER_HELD_CARD_Y = -0.132f;
    inline const float IN_GAME_TOP_PLAYER_BOARD_CARD_Y = 0.035f;
    inline const float IN_GAME_BOT_PLAYER_BOARD_CARD_Y = -0.035f;
    inline const float IN_GAME_BOT_PLAYER_SELECTED_CARD_Y_OFFSET = 0.039f;
    inline const float IN_GAME_DRAW_CARD_INIT_X = -0.274f;
    inline const float IN_GAME_DRAW_CARD_TOP_PLAYER_MID_POINT_Y = -0.075f;
    inline const float IN_GAME_DRAW_CARD_BOT_PLAYER_MID_POINT_Y = 0.075f;
    inline const float IN_GAME_DRAW_CARD_ANIMATION_DURATION_SECS = 0.5f;
    inline const float IN_GAME_HIGHLIGHTED_CARD_Z = 20.0f;
    inline const float IN_GAME_CARD_FREE_MOVEMENT_ANIMATION_DURATION_SECS = 0.075f;
    inline const float IN_GAME_DRAW_CARD_PUSH_EXISTING_CARDS_ANIMATION_DURATION_SECS = 0.2f;
    inline const float IN_GAME_DRAW_CARD_PUSH_EXISTING_CARDS_ANIMATION_DELAY_SECS = 0.25f;
    inline const float IN_GAME_PLAYED_CARD_SCALE_FACTOR = 0.666f;
    inline const float ACTION_HIGLIGHTER_Z_OFFSET = -0.05f;
    inline const float ACTION_HIGLIGHTER_PERLIN_TIME_SPEED = 12.595f;
    inline const float ACTION_HIGLIGHTER_PERLIN_RESOLUTION = 312.0f;
    inline const float ACTION_HIGLIGHTER_PERLIN_CLARITY = 5.23f;
    inline const float CARD_LOCATION_EFFECT_Z = 1.5f;
    inline const float CARD_LOCATION_EFFECT_TIME_SPEED = 1.0f;
    inline const float CARD_LOCATION_EFFECT_PERLIN_RESOLUTION = 70.0f;
    inline const float TURN_POINTER_ANIMATION_DURATION_SECS = 0.33f;
    inline const float INDIVIDUAL_CARD_BOARD_EFFECT_SCALE_UP_FACTOR = 1.5f;
    inline const float INDIVIDUAL_CARD_BOARD_EFFECT_PULSE_ANIMATION_PULSE_DURATION_SECS = 1.0f;
    inline const float POISON_STACK_SHOW_HIDE_ANIMATION_DURATION_SECS = 1.0f;
    inline const float ARMOR_SHOW_HIDE_ANIMATION_DURATION_SECS = 1.0f;
    inline const float RODENTS_RESPAWN_CHANCE = 0.5f;
    inline const float SCENE_SPEED_DILATION_ANIMATION_DURATION_SECS = 0.5f;
    inline const float PER_ARMOR_DROPPED_DELAY_ANIMATION_DURATION_SECS = 0.3f;
    inline const float RARE_ITEM_COLLECTION_ANIMATION_DURATION_SECS = 1.2f;

    inline const glm::vec3 INDIVIDUAL_CARD_BOARD_EFFECT_SCALE = {1/20.0f, 1/20.0f, 1/20.0f};
    inline const glm::vec3 CARD_HIGHLIGHTER_SCALE = {0.08f, 0.13f, 1.0f};
    inline const glm::vec3 HEALTH_CRYSTAL_TOP_POSITION = {-0.105f, 0.03f, 2.0f};
    inline const glm::vec3 HEALTH_CRYSTAL_BOT_POSITION = {-0.105f, -0.03f, 2.0f};
    inline const glm::vec3 ARMOR_CONTAINER_TOP_POSITION = {-0.105f, 0.036f, 2.05f};
    inline const glm::vec3 ARMOR_CONTAINER_BOT_POSITION = {-0.105f, -0.024f, 2.05f};
    inline const glm::vec3 WEIGHT_CRYSTAL_TOP_POSITION = {0.1f, 0.03f, 2.0f};
    inline const glm::vec3 WEIGHT_CRYSTAL_BOT_POSITION = {0.1f, -0.03f, 2.0f};
    inline const glm::vec3 POISON_STACK_TOP_POSITION = {-0.105f, 0.005f, 2.0f};
    inline const glm::vec3 POISON_STACK_BOT_POSITION = {-0.105f, -0.055f, 2.0f};
    inline const glm::vec3 GAME_BOARD_INIT_POSITION = {-0.039f, 0.011f, 0.0f };
    inline const glm::vec3 GAME_BOARD_INIT_ROTATION = {0.0f, 0.0f, 3.14f };

    inline const glm::vec2 GOLDEN_CARD_LIGHT_POS_MIN_MAX_X = {-0.5f, 0.5f};

    inline const glm::ivec2 STORY_MAP_INIT_COORD = {0, 3};
    inline const glm::ivec2 STORY_MAP_BOSS_COORD = {9, 3};
    inline const glm::ivec2 STORY_NODE_MAP_DIMENSIONS = {10, 7};
    inline const glm::ivec2 TUTORIAL_MAP_INIT_COORD = {0, 1};
    inline const glm::ivec2 TUTORIAL_MAP_BOSS_COORD = {6, 1};
    inline const glm::ivec2 TUTORIAL_NODE_MAP_DIMENSIONS = {7, 3};

    inline float GAME_BOARD_BASED_SCENE_ZOOM_FACTOR = 120.0f;
    inline float GAME_BOARD_GUI_DISTANCE_FACTOR = 2.0f;
    inline float IN_GAME_MOBILE_ONLY_FREE_MOVING_CARD_Y_OFFSET = 0.05f;

    inline constexpr std::pair<int, int> CARD_DELETION_PRODUCT_COORDS = std::make_pair(2, 4);

    inline const std::unordered_map<strutils::StringId, std::string, strutils::StringIdHasher> CARD_FAMILY_NAMES_TO_TEXTURES =
    {
        { INSECTS_FAMILY_NAME, "insect_duplication.png" },
        { RODENTS_FAMILY_NAME, "rodents_attack.png" },
        { DINOSAURS_FAMILY_NAME, "mighty_roar.png" }
    };

    inline const std::vector<std::string> MUTATION_TEXTS =
    {
        "Less <coin> from battles/events",
        "Cards/Artifacts cost more <coin>",
        "All opponents have more<weight>",
        "All opponents have more<health>",
        "Your normal cards have -1<damage>",
        "All your cards have +1<weight>",
        "All opponents have more<damage>",
        "No healing after first boss",
        "All normal fights are elite",
        "Final boss revives",
    };
}

///------------------------------------------------------------------------------------------------

#endif /* GameConstants_h */
