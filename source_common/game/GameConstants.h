///------------------------------------------------------------------------------------------------
///  GameConstants.h                                                                                          
///  TinyMMOClient                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 09/10/2023                                                       
///------------------------------------------------------------------------------------------------

#ifndef GameConstants_h
#define GameConstants_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

namespace game_constants
{
    // Resources
    inline const std::string DEFAULT_MESH_NAME = "quad.obj";
    inline const std::string DEFAULT_SHADER_NAME = "basic.vs";
    inline const std::string DEFAULT_TEXTURE_NAME = "debug.png";

    // Fonts
    inline const strutils::StringId DEFAULT_FONT_NAME = strutils::StringId("font");
    
    // Game Constants
    //inline const float PLAYER_RUBBERBAND_SPEED = 0.002f;
    inline const glm::vec3 PLAYER_NAMEPLATE_OFFSET = {0.01f, 0.065f, 0.0f};

    // Networking
    inline const float STATE_SEND_MIN_DELAY_MILLIS = 16.6666f;
    inline const float STATE_SEND_MAX_DELAY_MILLIS = 100.0f;

}

///------------------------------------------------------------------------------------------------

#endif /* GameConstants_h */
