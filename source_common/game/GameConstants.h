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
    inline const std::string DEFAULT_FONT_SHADER_NAME = "font.vs";
    inline const std::string DEFAULT_TEXTURE_NAME = "debug/debug_square.png";

    // Fonts
    inline const strutils::StringId DEFAULT_FONT_NAME = strutils::StringId("font");
    
    // Game Constants
    //inline const float PLAYER_RUBBERBAND_SPEED = 0.002f;
    inline const strutils::StringId WORLD_SCENE_NAME = strutils::StringId("world");
    inline const strutils::StringId GUI_SCENE_NAME = strutils::StringId("gui");
    inline const glm::vec3 PLAYER_NAMEPLATE_OFFSET = {0.01f, 0.065f, 0.0f};
    inline const float MAP_RENDERED_SCALE = 4.0f;

    // Networking
}

///------------------------------------------------------------------------------------------------

#endif /* GameConstants_h */
