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

class MapChangeEvent
{
public:
    MapChangeEvent(const strutils::StringId& newMapName)
        : mNewMapName(newMapName)
    {
    }
    
    const strutils::StringId mNewMapName;
};

///------------------------------------------------------------------------------------------------

class MapSupersessionEvent
{
public:
    MapSupersessionEvent(const strutils::StringId& supersededMapName)
        : mSupersededMapName(supersededMapName)
    {
    }
    
    const strutils::StringId mSupersededMapName;
};

///------------------------------------------------------------------------------------------------

class MapResourcesReadyEvent
{
public:
    MapResourcesReadyEvent(const strutils::StringId& mapName)
        : mMapName(mapName)
    {
    }
    
    const strutils::StringId mMapName;
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
