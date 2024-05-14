///------------------------------------------------------------------------------------------------
///  IInputStateManager.h
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023
///------------------------------------------------------------------------------------------------

#ifndef IInputStateManager_h
#define IInputStateManager_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <SDL_events.h>

///------------------------------------------------------------------------------------------------

namespace input
{

///------------------------------------------------------------------------------------------------

enum class Button
{
    MAIN_BUTTON = 1,
    MIDDLE_BUTTON = 2,
    SECONDARY_BUTTON = 3,
    
    MAX = 8
};

///------------------------------------------------------------------------------------------------

enum class Key
{
    W    = 1,
    A    = 2,
    S    = 3,
    D    = 4,
    Z    = 5,
    LALT = 6,
    RALT = 7,
    LCTL = 8,
    RCTL = 9,
    LCMD = 10,
    RCMD = 11,
    LSFT = 12,
    RSFT = 13,
    
    MAX  = 32
};

///------------------------------------------------------------------------------------------------

class IInputStateManager
{
public:
    virtual ~IInputStateManager() = default;
    
    virtual const glm::vec2& VGetPointingPos() const = 0;
    virtual const glm::ivec2& VGetScrollDelta() const = 0;
    virtual glm::vec2 VGetPointingPosInWorldSpace(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) const = 0;
    
    virtual bool VButtonPressed(const Button button) const = 0;
    virtual bool VButtonTapped(const Button button) const = 0;
    virtual bool VKeyPressed(const Key key) const = 0;
    virtual bool VKeyTapped(const Key key) const = 0;
    virtual bool VIsTouchInputPlatform() const = 0;
    
    virtual void VProcessInputEvent(const SDL_Event& event, bool& shouldQuit, bool& windowSizeChange, bool& applicationMovingToBackground, bool& applicationMovingToForeground) = 0;
    virtual void VUpdate() = 0;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* IInputStateManager_h */
