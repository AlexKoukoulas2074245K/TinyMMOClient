///------------------------------------------------------------------------------------------------
///  GiftingUtils.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 06/02/2024                                                       
///------------------------------------------------------------------------------------------------

#include <game/utils/GiftingUtils.h>
#include <game/DataRepository.h>
#include <game/ProductRepository.h>
#include <nlohmann/json.hpp>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace gift_utils
{

///------------------------------------------------------------------------------------------------

static std::string Base64Decode(const std::string &in)
{
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

    int val=0, valb=-8;
    for (unsigned char c : in)
    {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char((val>>valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

///------------------------------------------------------------------------------------------------

void ClaimGiftCode(const std::string& giftCodeString, strutils::StringId& resultProductName)
{
    if (giftCodeString.size() < 10)
    {
        DataRepository::GetInstance().SetCurrentGiftCodeClaimedResultType(GiftCodeClaimedResultType::FAILURE_INVALID_CODE);
        return;
    }
    
    const auto& base64Encoded = giftCodeString.substr(10); // Remove the random part
    const auto& decoded = Base64Decode(base64Encoded);
    
    nlohmann::json json = nlohmann::json::parse(decoded, nullptr, false);
    if (json.is_discarded())
    {
        DataRepository::GetInstance().SetCurrentGiftCodeClaimedResultType(GiftCodeClaimedResultType::FAILURE_INVALID_CODE);
        return;
    }
    
    if (json.count("gift") == 0)
    {
        DataRepository::GetInstance().SetCurrentGiftCodeClaimedResultType(GiftCodeClaimedResultType::FAILURE_INVALID_CODE);
        return;
    }
    auto productName = strutils::StringId(json.at("gift").get<std::string>());
    
    const auto& productDefinitions = ProductRepository::GetInstance().GetProductDefinitions();
    if (productDefinitions.count(productName) == 0)
    {
        DataRepository::GetInstance().SetCurrentGiftCodeClaimedResultType(GiftCodeClaimedResultType::FAILURE_INVALID_PRODUCT);
        return;
    }
    
    const auto& jsonString = json.dump(4);
    auto alreadyClaimedCodes = DataRepository::GetInstance().GetGiftCodesClaimed();
    if (std::find(alreadyClaimedCodes.cbegin(), alreadyClaimedCodes.cend(), jsonString) != alreadyClaimedCodes.cend())
    {
        DataRepository::GetInstance().SetCurrentGiftCodeClaimedResultType(GiftCodeClaimedResultType::FAILURE_USED_ALREADY);
        return;
    }
    
    alreadyClaimedCodes.push_back(jsonString);
    DataRepository::GetInstance().SetGiftCodesClaimed(alreadyClaimedCodes);
    DataRepository::GetInstance().SetCurrentGiftCodeClaimedResultType(GiftCodeClaimedResultType::SUCCESS);
    
    resultProductName = productName;
}

///------------------------------------------------------------------------------------------------

}
