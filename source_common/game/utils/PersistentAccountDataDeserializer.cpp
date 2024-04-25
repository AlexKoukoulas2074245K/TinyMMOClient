///------------------------------------------------------------------------------------------------
///  PersistentAccountDataDeserializer.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 05/01/2024
///------------------------------------------------------------------------------------------------

#include <fstream>
#include <game/utils/PersistentAccountDataDeserializer.h>
#include <game/gameactions/GameActionEngine.h>
#include <game/DataRepository.h>
#include <vector>

///------------------------------------------------------------------------------------------------

PersistentAccountDataDeserializer::PersistentAccountDataDeserializer(DataRepository& dataRepository)
    : serial::BaseDataFileDeserializer("persistent", serial::DataFileType::PERSISTENCE_FILE_TYPE, serial::WarnOnFileNotFoundBehavior::DO_NOT_WARN, serial::CheckSumValidationBehavior::VALIDATE_CHECKSUM)
{
    const auto& persistentDataJson = GetState();
    
    if (persistentDataJson.count("currency_coins"))
    {
        auto currency = persistentDataJson["currency_coins"].get<long long>();
        dataRepository.CurrencyCoins().SetDisplayedValue(currency);
        dataRepository.CurrencyCoins().SetValue(currency);
    }
    
    if (persistentDataJson.count("next_card_pack_seed"))
    {
        dataRepository.SetNextCardPackSeed(persistentDataJson["next_card_pack_seed"].get<int>());
    }
    
    if (persistentDataJson.count("games_finished_count"))
    {
        dataRepository.SetGamesFinishedCount(persistentDataJson["games_finished_count"].get<int>());
    }
    
    if (persistentDataJson.count("unlocked_card_ids"))
    {
        dataRepository.SetUnlockedCardIds(persistentDataJson["unlocked_card_ids"].get<std::vector<int>>());
    }
    
    if (persistentDataJson.count("mutation_level_victories"))
    {
        dataRepository.SetAllMutationLevelVictoryCounts(persistentDataJson["mutation_level_victories"].get<std::vector<int>>());
    }
    
    if (persistentDataJson.count("mutation_level_best_times"))
    {
        dataRepository.SetAllMutationLevelBestTimes(persistentDataJson["mutation_level_best_times"].get<std::vector<int>>());
    }
    
    if (persistentDataJson.count("new_card_ids"))
    {
        dataRepository.SetNewCardIds(persistentDataJson["new_card_ids"].get<std::vector<int>>());
    }
    
    if (persistentDataJson.count("seen_opponent_spell_card_ids"))
    {
        dataRepository.SetSeenOpponentSpellCardIds(persistentDataJson["seen_opponent_spell_card_ids"].get<std::vector<int>>());
    }

    if (persistentDataJson.count("seen_tutorials"))
    {
        std::vector<strutils::StringId> seenTutorials;
        
        if (!persistentDataJson["seen_tutorials"].is_null())
        {
            for (auto entryIter = persistentDataJson["seen_tutorials"].begin(); entryIter != persistentDataJson["seen_tutorials"].end(); ++entryIter)
            {
                auto tutorial = strutils::StringId(entryIter.value());
                if (std::find(seenTutorials.cbegin(), seenTutorials.cend(), tutorial) == seenTutorials.cend())
                {
                    seenTutorials.push_back(tutorial);
                }
            }
        }
        
        dataRepository.SetSeenTutorials(seenTutorials);
    }
    
    if (persistentDataJson.count("unlocked_achievements"))
    {
        std::vector<strutils::StringId> unlockedAchievements;
        
        if (!persistentDataJson["unlocked_achievements"].is_null())
        {
            for (auto entryIter = persistentDataJson["unlocked_achievements"].begin(); entryIter != persistentDataJson["unlocked_achievements"].end(); ++entryIter)
            {
                auto achievement = strutils::StringId(entryIter.value());
                if (std::find(unlockedAchievements.cbegin(), unlockedAchievements.cend(), achievement) == unlockedAchievements.cend())
                {
                    unlockedAchievements.push_back(achievement);
                }
            }
        }
        
        dataRepository.SetUnlockedAchievements(unlockedAchievements);
    }
    
    
    if (persistentDataJson.count("successful_transaction_ids"))
    {
        dataRepository.SetSuccessfulTransactionIds(persistentDataJson["successful_transaction_ids"].get<std::vector<std::string>>());
    }
    
    if (persistentDataJson.count("gift_codes_claimed"))
    {
        dataRepository.SetGiftCodesClaimed(persistentDataJson["gift_codes_claimed"].get<std::vector<std::string>>());
    }
    
    if (persistentDataJson.count("audio_enabled"))
    {
        dataRepository.SetAudioEnabled(persistentDataJson["audio_enabled"].get<bool>());
    }
    
    if (persistentDataJson.count("tutorials_enabled"))
    {
        dataRepository.SetTutorialsEnabled(persistentDataJson["tutorials_enabled"].get<bool>());
    }
    
    if (persistentDataJson.count("gold_carts_ignored"))
    {
        dataRepository.SetGoldCartsIgnored(persistentDataJson["gold_carts_ignored"].get<int>());
    }
    
    if (persistentDataJson.count("total_seconds_played"))
    {
        dataRepository.SetTotalSecondsPlayed(persistentDataJson["total_seconds_played"].get<int>());
    }
    
    if (persistentDataJson.count("has_seen_mountain_of_gold_event"))
    {
        dataRepository.SetHasSeenMountainOfGoldEvent(persistentDataJson["has_seen_mountain_of_gold_event"].get<bool>());
    }
    
    if (persistentDataJson.count("golden_card_id_map"))
    {
        dataRepository.ClearGoldenCardIdMap();
        
        if (!persistentDataJson["golden_card_id_map"].is_null())
        {
            for (auto entryIter = persistentDataJson["golden_card_id_map"].begin(); entryIter != persistentDataJson["golden_card_id_map"].end(); ++entryIter)
            {
                dataRepository.SetGoldenCardMapEntry(std::stoi(entryIter.key()), entryIter.value().get<bool>());
            }
        }
    }
    
    if (persistentDataJson.count("pending_card_packs"))
    {
        while (!dataRepository.GetPendingCardPacks().empty())
        {
            dataRepository.PopFrontPendingCardPack();
        }
        
        if (!persistentDataJson["pending_card_packs"].is_null())
        {
            for (auto entryIter = persistentDataJson["pending_card_packs"].begin(); entryIter != persistentDataJson["pending_card_packs"].end(); ++entryIter)
            {
                dataRepository.AddPendingCardPack(static_cast<CardPackType>(std::stoi(entryIter.value().get<std::string>())));
            }
        }
    }
}

///------------------------------------------------------------------------------------------------
