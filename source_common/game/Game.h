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

class ObjectAnimationController;
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
    void CreateObject(const network::ObjectData& objectData);
    void DestroyObject(const network::objectId_t objectId);
    void CreateDebugWidgets();
    
private:
    network::objectId_t mLocalPlayerId;
    std::unique_ptr<AnimatedButton> mTestButton;
    std::unique_ptr<ObjectAnimationController> mObjectAnimationController;
    std::unordered_map<network::objectId_t, network::ObjectData> mLocalObjectDataMap;
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
