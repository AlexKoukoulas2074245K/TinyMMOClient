///------------------------------------------------------------------------------------------------
///  DemonNameGeneratorTest.cpp
///  Predators
///
///  Created by Alex Koukoulas on 16/12/2023
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <engine/utils/MathUtils.h>
#include <game/utils/DemonNameGenerator.h>

TEST(DemonNameGeneratorTests, StressTest1000seedsX1000gens)
{
    for (int i = 0; i < 1000; ++i)
    {
        math::SetControlSeed(i);
        for (int j = 0; j < 1000; ++j)
        {
            GenerateControlledRandomDemonName();
        }
    }
}
