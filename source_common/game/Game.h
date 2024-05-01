///------------------------------------------------------------------------------------------------
///  Game.h                                                                                          
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#ifndef Game_h
#define Game_h

///------------------------------------------------------------------------------------------------

#include <atomic>
#include <memory>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <game/events/EventSystem.h>
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>
#include <nlohmann/json.hpp>

///------------------------------------------------------------------------------------------------


class AnimatedButton;
class Game final
{
public:
    Game(const int argc, char** argv);
    ~Game();
    
    void Init();
    void Update(const float dtMillis);
    void ApplicationMovedToBackground();
    void WindowResize();
    void OnOneSecondElapsed();
    void CreateDebugWidgets();
    
private:
    void SendNetworkMessage(const nlohmann::json& message, const networking::MessageType messageType, const bool highPriority);
    void CreateWorldObject(const networking::WorldObjectData& worldObjectData);
    void InterpolateLocalWorld(const float dtMillis);
    void CheckForStateSending(const float dtMillis);
    void OnServerResponse(const std::string& response);
    void OnServerPlayerStateResponse(const nlohmann::json& responseJson);
    void OnServerLoginResponse(const nlohmann::json& responseJson);
    void OnPlayButtonPressed();
    
private:
    std::atomic<int> mLastPingMillis = 0;
    std::unique_ptr<AnimatedButton> mPlayButton;
    std::vector<int> mWorldObjectIDsToCleanup;
    std::vector<networking::WorldObjectData> mWorldObjectData;
    std::vector<networking::WorldObjectData> mPendingWorldObjectDataToCreate;
    float mStateSendingDelayMillis;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
