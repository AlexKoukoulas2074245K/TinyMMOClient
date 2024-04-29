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

///------------------------------------------------------------------------------------------------

struct PlayerData
{
    strutils::StringId mPlayerName;
    glm::vec3 mPlayerPosition;
    glm::vec3 mPlayerVelocity;
    float mColor;
    bool mIsLocal;
    bool mInvalidated;
};

///------------------------------------------------------------------------------------------------

class GameSceneTransitionManager;
class TutorialManager;
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
    void CreatePlayer(const std::string& name, const glm::vec3& position, const glm::vec3& velocity, const float color, const bool isLocal);
    void InterpolateLocalWorld(const float dtMillis);
    void CheckForStateSending(const float dtMillis);
    void OnServerWorldStateUpdate(const std::string& worldStateString);
    
private:
    std::atomic<int> mLastPingMillis = 0;
    std::vector<PlayerData> mPlayerData;
    std::vector<strutils::StringId> mPlayerNamesToCleanup;
    bool mCanSendNetworkMessage;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
