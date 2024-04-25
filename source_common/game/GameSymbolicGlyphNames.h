///------------------------------------------------------------------------------------------------
///  GameSymbolicGlyphNames.h
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 03/03/2024
///------------------------------------------------------------------------------------------------

#ifndef GameSymbolicGlyphNames_h
#define GameSymbolicGlyphNames_h

///------------------------------------------------------------------------------------------------

#include <engine/utils/StringUtils.h>
#include <unordered_map>

///------------------------------------------------------------------------------------------------

namespace symbolic_glyph_names
{

inline const strutils::StringId HEALTH("health");
inline const strutils::StringId COIN("coin");
inline const strutils::StringId DAMAGE("damage");
inline const strutils::StringId WEIGHT("weight");
inline const strutils::StringId ARMOR("armor");
inline const strutils::StringId POISON("poison");
inline const strutils::StringId SKULL("skull");

inline const std::unordered_map<strutils::StringId, char, strutils::StringIdHasher> SYMBOLIC_NAMES =
{
    { HEALTH, 42 },
    { COIN, 124 },
    { DAMAGE, 1},
    { WEIGHT, 2},
    { POISON, 3},
    { ARMOR, 4},
    { SKULL, 5}
};

}

///------------------------------------------------------------------------------------------------

#endif /* GameSymbolicGlyphNames_h */
