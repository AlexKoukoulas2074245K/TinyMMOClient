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
#include <net_common/NetworkMessages.h>
#include <net_common/SerializableNetworkObjects.h>
#include <net_common/Board.h>
#include <nlohmann/json.hpp>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene
{
    struct SceneObject;
}

class AnimatedButton;
class BoardView;
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
    void OnServerError(const std::string& error);
    void OnServerResponse(const std::string& response);
    void OnServerLoginResponse(const nlohmann::json& responseJson);
    void OnServerSpinResponse(const nlohmann::json& responseJson);
    void OnLoginButtonPressed();
    void OnSpinButtonPressed();
    void UpdateCredits(const int wonCreditMultiplier);
private:
    std::atomic<int> mLastPingMillis = 0;
    std::unique_ptr<AnimatedButton> mLoginButton;
    std::unique_ptr<events::IListener> mSendNetworkMessageEventListener;
    std::unique_ptr<BoardView> mBoardView;
    slots::Board mBoardModel;
    long long mPlayerId = 0;
    long long mCredits = 0;
    float mDisplayedCredits = 0.0f;
    int mSpinId = 0;
    ThreadSafeQueue<std::string> mQueuedServerResponses;
    ThreadSafeQueue<std::string> mQueuedServerErrors;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
