///------------------------------------------------------------------------------------------------
///  InputStateManagerPlatformImpl.cpp
///  TinyMMOClient                                                                                            
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
    return mCurrentWheelDelta;
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
    return false;
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
    const auto& renderableDimensions = CoreSystemsEngine::GetInstance().GetContextRenderableDimensions();
    shouldQuit = false;

    //User requests quit
    switch (event.type)
    {
        case SDL_QUIT:
        case SDL_APP_TERMINATING:
        {
            applicationMovingToBackground = true;
            shouldQuit = true;
        } break;
        
        case SDL_WINDOWEVENT:
        {
            if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                windowSizeChange = true;
            }
            else if (event.window.event == SDL_WINDOWEVENT_SHOWN)
            {
                applicationMovingToForeground = true;
            }
            else if (event.window.event == SDL_WINDOWEVENT_HIDDEN)
            {
                applicationMovingToBackground = true;
            }
        }
        break;
        
        case SDL_MOUSEBUTTONDOWN:
        {
            mCurrentFrameButtonState |= (1 << event.button.button);
        } break;
            
        case SDL_MOUSEBUTTONUP:
        {
            mCurrentFrameButtonState ^= (1 << event.button.button);
        } break;
            
        case SDL_MOUSEMOTION:
        {
            mPointingPos = glm::vec2(event.motion.x/renderableDimensions.x, event.motion.y/renderableDimensions.y);
            mPointingPos.x = (mPointingPos.x - 0.5f) * 2;
            mPointingPos.y = -(mPointingPos.y - 0.5f) * 2;
        } break;
            
        case SDL_MOUSEWHEEL:
        {
            mCurrentWheelDelta = glm::ivec2(event.wheel.x, event.wheel.y);
        } break;
    }
    
#if defined(USE_IMGUI)
    ImGui_ImplSDL2_ProcessEvent(&event);
#endif
}

///------------------------------------------------------------------------------------------------

void InputStateManagerPlatformImpl::VUpdate()
{
    mPreviousFrameButtonState = mCurrentFrameButtonState;
    mCurrentWheelDelta = glm::ivec2(0);
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
