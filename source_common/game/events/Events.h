///------------------------------------------------------------------------------------------------
///  Events.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 02/11/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef Events_h
#define Events_h

///------------------------------------------------------------------------------------------------

#include <net_common/NetworkMessages.h>
#include <nlohmann/json.hpp>

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
    SendNetworkMessageEvent(const nlohmann::json& messageJson, const networking::MessageType messageType, const bool isHighPriority)
        : mMessageJson(messageJson)
        , mMessageType(messageType)
        , mIsHighPriority(isHighPriority)
    {
    }
    
    const nlohmann::json& mMessageJson;
    const networking::MessageType mMessageType;
    const bool mIsHighPriority;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* Events_h */
