///------------------------------------------------------------------------------------------------
///  Cards.cpp
///  Predators
///                                                                                                
///  Created by Alex Koukoulas on 10/10/2023
///------------------------------------------------------------------------------------------------

#include <algorithm>
#include <engine/CoreSystemsEngine.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/resloading/DataFileResource.h>
#include <engine/resloading/TextureResource.h>
#include <engine/utils/BaseDataFileDeserializer.h>
#include <engine/utils/OSMessageBox.h>
#include <game/Cards.h>
#include <game/GameConstants.h>
#include <game/GameSymbolicGlyphNames.h>
#include <game/DataRepository.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------

static const std::vector<strutils::StringId> FRESH_ACCOUNT_UNLOCKED_CARD_NAMES =
{
    // All family story starting cards
    strutils::StringId("Stegosaurus"), strutils::StringId("Baby Dino"), strutils::StringId("Dilophosaurus"), strutils::StringId("Brachiosaurus"),
    strutils::StringId("Bunny"), strutils::StringId("Squirrel"), strutils::StringId("Ground Hog"), strutils::StringId("Guinea Pig"),
    strutils::StringId("Fly"), strutils::StringId("Ladybug"), strutils::StringId("Cockroach"), strutils::StringId("Mosquito"),
    
    // Rest of available cards
    strutils::StringId("Dragonfly"),
    strutils::StringId("Toxic Wave"),
    strutils::StringId("Insect Duplication"),
    
    strutils::StringId("Beaver"),
    strutils::StringId("Fluff Attack"),
    strutils::StringId("Bear Trap"),
    strutils::StringId("Gust of Wind"),
    
    strutils::StringId("Metal Claws"),
    strutils::StringId("Mighty Dino Roar"),
    strutils::StringId("Throwing Net"),
    strutils::StringId("Triceratops"),
};

static const std::unordered_map<strutils::StringId, std::vector<strutils::StringId>, strutils::StringIdHasher> FAMILY_STORY_STARTING_CARD_NAMES =
{
    { game_constants::DINOSAURS_FAMILY_NAME, { strutils::StringId("Stegosaurus"), strutils::StringId("Baby Dino"), strutils::StringId("Dilophosaurus"), strutils::StringId("Brachiosaurus") }},
    { game_constants::RODENTS_FAMILY_NAME,   { strutils::StringId("Bunny"), strutils::StringId("Squirrel"), strutils::StringId("Ground Hog"), strutils::StringId("Guinea Pig") }},
    { game_constants::INSECTS_FAMILY_NAME,   { strutils::StringId("Fly"), strutils::StringId("Ladybug"), strutils::StringId("Cockroach"), strutils::StringId("Mosquito") }}
};

///------------------------------------------------------------------------------------------------

CardDataRepository& CardDataRepository::GetInstance()
{
    static CardDataRepository CardDataRepository;
    return CardDataRepository;
}

///------------------------------------------------------------------------------------------------

size_t CardDataRepository::GetCardDataCount() const
{
    return mCardDataMap.size();
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetAllCardIds() const
{
    std::vector<int> resultCardIds;
    for (const auto& [id, data]: mCardDataMap)
    {
        resultCardIds.emplace_back(data.mCardId);
    }
    return resultCardIds;
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetAllNonSpellCardIds() const
{
    std::vector<int> resultCardIds;
    for (const auto& [id, data]: mCardDataMap)
    {
        if (!data.IsSpell())
        {
            resultCardIds.emplace_back(data.mCardId);
        }
    }
    return resultCardIds;
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetCardIdsByFamily(const strutils::StringId& family) const
{
    std::vector<int> resultCardIds;
    for (const auto& [id, data]: mCardDataMap)
    {
        if (data.mCardFamily == family)
        {
            resultCardIds.emplace_back(data.mCardId);
        }
    }
    return resultCardIds;
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetStoryStartingFamilyCards(const strutils::StringId& family) const
{
    return mStoryStartingFamilyCards.at(family);
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetFreshAccountUnlockedCardIds() const
{
    return mFreshAccountUnlockedCardIds;
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetStoryUnlockedCardRewardsPool() const
{
    auto guessedStoryDeckFamilyName = GuessCurrentStoryDeckFamily();
    auto allStoryDeckFamilyCards = GetCardIdsByFamily(guessedStoryDeckFamilyName);
    std::sort(allStoryDeckFamilyCards.begin(), allStoryDeckFamilyCards.end());
    
    auto unlockedCards = DataRepository::GetInstance().GetUnlockedCardIds();
    std::sort(unlockedCards.begin(), unlockedCards.end());
    
    // Find unlocked cards for the current story deck's family
    std::vector<int> familyUnlockedCards;
    std::set_intersection(allStoryDeckFamilyCards.begin(), allStoryDeckFamilyCards.end(), unlockedCards.begin(), unlockedCards.end(), std::back_inserter(familyUnlockedCards));
    
    auto currentStoryDeck = DataRepository::GetInstance().GetCurrentStoryPlayerDeck();
    std::sort(currentStoryDeck.begin(), currentStoryDeck.end());
    
    // Final reward card pool is unlocked family cards minus any card on the current story deck
    std::vector<int> rewardCardPoolWithDeletedCards;
    std::set_difference(familyUnlockedCards.begin(), familyUnlockedCards.end(), currentStoryDeck.begin(), currentStoryDeck.end(), std::back_inserter(rewardCardPoolWithDeletedCards));
    
    // Remove deleted cards
    std::vector<int> finalRewardCardPool;
    auto storyDeletedCards = DataRepository::GetInstance().GetStoryDeletedCardIds();
    std::set_difference(rewardCardPoolWithDeletedCards.begin(), rewardCardPoolWithDeletedCards.end(), storyDeletedCards.begin(), storyDeletedCards.end(), std::back_inserter(finalRewardCardPool));
    
    // Reward card pool is forced to be 3 or more cards by union with existing cards in story deck
    if (finalRewardCardPool.size() < 3)
    {
        for (auto cardId: currentStoryDeck)
        {
            finalRewardCardPool.push_back(cardId);
        }
    };
    
    return finalRewardCardPool;
}

///------------------------------------------------------------------------------------------------

std::vector<int> CardDataRepository::GetCardPackLockedCardRewardsPool() const
{
    std::vector<int> baseCardPool;
    for (const auto& familyCardEntry: mStoryStartingFamilyCards)
    {
        auto allFamilyCards = GetCardIdsByFamily(familyCardEntry.first);
        baseCardPool.insert(baseCardPool.end(), allFamilyCards.begin(), allFamilyCards.end());
    }
    
    std::vector<int> finalCardPool;
    auto unlockedCards = DataRepository::GetInstance().GetUnlockedCardIds();
    
    std::sort(baseCardPool.begin(), baseCardPool.end());
    std::sort(unlockedCards.begin(), unlockedCards.end());
    
    std::set_difference(baseCardPool.begin(), baseCardPool.end(), unlockedCards.begin(), unlockedCards.end(), std::back_inserter(finalCardPool));
    
    return finalCardPool;
}

///------------------------------------------------------------------------------------------------

int CardDataRepository::GetCardId(const strutils::StringId& cardName) const
{
    for (const auto& cardDataMapEntry: mCardDataMap)
    {
        if (cardDataMapEntry.second.mCardName == cardName)
        {
            return cardDataMapEntry.first;
        }
    }
    
    ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, ("Cannot find card with name " + cardName.GetString()).c_str());
    return 0;
}

///------------------------------------------------------------------------------------------------

CardData CardDataRepository::GetCardDataByCardName(const strutils::StringId& cardName, const size_t forPlayerIndex) const
{
    return GetCardData(GetCardId(cardName), forPlayerIndex);
}

///------------------------------------------------------------------------------------------------

CardData CardDataRepository::GetCardData(const int cardId, const size_t forPlayerIndex) const
{
    auto findIter = mCardDataMap.find(cardId);
    if (findIter != mCardDataMap.end())
    {
        CardData cardData = findIter->second;
        
        if (!DataRepository::GetInstance().GetQuickPlayData() && DataRepository::GetInstance().IsCurrentlyPlayingStoryMode())
        {
            if (forPlayerIndex == game_constants::LOCAL_PLAYER_INDEX)
            {
                const auto& storyCardStatModifiers = DataRepository::GetInstance().GetStoryPlayerCardStatModifiers();
                if (storyCardStatModifiers.count(CardStatType::DAMAGE))
                {
                    cardData.mCardDamage += storyCardStatModifiers.at(CardStatType::DAMAGE);
                }
                if (storyCardStatModifiers.count(CardStatType::WEIGHT))
                {
                    cardData.mCardWeight += storyCardStatModifiers.at(CardStatType::WEIGHT);
                }
                
                if (DataRepository::GetInstance().DoesCurrentStoryHaveMutation(game_constants::MUTATION_INCREASED_CARD_WEIGHT))
                {
                    cardData.mCardWeight++;
                }
                
                if (!cardData.IsSpell() && DataRepository::GetInstance().DoesCurrentStoryHaveMutation(game_constants::MUTATION_REDUCED_NORMAL_CARD_DAMAGE))
                {
                    cardData.mCardDamage = math::Max(0, cardData.mCardDamage - 1);
                }
            }
        }
        
        return cardData;
    }
    
    ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, ("Cannot find card with id " + std::to_string(cardId)).c_str());
    return CardData();
}

///------------------------------------------------------------------------------------------------

const std::unordered_set<strutils::StringId, strutils::StringIdHasher>& CardDataRepository::GetCardFamilies() const
{
    return mCardFamilies;
}

///------------------------------------------------------------------------------------------------

const std::unordered_map<strutils::StringId, ExpansionData, strutils::StringIdHasher>& CardDataRepository::GetCardExpansions() const
{
    return mCardExpansions;
}

///------------------------------------------------------------------------------------------------

strutils::StringId CardDataRepository::GuessCurrentStoryDeckFamily() const
{
    auto currentStoryDeck = DataRepository::GetInstance().GetCurrentStoryPlayerDeck();
    std::sort(currentStoryDeck.begin(), currentStoryDeck.end());
    
    for (const auto& cardFamily: mCardFamilies)
    {
        auto allFamilyCards = GetCardIdsByFamily(cardFamily);
        std::sort(allFamilyCards.begin(), allFamilyCards.end());
        
        std::vector<int> intersection;
        std::set_intersection(currentStoryDeck.begin(), currentStoryDeck.end(), allFamilyCards.begin(), allFamilyCards.end(), std::back_inserter(intersection));
        
        if (!intersection.empty())
        {
            return cardFamily;
        }
    }
    
    assert(false);
    return game_constants::RODENTS_FAMILY_NAME;
}

///------------------------------------------------------------------------------------------------

void CardDataRepository::CleanDeckFromTempIds(std::vector<int>& deck)
{
    for (auto iter = deck.begin(); iter != deck.end();)
    {
        if (!mCardDataMap.count(*iter))
        {
            iter = deck.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

///------------------------------------------------------------------------------------------------

void CardDataRepository::ClearCardData()
{
    mCardFamilies.clear();
    mCardDataMap.clear();
}

///------------------------------------------------------------------------------------------------

void CardDataRepository::LoadCardData(bool loadCardAssets)
{
    auto& resourceService = CoreSystemsEngine::GetInstance().GetResourceLoadingService();
    auto cardsDefinitionJsonResourceId = resourceService.LoadResource(resources::ResourceLoadingService::RES_DATA_ROOT + "card_data.json");
    const auto cardDataJson =  nlohmann::json::parse(resourceService.GetResource<resources::DataFileResource>(cardsDefinitionJsonResourceId).GetContents());
    for (const auto& cardFamily: cardDataJson["card_families"])
    {
        mCardFamilies.insert(strutils::StringId(cardFamily.get<std::string>()));
    }
    
    for (const auto& cardExpansionObject: cardDataJson["expansions"])
    {
        ExpansionData expansionData;
        expansionData.mExpansionId = strutils::StringId(cardExpansionObject["id"].get<std::string>());
        expansionData.mExpansionName = cardExpansionObject["name"].get<std::string>();
        
        mCardExpansions[expansionData.mExpansionId] = std::move(expansionData);
    }
    
    std::unordered_set<int> cardIdsSeenThisLoad;
    bool freshCardLoad = mCardDataMap.empty();
    
    for (const auto& cardObject: cardDataJson["card_data"])
    {
        CardData cardData = {};
        
        if (freshCardLoad)
        {
            cardData.mCardId = static_cast<int>(mCardDataMap.size());
        }
        else
        {
            cardData.mCardId = GetCardId(strutils::StringId(cardObject["name"].get<std::string>()));
        }
        
        cardData.mCardWeight = cardObject["weight"].get<int>();
        
        assert(cardIdsSeenThisLoad.count(cardData.mCardId) == 0);
        
        // Normal card
        if (cardObject.count("damage"))
        {
            cardData.mCardDamage = cardObject["damage"].get<int>();
        }
        // Spell card
        else
        {
            cardData.mCardEffect = cardObject["effect"].get<std::string>();
            cardData.mCardEffectTooltip = cardObject["tooltip"].get<std::string>();
            
            // preprocess effect
            for (const auto& symbolicNameEntry: symbolic_glyph_names::SYMBOLIC_NAMES)
            {
                strutils::StringReplaceAllOccurences("<" + symbolicNameEntry.first.GetString() + ">", std::string(1, symbolicNameEntry.second), cardData.mCardEffectTooltip);
            }
            
            assert(strutils::StringSplit(cardData.mCardEffectTooltip, '$').size() <= game_constants::CARD_TOOLTIP_TEXT_ROWS_COUNT);
        }
        
        // Optional particle effect
        if (cardObject.count("particle_effect"))
        {
            cardData.mParticleEffect = strutils::StringId(cardObject["particle_effect"].get<std::string>());
        }
        
        // Optional single use
        cardData.mIsSingleUse = false;
        if (cardObject.count("single_use"))
        {
            cardData.mIsSingleUse = cardObject["single_use"].get<bool>();
        }
        
        // Shake strength on particle step
        if (cardObject.count("particle_shake_strength"))
        {
            cardData.mParticleShakeStrength = cardObject["particle_shake_strength"].get<float>();
        }
        
        // Shake seconds duration on particle step
        if (cardObject.count("particle_shake_duration"))
        {
            cardData.mParticleShakeDurationSecs = cardObject["particle_shake_duration"].get<float>();
        }
    
        cardData.mCardName = strutils::StringId(cardObject["name"].get<std::string>());
        
        // Make sure card has a registered card family
        cardData.mCardFamily = strutils::StringId(cardObject["family"].get<std::string>());
        if (cardData.mCardFamily != game_constants::DEMONS_GENERIC_FAMILY_NAME && cardData.mCardName != game_constants::EMPTY_DECK_TOKEN_CARD_NAME && !mCardFamilies.count(cardData.mCardFamily))
        {
            ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, ("Cannot find family \"" + cardData.mCardFamily.GetString() + "\" for card with id=" + std::to_string(cardData.mCardId)).c_str());
        }
        
        // Make sure card has a registered card expansion
        cardData.mExpansion = strutils::StringId(cardObject["expansion"].get<std::string>());
        if (!mCardExpansions.count(cardData.mExpansion))
        {
            ospopups::ShowMessageBox(ospopups::MessageBoxType::ERROR, ("Cannot find expansion \"" + cardData.mExpansion.GetString() + "\" for card with id=" + std::to_string(cardData.mCardId)).c_str());
        }
        
        if (loadCardAssets)
        {
            cardData.mCardTextureResourceId = resourceService.LoadResource(resources::ResourceLoadingService::RES_TEXTURES_ROOT + cardObject["texture"].get<std::string>());
            cardData.mCardShaderResourceId = resourceService.LoadResource(resources::ResourceLoadingService::RES_SHADERS_ROOT + cardObject["shader"].get<std::string>());
        }
        
        cardIdsSeenThisLoad.insert(cardData.mCardId);
        mCardDataMap[cardData.mCardId] = cardData;
    }
    
    mFreshAccountUnlockedCardIds.clear();
    for (const auto& freshAccountUnlockedCardName: FRESH_ACCOUNT_UNLOCKED_CARD_NAMES)
    {
        mFreshAccountUnlockedCardIds.push_back(GetCardId(freshAccountUnlockedCardName));
    }
    
    mStoryStartingFamilyCards.clear();
    for (const auto& storyStartingFamilyCardEntry: FAMILY_STORY_STARTING_CARD_NAMES)
    {
        for (const auto& cardName: storyStartingFamilyCardEntry.second)
        {
            mStoryStartingFamilyCards[storyStartingFamilyCardEntry.first].push_back(GetCardId(cardName));
        }
    }
}

///------------------------------------------------------------------------------------------------

int CardDataRepository::InsertDynamicCardData(const CardData& cardData)
{
    auto allCardIds = GetAllCardIds();
    std::sort(allCardIds.begin(), allCardIds.end());
    
    auto newCardId = allCardIds.back() + 1;
    mCardDataMap[newCardId] = cardData;
    mCardDataMap[newCardId].mCardId = newCardId;
    
    return newCardId;
}

///------------------------------------------------------------------------------------------------
