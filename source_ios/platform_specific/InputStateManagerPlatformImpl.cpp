///------------------------------------------------------------------------------------------------
///  InputStateManagerPlatformImpl.cpp
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/utils/Logging.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <platform_specific/InputStateManagerPlatformImpl.h>

///------------------------------------------------------------------------------------------------

namespace input
{

///------------------------------------------------------------------------------------------------

const glm::vec2& InputStateManagerPlatformImpl::VGetPointingPos() const
{
    return mPointingPos;
}

///------------------------------------------------------------------------------------------------

const glm::ivec2& InputStateManagerPlatformImpl::VGetScrollDelta() const
{
    static glm::ivec2 dummyDelta(0);
    return dummyDelta;
}

///------------------------------------------------------------------------------------------------

glm::vec2 InputStateManagerPlatformImpl::VGetPointingPosInWorldSpace(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) const
{
    const auto& invVP = glm::inverse(projMatrix * viewMatrix);
    const auto& screenPos = glm::vec4(mPointingPos.x, mPointingPos.y, 1.0f, 1.0f);
    const auto& worldPos = invVP * screenPos;
    return glm::vec2(worldPos.x, worldPos.y);
}

///------------------------------------------------------------------------------------------------

bool InputStateManagerPlatformImpl::VIsTouchInputPlatform() const
{
    return true;
}

///------------------------------------------------------------------------------------------------

bool InputStateManagerPlatformImpl::VButtonPressed(const Button button) const
{
    return (mCurrentFrameButtonState & (1 << static_cast<uint8_t>(button))) != 0;
}

///------------------------------------------------------------------------------------------------

bool InputStateManagerPlatformImpl::VButtonTapped(const Button button) const
{
    return VButtonPressed(button) && (mPreviousFrameButtonState & (1 << static_cast<uint8_t>(button))) == 0;
}

///------------------------------------------------------------------------------------------------

void InputStateManagerPlatformImpl::VProcessInputEvent(const SDL_Event& event, bool& shouldQuit, bool& windowSizeChange, bool& applicationMovingToBackground, bool& applicationMovingToForeground)
{
    shouldQuit = false;
    applicationMovingToBackground = false;
    
    //User requests quit
    switch (event.type)
    {
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
        {
            shouldQuit = true;
        } break;
        
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_DIDENTERBACKGROUND:
        {
            applicationMovingToBackground = true;
        } break;
        
        case SDL_APP_WILLENTERFOREGROUND:
        case SDL_APP_DIDENTERFOREGROUND:
        {
            applicationMovingToForeground = true;
        } break;
            
        case SDL_WINDOWEVENT:
        {
            if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                windowSizeChange = true;
            }
        }
        break;
        
        case SDL_FINGERDOWN:
        {
            if (!mCurrentFingerId || event.tfinger.fingerId == mCurrentFingerId)
            {
                mCurrentFingerId = event.tfinger.fingerId;
                mPointingPos = glm::vec2(event.tfinger.x, event.tfinger.y);
                mPointingPos.x = (mPointingPos.x - 0.5f) * 2;
                mPointingPos.y = -(mPointingPos.y - 0.5f) * 2;
                mCurrentFrameButtonState |= (1 << static_cast<uint8_t>(Button::MAIN_BUTTON));
            }
        } break;
            
        case SDL_FINGERUP:
        {
            if (!mCurrentFingerId || event.tfinger.fingerId == mCurrentFingerId)
            {
                mCurrentFingerId = 0;
                mPointingPos = glm::vec2(event.tfinger.x, event.tfinger.y);
                mPointingPos.x = (mPointingPos.x - 0.5f) * 2;
                mPointingPos.y = -(mPointingPos.y - 0.5f) * 2;
                mCurrentFrameButtonState ^= (1 << static_cast<uint8_t>(Button::MAIN_BUTTON));
            }
        } break;
            
        case SDL_FINGERMOTION:
        {
            if (!mCurrentFingerId || event.tfinger.fingerId == mCurrentFingerId)
            {
                mPointingPos = glm::vec2(event.tfinger.x, event.tfinger.y);
                mPointingPos.x = (mPointingPos.x - 0.5f) * 2;
                mPointingPos.y = -(mPointingPos.y - 0.5f) * 2;
            }
        } break;
            
        case SDL_MOUSEWHEEL:
        {
        } break;
    }
}

///------------------------------------------------------------------------------------------------

void InputStateManagerPlatformImpl::VUpdate()
{
    mPreviousFrameButtonState = mCurrentFrameButtonState;
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
