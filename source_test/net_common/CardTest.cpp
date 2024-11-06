///------------------------------------------------------------------------------------------------
///  CardTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 6/11/2024
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/Card.h>

///------------------------------------------------------------------------------------------------

TEST(CardTest, TestBasicCardComparisons)
{
    poker::Card aceOfDiamonds(poker::CardRank::ACE, poker::CardSuit::DIAMOND);
    poker::Card kingOfDiamonds(poker::CardRank::KING, poker::CardSuit::DIAMOND);
    poker::Card aceOfSpades(poker::CardRank::ACE, poker::CardSuit::SPADE);
    
    EXPECT_GT(aceOfDiamonds, kingOfDiamonds);
    EXPECT_EQ(aceOfSpades, aceOfSpades);
    //EXPECT_NE(aceOfSpades, aceOfDiamonds);
}

///------------------------------------------------------------------------------------------------

TEST(CardTest, TestCardStringRepresentations)
{
    poker::Card aceOfDiamonds(poker::CardRank::ACE, poker::CardSuit::DIAMOND);
    poker::Card threeOfSpades(poker::CardRank::THREE, poker::CardSuit::SPADE);
    poker::Card tenOfHearts(poker::CardRank::TEN, poker::CardSuit::HEART);
    poker::Card jackOfClubs(poker::CardRank::JACK, poker::CardSuit::CLUB);
    
    EXPECT_EQ(aceOfDiamonds.ToString(), "AD");
    EXPECT_EQ(threeOfSpades.ToString(), "3S");
    EXPECT_EQ(tenOfHearts.ToString(), "10H");
    EXPECT_EQ(jackOfClubs.ToString(), "JC");
}

///------------------------------------------------------------------------------------------------
