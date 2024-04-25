///------------------------------------------------------------------------------------------------
///  StoryDeserializer.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/01/2024
///------------------------------------------------------------------------------------------------

#include <fstream>
#include <game/utils/StoryDeserializer.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/DataRepository.h>
#include <vector>

///------------------------------------------------------------------------------------------------

StoryDeserializer::StoryDeserializer(DataRepository& dataRepository)
    : serial::BaseDataFileDeserializer("story", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::WarnOnFileNotFoundBehavior::DO_NOT_WARN, serial::CheckSumValidationBehavior::VALIDATE_CHECKSUM)
{
    const auto& storyJson = GetState();
    
    if (storyJson.count("story_player_card_stat_modifiers"))
    {
        dataRepository.ClearStoryPlayerCardStatModifiers();
        
        if (!storyJson["story_player_card_stat_modifiers"].is_null())
        {
            for (auto entryIter = storyJson["story_player_card_stat_modifiers"].begin(); entryIter != storyJson["story_player_card_stat_modifiers"].end(); ++entryIter)
            {
                dataRepository.SetStoryPlayerCardStatModifier(static_cast<CardStatType>(std::stoi(entryIter.key())), entryIter.value().get<int>());
            }
        }
    }
    
    if (storyJson.count("current_story_artifacts"))
    {
        dataRepository.ClearCurrentStoryArtifacts();
        
        if (!storyJson["current_story_artifacts"].is_null())
        {
            for (auto entryIter = storyJson["current_story_artifacts"].begin(); entryIter != storyJson["current_story_artifacts"].end(); ++entryIter)
            {
                for (auto i = 0; i < entryIter.value().get<int>(); ++i)
                {
                    dataRepository.AddStoryArtifact(strutils::StringId(entryIter.key()));
                }
            }
        }
    }
    
    if (storyJson.count("current_story_health"))
    {
        auto storyHealth = storyJson["current_story_health"].get<int>();
        dataRepository.StoryCurrentHealth().SetDisplayedValue(storyHealth);
        dataRepository.StoryCurrentHealth().SetValue(storyHealth);
    }
    
    if (storyJson.count("current_story_player_deck"))
    {
        dataRepository.SetCurrentStoryPlayerDeck(storyJson["current_story_player_deck"].get<std::vector<int>>());
    }
    
    if (storyJson.count("current_story_mutation_level"))
    {
        dataRepository.SetCurrentStoryMutationLevel(storyJson["current_story_mutation_level"].get<int>());
    }
    
    if (storyJson.count("next_top_player_deck"))
    {
        dataRepository.SetNextTopPlayerDeck(storyJson["next_top_player_deck"].get<std::vector<int>>());
    }
    
    if (storyJson.count("next_bot_player_deck"))
    {
        dataRepository.SetNextBotPlayerDeck(storyJson["next_bot_player_deck"].get<std::vector<int>>());
    }
    
    if (storyJson.count("story_deleted_cards") && !storyJson["story_deleted_cards"].is_null())
    {
        dataRepository.SetStoryDeletedCardIds(storyJson["story_deleted_cards"].get<std::vector<int>>());
    }
    
    if (storyJson.count("current_story_map_scene_type"))
    {
        dataRepository.SetCurrentStoryMapSceneType(static_cast<StoryMapSceneType>(storyJson["current_story_map_scene_type"].get<int>()));
    }
    
    if (storyJson.count("current_story_map_type"))
    {
        dataRepository.SetCurrentStoryMapType(static_cast<StoryMapType>(storyJson["current_story_map_type"].get<int>()));
    }
    
    if (storyJson.count("current_wheel_of_fortune_type"))
    {
        dataRepository.SetCurrentWheelOfFortuneType(static_cast<WheelOfFortuneType>(storyJson["current_wheel_of_fortune_type"].get<int>()));
    }
    
    if (storyJson.count("current_shop_type"))
    {
        dataRepository.SetCurrentShopBehaviorType(static_cast<ShopBehaviorType>(storyJson["current_shop_type"].get<int>()));
    }
    
    if (storyJson.count("current_event_screen"))
    {
        dataRepository.SetCurrentEventScreenIndex(storyJson["current_event_screen"].get<int>());
    }
    
    if (storyJson.count("current_event"))
    {
        dataRepository.SetCurrentEventIndex(storyJson["current_event"].get<int>());
    }
    
    if (storyJson.count("story_max_health"))
    {
        dataRepository.SetStoryMaxHealth(storyJson["story_max_health"].get<int>());
    }
    
    if (storyJson.count("story_starting_gold"))
    {
        dataRepository.SetStoryStartingGold(storyJson["story_starting_gold"].get<int>());
    }
    
    if (storyJson.count("story_seed"))
    {
        dataRepository.SetStoryMapGenerationSeed(storyJson["story_seed"].get<int>());
    }
    
    if (storyJson.count("current_shop_bought_product_coordinates") && !storyJson["current_shop_bought_product_coordinates"].is_null())
    {
        dataRepository.SetShopBoughtProductCoordinates(storyJson["current_shop_bought_product_coordinates"].get<std::vector<std::pair<int, int>>>());
    }
    
    if (storyJson.count("current_story_seconds_played"))
    {
        dataRepository.SetCurrentStorySecondPlayed(storyJson["current_story_seconds_played"].get<int>());
    }
    
    if (storyJson.count("current_story_map_node_seed"))
    {
        dataRepository.SetCurrentStoryMapNodeSeed(storyJson["current_story_map_node_seed"].get<int>());
    }
    
    if (storyJson.count("current_story_map_node_type"))
    {
        dataRepository.SetCurrentStoryMapNodeType(static_cast<StoryMap::NodeType>(storyJson["current_story_map_node_type"].get<int>()));
    }
    
    if (storyJson.count("current_battle_sub_scene_type"))
    {
        dataRepository.SetCurrentBattleSubSceneType(static_cast<BattleSubSceneType>(storyJson["current_battle_sub_scene_type"].get<int>()));
    }
    
    if (storyJson.count("next_battle_top_health"))
    {
        dataRepository.SetNextBattleTopPlayerHealth(storyJson["next_battle_top_health"].get<int>());
    }
    
    if (storyJson.count("next_battle_bot_health"))
    {
        dataRepository.SetNextBattleBotPlayerHealth(storyJson["next_battle_bot_health"].get<int>());
    }
    
    if (storyJson.count("next_battle_top_init_weight"))
    {
        dataRepository.SetNextBattleTopPlayerInitWeight(storyJson["next_battle_top_init_weight"].get<int>());
    }
    
    if (storyJson.count("next_battle_bot_init_weight"))
    {
        dataRepository.SetNextBattleBotPlayerInitWeight(storyJson["next_battle_bot_init_weight"].get<int>());
    }
    
    if (storyJson.count("next_battle_top_weight_limit"))
    {
        dataRepository.SetNextBattleTopPlayerWeightLimit(storyJson["next_battle_top_weight_limit"].get<int>());
    }
    
    if (storyJson.count("next_battle_bot_weight_limit"))
    {
        dataRepository.SetNextBattleBotPlayerWeightLimit(storyJson["next_battle_bot_weight_limit"].get<int>());
    }
    
    if (storyJson.count("next_story_opponent_damage"))
    {
        dataRepository.SetNextStoryOpponentDamage(storyJson["next_story_opponent_damage"].get<int>());
    }
    
    if (storyJson.count("current_story_map_node_coord"))
    {
        dataRepository.SetCurrentStoryMapNodeCoord(glm::ivec2(storyJson["current_story_map_node_coord"]["col"].get<int>(), storyJson["current_story_map_node_coord"]["row"].get<int>()));
    }
    
    if (storyJson.count("pre_boss_mid_map_node_coord"))
    {
        dataRepository.SetPreBossMidMapNodeCoord(glm::ivec2(storyJson["pre_boss_mid_map_node_coord"]["col"].get<int>(), storyJson["pre_boss_mid_map_node_coord"]["row"].get<int>()));
    }
    
    if (storyJson.count("next_story_opponent_path"))
    {
        dataRepository.SetNextStoryOpponentTexturePath(storyJson["next_story_opponent_path"].get<std::string>());
    }
    
    if (storyJson.count("next_story_opponent_name"))
    {
        dataRepository.SetNextStoryOpponentName(storyJson["next_story_opponent_name"].get<std::string>());
    }
}

///------------------------------------------------------------------------------------------------
