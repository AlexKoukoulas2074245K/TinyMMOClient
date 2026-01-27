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
#include <engine/rendering/CommonUniforms.h>
#include <engine/scene/SceneObject.h>

///-----------------------------------------------------------------------------------------------

namespace editor_utils
{
    inline void SetTilesetUVs(std::shared_ptr<scene::SceneObject> tile, const glm::ivec2& coords, const float tileUVSize)
    {
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
}

///------------------------------------------------------------------------------------------------

#endif /* EditorUtils_h */
