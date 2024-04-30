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
#include <net_common/SerializableNetworkObjects.h>

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
    void CreatePlayerWorldObject(const networking::PlayerData& playerData);
    void InterpolateLocalWorld(const float dtMillis);
    void CheckForStateSending(const float dtMillis);
    void OnServerWorldStateUpdate(const std::string& worldStateString);
    
private:
    std::atomic<int> mLastPingMillis = 0;
    std::vector<networking::PlayerData> mPlayerData;
    std::vector<strutils::StringId> playerNamesToCleanup;
    bool mCanSendNetworkMessage;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
