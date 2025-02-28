///------------------------------------------------------------------------------------------------
///  BoardTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 27/02/2025
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/Board.h>

///------------------------------------------------------------------------------------------------

TEST(BoardTest, TestBasicBoardAccessors)
{
    slots::Board b;
    b.SetBoardSymbol(0, 0, slots::SymbolType::CHICKEN);
    ASSERT_EQ(b.GetBoardSymbol(0, 0), slots::SymbolType::CHICKEN);
}

///------------------------------------------------------------------------------------------------
