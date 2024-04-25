///------------------------------------------------------------------------------------------------
///  Game.h                                                                                          
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#ifndef Game_h
#define Game_h

///------------------------------------------------------------------------------------------------

#include <memory>
#include <game/events/EventSystem.h>

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
};

///------------------------------------------------------------------------------------------------

#endif /* Game_h */
