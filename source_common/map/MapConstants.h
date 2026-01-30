///------------------------------------------------------------------------------------------------
///  MapConstants.h
///  TinyMMOClient
///                                                                                                
///  Created by Alex Koukoulas on 14/05/2024
///------------------------------------------------------------------------------------------------

#ifndef MapConstants_h
#define MapConstants_h

///------------------------------------------------------------------------------------------------

namespace map_constants
{

///------------------------------------------------------------------------------------------------

enum class LayerType
{
    BOTTOM_LAYER = 0,
    TOP_LAYER = 1,
    NAVMAP = 2,
    LAYER_COUNT = 3
};

///------------------------------------------------------------------------------------------------

inline const strutils::StringId NO_MAP_CONNECTION_NAME = strutils::StringId("None");

///------------------------------------------------------------------------------------------------

inline const float MAP_RENDERING_SEAMS_BIAS = 0.001f;
inline const float TILE_BOTTOM_LAYER_Z = 0.1f;
inline const float TILE_TOP_LAYER_Z = 10.0f;
inline const float TILE_NAVMAP_LAYER_Z = 20.0f;

///------------------------------------------------------------------------------------------------

inline constexpr int CLIENT_NAVMAP_IMAGE_SIZE = 128;
inline constexpr int CLIENT_WORLD_MAP_IMAGE_SIZE = 1024;

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------

#endif /* MapConstants_h */
