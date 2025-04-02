///------------------------------------------------------------------------------------------------
///  BoardTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/02/2025
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/Board.h>
#include <game/SimulationRunner.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/StringUtils.h>
#include <SDL.h>

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestBasicBoardAccessors)
{
    slots::Board b;
    b.SetBoardSymbol(0, 0, slots::SymbolType::CHICKEN);
    ASSERT_EQ(b.GetBoardSymbol(0, 0), slots::SymbolType::CHICKEN);
}

///------------------------------------------------------------------------------------------------



///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestComplexSymbolsNotAppearingInRandomBoardPopulation)
{
    static const long long SIMULATIONS = 100000;

    slots::Board b;
    bool foundComplexSymbol = false;

    simulation::RunGenericSimulation("ComplexSymbolsNotAppearing", SIMULATIONS,
    // Iteration Lambda
    [&](long long){
        b.PopulateBoardForSpin(math::RandomInt());
        
        for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
        {
            if (b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::STRAWBERRY_CAKE) > 0 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::CHOCOLATE_CAKE) > 0 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::ROAST_CHICKEN) > 0 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::CHICKEN_SOUP) > 0)
            {
                foundComplexSymbol = true;
                break;
            }
        }
    },
    // Footer Lambda
    [&](){
        ASSERT_EQ(foundComplexSymbol, false);
    });
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestWildAndScatterAppearOnlyOnceInEachReelInRandomBoardPopulation)
{
    static const long long SIMULATIONS = 100000;
    bool foundDuplicateWildOrScatterInReel = false;
    slots::Board b;
    
    simulation::RunGenericSimulation("Wild/ScatterAppearOnce", SIMULATIONS,
    // Iteration Lambda
    [&](long long){
        b.PopulateBoardForSpin(math::RandomInt());
        
        for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
        {
            if (b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::WILD) > 1 ||
                b.GetSymbolCountInEntireReel(reelIndex, slots::SymbolType::SCATTER) > 1)
            {
                foundDuplicateWildOrScatterInReel = true;
                break;
            }
        }
    },
    // Footer Lambda
    [&](){
        ASSERT_EQ(foundDuplicateWildOrScatterInReel, false);
    });
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestRandomBoardWinStats)
{
    static const long long SIMULATIONS = 1000000;
    simulation::RunStatsSimulation(SIMULATIONS);
}

