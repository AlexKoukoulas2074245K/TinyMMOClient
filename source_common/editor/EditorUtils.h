///------------------------------------------------------------------------------------------------
///  EditorUtils.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/01/2026.
///-----------------------------------------------------------------------------------------------

#ifndef EditorUtils_h
#define EditorUtils_h

///-----------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/SceneObject.h>
#include <net_common/Navmap.h>

///-----------------------------------------------------------------------------------------------

inline const strutils::StringId TILE_IS_NAVMAP_TILE_UNIFORM_NAME = strutils::StringId("is_navmap_tile");
inline const strutils::StringId TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME = strutils::StringId("navmap_tile_type");
inline const strutils::StringId TILE_NAVMAP_TILE_COLOR_R_UNIFORM_NAME = strutils::StringId("navmap_tile_color_r");
inline const strutils::StringId TILE_NAVMAP_TILE_COLOR_G_UNIFORM_NAME = strutils::StringId("navmap_tile_color_g");
inline const strutils::StringId TILE_NAVMAP_TILE_COLOR_B_UNIFORM_NAME = strutils::StringId("navmap_tile_color_b");
inline const strutils::StringId TILE_NAVMAP_TILE_COLOR_A_UNIFORM_NAME = strutils::StringId("navmap_tile_color_a");

///-----------------------------------------------------------------------------------------------

namespace editor_utils
{
    inline void SetNormalTileUniforms(std::shared_ptr<scene::SceneObject> tile, const glm::ivec2& coords, const float tileUVSize)
    {
        tile->mShaderBoolUniformValues[TILE_IS_NAVMAP_TILE_UNIFORM_NAME] = false;
        tile->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME] = coords.g * tileUVSize;
        tile->mShaderFloatUniformValues[MIN_V_UNIFORM_NAME] = 1.0f - (coords.r + 1) * tileUVSize;
        tile->mShaderFloatUniformValues[MAX_U_UNIFORM_NAME] = (coords.g + 1) * tileUVSize;
        tile->mShaderFloatUniformValues[MAX_V_UNIFORM_NAME] = 1.0f - coords.r * tileUVSize;
    }

    inline glm::ivec2 GetTilesetCoords(std::shared_ptr<scene::SceneObject> tile, const float tileUVSize)
    {
        assert(tile->mShaderFloatUniformValues.contains(MIN_U_UNIFORM_NAME));
        assert(tile->mShaderFloatUniformValues.contains(MAX_V_UNIFORM_NAME));

        return glm::ivec2(
            static_cast<int>((1.0f - tile->mShaderFloatUniformValues[MAX_V_UNIFORM_NAME])/tileUVSize),
            static_cast<int>(tile->mShaderFloatUniformValues[MIN_U_UNIFORM_NAME]/tileUVSize));
    }

    inline void SetNavmapTileUniforms(std::shared_ptr<scene::SceneObject> tile)
    {
        auto navmapTileTypeColor = network::GetColorFromNavmapTileType(static_cast<network::NavmapTileType>(tile->mShaderIntUniformValues.at(TILE_NAVMAP_TILE_TYPE_UNIFORM_NAME)));
        tile->mShaderBoolUniformValues[TILE_IS_NAVMAP_TILE_UNIFORM_NAME] = true;
        tile->mShaderFloatUniformValues[TILE_NAVMAP_TILE_COLOR_R_UNIFORM_NAME] = navmapTileTypeColor.r/255.0f;
        tile->mShaderFloatUniformValues[TILE_NAVMAP_TILE_COLOR_G_UNIFORM_NAME] = navmapTileTypeColor.g/255.0f;
        tile->mShaderFloatUniformValues[TILE_NAVMAP_TILE_COLOR_B_UNIFORM_NAME] = navmapTileTypeColor.b/255.0f;
        tile->mShaderFloatUniformValues[TILE_NAVMAP_TILE_COLOR_A_UNIFORM_NAME] = navmapTileTypeColor.a/255.0f;
    }
}

///------------------------------------------------------------------------------------------------

#endif /* EditorUtils_h */
