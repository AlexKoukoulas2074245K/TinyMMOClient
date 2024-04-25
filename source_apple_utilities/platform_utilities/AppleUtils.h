///------------------------------------------------------------------------------------------------
///  AppleUtils.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 16/11/2023.
///-----------------------------------------------------------------------------------------------

#ifndef AppleUtils_h
#define AppleUtils_h

///-----------------------------------------------------------------------------------------------

#include <functional>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

///-----------------------------------------------------------------------------------------------

namespace apple_utils
{

///-----------------------------------------------------------------------------------------------

struct PurchaseResultData
{
    std::string mTransactionId;
    std::string mProductId;
    bool mWasSuccessful;
};

///-----------------------------------------------------------------------------------------------

bool IsConnectedToTheInternet();
std::string GetPersistentDataDirectoryPath();
std::string GetDeviceId();
std::string GetDeviceName();
std::string GetAppVersion();
void SetAssetFolder();
bool HasLoadedProducts();
void LoadStoreProducts(const std::vector<std::string>& productIdsToLoad);
std::string GetProductPrice(const std::string& productId);
void InitiateProductPurchase(const std::string& productId, std::function<void(PurchaseResultData)> onPurchaseFinishedCallback);
void GetMessageBoxTextInput(std::function<void(const std::string&)> inputTextReceivedCallback);
void RequestReview();
void SendPlayMessage();

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------

#endif /* AppleUtils_h */
