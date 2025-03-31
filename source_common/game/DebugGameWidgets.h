///------------------------------------------------------------------------------------------------
///  DebugGameWidgets.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 31/03/2023
///------------------------------------------------------------------------------------------------

#ifndef DebugGameWidgets_h
#define DebugGameWidgets_h

///------------------------------------------------------------------------------------------------

class Game;
class DebugGameWidgets final
{
public:
    static void CreateDebugWidgets(Game& game);

private:
    DebugGameWidgets() = delete;
};

///------------------------------------------------------------------------------------------------

#endif /* DebugGameWidgets_h */
