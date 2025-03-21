///------------------------------------------------------------------------------------------------
///  BoardTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/02/2025
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/Board.h>
#include <engine/utils/MathUtils.h>

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestBasicBoardAccessors)
{
    slots::Board b;
    b.SetBoardSymbol(0, 0, slots::SymbolType::CHICKEN);
    ASSERT_EQ(b.GetBoardSymbol(0, 0), slots::SymbolType::CHICKEN);
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestComplexSymbolsNotAppearingInRandomBoardPopulation)
{
    slots::Board b;
    
    bool foundComplexSymbol = false;
    for (int i = 0; i < 1000000 && !foundComplexSymbol; ++i)
    {
        b.PopulateBoardForSpin(math::RandomInt());
        
        for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
        {
            if (b.GetSymbolCountInReel(reelIndex, slots::SymbolType::STRAWBERRY_CAKE) > 0 ||
                b.GetSymbolCountInReel(reelIndex, slots::SymbolType::CHOCOLATE_CAKE) > 0 ||
                b.GetSymbolCountInReel(reelIndex, slots::SymbolType::ROAST_CHICKEN) > 0)
            {
                foundComplexSymbol = true;
                break;
            }
        }
    }
    
    ASSERT_EQ(foundComplexSymbol, false);
}

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestWildAndScatterAppearOnlyOnceInEachReelInRandomBoardPopulation)
{
    slots::Board b;
    
    bool foundDuplicateWildOrScatterInReel = false;
    for (int i = 0; i < 1000000 && !foundDuplicateWildOrScatterInReel; ++i)
    {
        b.PopulateBoardForSpin(math::RandomInt());
        
        for (int reelIndex = 0; reelIndex < slots::BOARD_COLS; ++reelIndex)
        {
            if (b.GetSymbolCountInReel(reelIndex, slots::SymbolType::WILD) > 1 ||
                b.GetSymbolCountInReel(reelIndex, slots::SymbolType::SCATTER) > 1)
            {
                foundDuplicateWildOrScatterInReel = true;
                break;
            }
        }
    }
    
    ASSERT_EQ(foundDuplicateWildOrScatterInReel, false);
}

///------------------------------------------------------------------------------------------------
