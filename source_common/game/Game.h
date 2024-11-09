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
#include <engine/utils/ThreadSafeQueue.h>
#include <game/events/EventSystem.h>
#include <net_common/Card.h>
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>
#include <nlohmann/json.hpp>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene
{
    struct SceneObject;
}

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
    void UpdateGUI(const float dtMillis);
    void SendNetworkMessage(const nlohmann::json& message, const networking::MessageType messageType, const networking::MessagePriority messagePriority);
    void OnServerResponse(const std::string& response);
    void OnServerPlayResponse(const nlohmann::json& responseJson);
    void OnServerTableStateResponse(const nlohmann::json& responseJson);
    void OnPlayButtonPressed();
    
private:
    std::atomic<int> mLastPingMillis = 0;
    std::unique_ptr<AnimatedButton> mPlayButton;
    std::unique_ptr<events::IListener> mSendNetworkMessageEventListener;
    std::vector<poker::Card> mHoleCards;
    std::vector<poker::Card> mCommunityCards;
    long long mPlayerId = 0;
    long long mTableId = 0;
    ThreadSafeQueue<std::string> mQueuedServerResponses;
    float mTableStateRequestTimer;
    std::string mRoundStateName;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
