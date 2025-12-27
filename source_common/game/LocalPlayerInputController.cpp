///------------------------------------------------------------------------------------------------
///  LocalPlayerInputController.cpp
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex  Koukoulas on 27/12/2025
///------------------------------------------------------------------------------------------------

#include <game/LocalPlayerInputController.h>
#include <engine/CoreSystemsEngine.h>
#include <engine/input/IInputStateManager.h>

///------------------------------------------------------------------------------------------------

glm::vec2 LocalPlayerInputController::GetMovementDirection()
{
    const auto& ism = CoreSystemsEngine::GetInstance().GetInputStateManager();
    glm::vec2 direction = { 0.0f, 0.0f };
    
    if (ism.VKeyPressed(input::Key::W))
    {
        direction.y = 1.0f;
    }
    
    if (ism.VKeyPressed(input::Key::D))
    {
        direction.x = 1.0f;
    }
    
    if (ism.VKeyPressed(input::Key::S))
    {
        direction.y = -1.0f;
    }
    
    if (ism.VKeyPressed(input::Key::A))
    {
        direction.x = -1.0f;
    }
    
    if (glm::length(direction) > 0.0f)
    {
        return glm::normalize(direction);
    }
    
    return direction;
}
