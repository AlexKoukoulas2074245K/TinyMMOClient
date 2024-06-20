///------------------------------------------------------------------------------------------------
///  Events.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/11/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef Events_h
#define Events_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <nlohmann/json.hpp>
#include <net_common/NetworkMessages.h>

///------------------------------------------------------------------------------------------------

namespace events
{

///------------------------------------------------------------------------------------------------

class DummyEvent
{
    
};

///------------------------------------------------------------------------------------------------

class SendNetworkMessageEvent
{
public:
    SendNetworkMessageEvent(const nlohmann::json& messageJson, const networking::MessageType messageType, const networking::MessagePriority messagePriority)
        : mMessageJson(messageJson)
        , mMessageType(messageType)
        , mMessagePriority(messagePriority)
    {
    }
    
    const nlohmann::json mMessageJson;
    const networking::MessageType mMessageType;
    const networking::MessagePriority mMessagePriority;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* Events_h */
