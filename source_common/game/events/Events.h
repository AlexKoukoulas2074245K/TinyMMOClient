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
#include <net_common/NetworkCommon.h>

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

class ObjectDestroyedEvent
{
public:
    ObjectDestroyedEvent(const strutils::StringId& sceneObjectName)
        : mSceneObjectName(sceneObjectName) {}

    const strutils::StringId mSceneObjectName;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* Events_h */
