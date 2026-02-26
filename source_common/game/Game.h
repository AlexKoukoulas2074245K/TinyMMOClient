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
#include <net_common/NetworkCommon.h>
#include <game/events/EventSystem.h>
#include <vector>

///------------------------------------------------------------------------------------------------

namespace scene
{
    struct SceneObject;
}

namespace network
{
    class Navmap;
}

class ObjectAnimationController;
class AnimatedButton;
class CastBarController;
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
    void CreateObject(const network::ObjectData& objectData);
    void DestroyObject(const network::objectId_t objectId);
    void CreateMapSceneObjects(const strutils::StringId& mapName);
    void CreateDebugWidgets();
    
private:
    void ShowDebugNavmap();
    void HideDebugNavmap();
    
private:
    struct LocalObjectWrapper
    {
        // Anim(equipment) layers
        network::ObjectData mObjectData;
        std::vector<std::shared_ptr<scene::SceneObject>> mSceneObjects;
    };

private:
    network::objectId_t mLocalPlayerId;
    std::unique_ptr<AnimatedButton> mTestButton;
    std::unique_ptr<CastBarController> mCastBarController;
    std::unique_ptr<ObjectAnimationController> mObjectAnimationController;
    std::unique_ptr<events::IListener> mMapChangeEventListener;
    std::unique_ptr<events::IListener> mMapSupersessionEventListener;
    std::unique_ptr<events::IListener> mMapResourcesReadyEventListener;
    std::unique_ptr<MapResourceController> mMapResourceController;
    std::shared_ptr<network::Navmap> mCurrentNavmap;
    strutils::StringId mCurrentMap;
    std::unordered_map<network::objectId_t, LocalObjectWrapper> mLocalObjectWrappers;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
