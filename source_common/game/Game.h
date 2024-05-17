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

namespace scene
{
    struct SceneObject;
}

class AnimatedButton;
class PlayerController;
class MapResourceController;
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
    void CheckForPendingWorldObjectsToBeCreated();
    void InterpolateLocalWorld(const float dtMillis);
    void CheckForStateSending(const float dtMillis);
    void UpdateCamera(const float dtMillis);
    void SendNetworkMessage(const nlohmann::json& message, const networking::MessageType messageType, const bool highPriority);
    void CreateWorldObject(const networking::WorldObjectData& worldObjectData);
    void OnServerResponse(const std::string& response);
    void OnServerPlayerStateResponse(const nlohmann::json& responseJson);
    void OnServerLoginResponse(const nlohmann::json& responseJson);
    void OnPlayButtonPressed();
    void OnMapChange(const events::MapChangeEvent& mapChangeEvent, const bool shouldLoadNeighbourMapResourcesAsync);
    void CreateMapSceneObjects(const strutils::StringId& mapName);
    
private:
    std::atomic<int> mLastPingMillis = 0;
    std::unique_ptr<MapResourceController> mMapResourceController;
    std::unique_ptr<PlayerController> mPlayerController;
    std::unique_ptr<AnimatedButton> mPlayButton;
    std::unique_ptr<events::IListener> mSendNetworkMessageEventListener;
    std::unique_ptr<events::IListener> mMapChangeEventListener;
    std::unique_ptr<events::IListener> mMapSupersessionEventListener;
    std::unique_ptr<events::IListener> mMapResourcesReadyEventListener;
    std::shared_ptr<scene::SceneObject> mLocalPlayerSceneObject;
    std::vector<int> mWorldObjectIDsToCleanup;
    std::vector<networking::WorldObjectData> mWorldObjectData;
    std::vector<networking::WorldObjectData> mPendingWorldObjectDataToCreate;
    
    float mStateSendingDelayMillis;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
