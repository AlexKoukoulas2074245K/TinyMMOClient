///------------------------------------------------------------------------------------------------
///  DemonNameGenerator.cpp                                                                                        
///  Predators                                                                                            
///                                                                                                
///  Created by Alex Koukoulas on 16/12/2023                                                       
///------------------------------------------------------------------------------------------------

#include <engine/utils/MathUtils.h>
#include <game/utils/DemonNameGenerator.h>
#include <vector>

///------------------------------------------------------------------------------------------------

static const std::vector<std::string> NAME_COMPONENTS_1 = {"", "", "", "", "b", "br", "d", "dr", "g", "j", "k", "m", "r", "s", "t", "th", "tr", "v", "x", "z"};
static const std::vector<std::string> NAME_COMPONENTS_2 = {"a", "e", "i", "o", "u", "a", "a", "o", "o"};
static const std::vector<std::string> NAME_COMPONENTS_3 = {"g", "g'dr", "g'th", "gdr", "gg", "gl", "gm", "gr", "gth", "k", "l'g", "lg", "lgr", "llm", "lm", "lr", "lv", "n", "ngr", "nn", "r", "r'", "r'g", "rg", "rgr", "rk", "rn", "rr", "rthr", "rz", "str", "th't", "z", "z'g", "zg", "zr", "zz"};
static const std::vector<std::string> NAME_COMPONENTS_4 = {"a", "e", "i", "o", "u", "a", "a", "o", "o", "a", "e", "i", "o", "u", "a", "a", "o", "o", "a", "e", "i", "o", "u", "a", "a", "o", "o", "a", "e", "i", "o", "u", "a", "a", "o", "o", "a", "e", "i", "o", "u", "a", "a", "o", "o", "iu", "uu", "au", "aa"};
static const std::vector<std::string> NAME_COMPONENTS_5 = {"d", "k", "l", "ll", "m", "m", "m", "n", "n", "n", "nn", "r", "r", "r", "th", "x", "z"};
static const std::vector<std::string> NAME_COMPONENTS_6 = {"ch", "d", "g", "k", "l", "n", "n", "n", "n", "n", "r", "s", "th", "th", "th", "th", "th", "z"};

///------------------------------------------------------------------------------------------------

std::string GenerateControlledRandomDemonName()
{
    const auto nameType = math::ControlledRandomInt(0, 2);
    
    auto randomIndex1 = math::ControlledRandomInt() % NAME_COMPONENTS_1.size();
    auto randomIndex2 = math::ControlledRandomInt() % NAME_COMPONENTS_2.size();
    auto randomIndex3 = math::ControlledRandomInt() % NAME_COMPONENTS_6.size();
    auto randomIndex4 = math::ControlledRandomInt() % NAME_COMPONENTS_3.size();
    auto randomIndex5 = math::ControlledRandomInt() % NAME_COMPONENTS_4.size();
    
    while (NAME_COMPONENTS_3[randomIndex4] == NAME_COMPONENTS_1[randomIndex1] || NAME_COMPONENTS_3[randomIndex4] == NAME_COMPONENTS_6[randomIndex3])
    {
        randomIndex4 = math::ControlledRandomInt() % NAME_COMPONENTS_3.size();
    }
    
    std::string result;
    if (nameType == 0)
    {
        result = NAME_COMPONENTS_1[randomIndex1] +
                 NAME_COMPONENTS_2[randomIndex2] +
                 NAME_COMPONENTS_3[randomIndex4] +
                 NAME_COMPONENTS_4[randomIndex5] +
                 NAME_COMPONENTS_6[randomIndex3];
    }
    else
    {
        auto randomIndex6 = math::ControlledRandomInt() % NAME_COMPONENTS_2.size();
        auto randomIndex7 = math::ControlledRandomInt() % NAME_COMPONENTS_5.size();

        while (NAME_COMPONENTS_5[randomIndex7] == NAME_COMPONENTS_3[randomIndex4] || NAME_COMPONENTS_5[randomIndex7] == NAME_COMPONENTS_6[randomIndex3])
        {
            randomIndex7 = math::ControlledRandomInt() % NAME_COMPONENTS_5.size();
        }

        result = NAME_COMPONENTS_1[randomIndex1] +
                 NAME_COMPONENTS_2[randomIndex2] +
                 NAME_COMPONENTS_3[randomIndex4] +
                 NAME_COMPONENTS_2[randomIndex6] +
                 NAME_COMPONENTS_5[randomIndex7] +
                 NAME_COMPONENTS_4[randomIndex5] +
                 NAME_COMPONENTS_6[randomIndex3];
    }
    
    result[0] -= 32; // Capitalize first character;
    return result;
}

///------------------------------------------------------------------------------------------------
