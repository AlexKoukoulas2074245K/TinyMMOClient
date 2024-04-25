///------------------------------------------------------------------------------------------------
///  InputStateManagerPlatformImpl.h
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/10/2023
///------------------------------------------------------------------------------------------------

#ifndef InputStateManagerPlatformImpl_h
#define InputStateManagerPlatformImpl_h

///------------------------------------------------------------------------------------------------

#include <cstdint>
#include <engine/input/IInputStateManager.h>
#include <engine/CoreSystemsEngine.h>
#include <vector>


///------------------------------------------------------------------------------------------------

namespace input
{

///------------------------------------------------------------------------------------------------

class InputStateManagerPlatformImpl final: public IInputStateManager
{
    friend struct CoreSystemsEngine::SystemsImpl;
public:
    const glm::vec2& VGetPointingPos() const override;
    const glm::ivec2& VGetScrollDelta() const override;
    glm::vec2 VGetPointingPosInWorldSpace(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) const override;
    
    bool VIsTouchInputPlatform() const override;
    bool VButtonPressed(const Button button) const override;
    bool VButtonTapped(const Button button) const override;
    
    void VProcessInputEvent(const SDL_Event& event, bool& shouldQuit, bool& windowSizeChange, bool& applicationMovingToBackground, bool& applicationMovingToForeground) override;
    void VUpdate() override;
    
private:
    InputStateManagerPlatformImpl() = default;
    
private:
    glm::vec2 mPointingPos;
    glm::ivec2 mCurrentWheelDelta;
    uint8_t mCurrentFrameButtonState = 0U;
    uint8_t mPreviousFrameButtonState = 0U;
};

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* InputStateManagerPlatformImpl_h */
