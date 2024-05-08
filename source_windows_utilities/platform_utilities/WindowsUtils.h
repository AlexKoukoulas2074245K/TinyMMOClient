///------------------------------------------------------------------------------------------------
///  WindowsUtils.h
///  Predators
///
///  Created by Alex Koukoulas on 20/01/2024.
///-----------------------------------------------------------------------------------------------

#ifndef WindowsUtils_h
#define WindowsUtils_h

///-----------------------------------------------------------------------------------------------

#include <string>
#include <nlohmann/json.hpp>
#include <net_common/NetworkMessages.h>

///-----------------------------------------------------------------------------------------------

namespace windows_utils
{

///-----------------------------------------------------------------------------------------------

bool IsConnectedToTheInternet();
std::string GetPersistentDataDirectoryPath();
void SendNetworkMessage(const nlohmann::json& networkMessage, const networking::MessageType messageType, const bool highPriority, std::function<void(const networking::ServerResponseData&)> serverResponseCallback);

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------

#endif /* WindowsUtils_h */
