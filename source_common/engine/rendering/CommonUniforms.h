///------------------------------------------------------------------------------------------------
///  CommonUniforms.h
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/03/2025.
///-----------------------------------------------------------------------------------------------

#ifndef CommonUniforms_h
#define CommonUniforms_h

///-----------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>

///-----------------------------------------------------------------------------------------------

inline const strutils::StringId WORLD_MATRIX_UNIFORM_NAME = strutils::StringId("world");
inline const strutils::StringId VIEW_MATRIX_UNIFORM_NAME  = strutils::StringId("view");
inline const strutils::StringId PROJ_MATRIX_UNIFORM_NAME  = strutils::StringId("proj");
inline const strutils::StringId ROT_MATRIX_UNIFORM_NAME  = strutils::StringId("rot");
inline const strutils::StringId TIME_UNIFORM_NAME  = strutils::StringId("time");
inline const strutils::StringId MIN_U_UNIFORM_NAME = strutils::StringId("min_u");
inline const strutils::StringId MIN_V_UNIFORM_NAME = strutils::StringId("min_v");
inline const strutils::StringId MAX_U_UNIFORM_NAME = strutils::StringId("max_u");
inline const strutils::StringId MAX_V_UNIFORM_NAME = strutils::StringId("max_v");
inline const strutils::StringId ACTIVE_LIGHT_COUNT_UNIFORM_NAME = strutils::StringId("active_light_count");
inline const strutils::StringId AMBIENT_LIGHT_COLOR_UNIFORM_NAME = strutils::StringId("ambient_light_color");
inline const strutils::StringId POINT_LIGHT_COLORS_UNIFORM_NAME = strutils::StringId("point_light_colors");
inline const strutils::StringId POINT_LIGHT_POSITIONS_UNIFORM_NAME = strutils::StringId("point_light_positions");
inline const strutils::StringId POINT_LIGHT_POWERS_UNIFORM_NAME = strutils::StringId("point_light_powers");
inline const strutils::StringId IS_TEXTURE_SHEET_UNIFORM_NAME = strutils::StringId("texture_sheet");
inline const strutils::StringId CUSTOM_ALPHA_UNIFORM_NAME = strutils::StringId("custom_alpha");
inline const strutils::StringId IS_AFFECTED_BY_LIGHT_UNIFORM_NAME = strutils::StringId("affected_by_light");

///-----------------------------------------------------------------------------------------------

#endif /* CommonUniforms_h */
