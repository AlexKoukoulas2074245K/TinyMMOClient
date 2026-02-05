///------------------------------------------------------------------------------------------------
///  GameCommon.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 05/02/2026
///------------------------------------------------------------------------------------------------

#ifndef GameCommon_h
#define GameCommon_h

///------------------------------------------------------------------------------------------------

#include <net_common/NetworkCommon.h>
#include <engine/utils/StringUtils.h>

///------------------------------------------------------------------------------------------------

inline std::string GetSceneObjectName(network::objectId_t objectId) { return "object-" + std::to_string(objectId); }
inline strutils::StringId GetSceneObjectNameId(network::objectId_t objectId) { return strutils::StringId("object-" + std::to_string(objectId)); }

///------------------------------------------------------------------------------------------------

#endif /* GameCommon_h */
