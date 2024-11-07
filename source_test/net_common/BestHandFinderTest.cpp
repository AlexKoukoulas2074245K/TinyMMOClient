///------------------------------------------------------------------------------------------------
///  BestHandFinderTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 7/11/2024
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/BestHandFinder.h>

///------------------------------------------------------------------------------------------------

static poker::Card CreateCard(std::string cardString)
{
    auto suit = static_cast<poker::CardSuit>(cardString.back());
    cardString = cardString.substr(0, cardString.size() - 1);
    
    switch (cardString[0])
    {
        case 'J': return poker::Card(poker::CardRank::JACK, suit);
        case 'Q': return poker::Card(poker::CardRank::QUEEN, suit);
        case 'K': return poker::Card(poker::CardRank::KING, suit);
        case 'A': return poker::Card(poker::CardRank::ACE, suit);
        case '1': return poker::Card(poker::CardRank::TEN, suit);
        default: return poker::Card(static_cast<poker::CardRank>(std::stoi(cardString)), suit);
    }
}

static poker::Hand CreateHand(const std::string& handString, poker::HandKind handKind)
{
    auto handStringSplitByComma = strutils::StringSplit(handString, ',');
    std::array<poker::Card, poker::HAND_SIZE> handCards =
    {
        CreateCard(handStringSplitByComma[0]),
        CreateCard(handStringSplitByComma[1]),
        CreateCard(handStringSplitByComma[2]),
        CreateCard(handStringSplitByComma[3]),
        CreateCard(handStringSplitByComma[4])
    };
    
    return poker::Hand(handCards, handKind);
}

static std::array<poker::Card, poker::CARD_POOL_SIZE> CreateCardPool(const std::string& cardPoolString)
{
    auto cardPoolStringSplitByComma = strutils::StringSplit(cardPoolString, ',');
    return
    {
        CreateCard(cardPoolStringSplitByComma[0]),
        CreateCard(cardPoolStringSplitByComma[1]),
        CreateCard(cardPoolStringSplitByComma[2]),
        CreateCard(cardPoolStringSplitByComma[3]),
        CreateCard(cardPoolStringSplitByComma[4]),
        CreateCard(cardPoolStringSplitByComma[5]),
        CreateCard(cardPoolStringSplitByComma[6])
    };
}

static void EXPECT_HAND(const poker::Hand& resultHand, const poker::HandKind expectedHandKind, const std::string& expectedHandString)
{
    auto expectedHand = CreateHand(expectedHandString, expectedHandKind);
    EXPECT_EQ(resultHand.GetHandKind(), expectedHand.GetHandKind());
    
    for (int i = 0; i < poker::HAND_SIZE; ++i)
    {
        EXPECT_EQ(expectedHand.GetHandCards()[i].GetRank(), resultHand.GetHandCards()[i].GetRank());
        EXPECT_EQ(expectedHand.GetHandCards()[i].GetSuit(), resultHand.GetHandCards()[i].GetSuit());
    }
}

static void EXPECT_FIND_BEST_HAND(const std::string& cardPoolString, const poker::HandKind expectedHandKind, const std::string& expectedHandString)
{
    const auto& cardPool = CreateCardPool(cardPoolString);
    EXPECT_HAND(poker::BestHandFinder::FindBestHand(cardPool), expectedHandKind, expectedHandString);
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestRoyalFlushFinding)
{
    EXPECT_FIND_BEST_HAND("2H,10S,JS,QS,AS,KS,6H", poker::HandKind::ROYAL_FLUSH, "10S,JS,QS,KS,AS");
    EXPECT_FIND_BEST_HAND("2H,10C,JC,QC,AC,KC,6H", poker::HandKind::ROYAL_FLUSH, "10C,JC,QC,KC,AC");
    EXPECT_FIND_BEST_HAND("2H,10H,JH,QH,AH,KH,6H", poker::HandKind::ROYAL_FLUSH, "10H,JH,QH,KH,AH");
    EXPECT_FIND_BEST_HAND("2H,10D,JD,QD,AD,KD,6H", poker::HandKind::ROYAL_FLUSH, "10D,JD,QD,KD,AD");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestStraightFlushFinding)
{
    EXPECT_FIND_BEST_HAND("2H,2D,5D,4D,3D,6D,6H", poker::HandKind::STRAIGHT_FLUSH, "2D,3D,4D,5D,6D");
    EXPECT_FIND_BEST_HAND("2H,2D,5D,4D,3D,6D,AD", poker::HandKind::STRAIGHT_FLUSH, "2D,3D,4D,5D,6D");
    EXPECT_FIND_BEST_HAND("2H,2S,5S,4S,3S,6H,AS", poker::HandKind::STRAIGHT_FLUSH, "AS,2S,3S,4S,5S");
    EXPECT_FIND_BEST_HAND("2H,QC,KC,JC,10C,6C,9C", poker::HandKind::STRAIGHT_FLUSH, "9C,10C,JC,QC,KC");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestFourOfAKindFinding)
{
    EXPECT_FIND_BEST_HAND("2H,2D,2C,2S,3D,6D,AH", poker::HandKind::FOUR_OF_A_KIND, "2S,2C,2D,2H,AH");
    EXPECT_FIND_BEST_HAND("2H,2D,5D,AS,AC,AH,AD", poker::HandKind::FOUR_OF_A_KIND, "AS,AC,AD,AH,5D");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestFullHouseFinding)
{
    EXPECT_FIND_BEST_HAND("2H,2D,2C,6D,AH,3S,3D", poker::HandKind::FULL_HOUSE, "2H,2D,2C,3S,3D");
    EXPECT_FIND_BEST_HAND("2H,2D,2C,AD,AH,3S,3D", poker::HandKind::FULL_HOUSE, "2H,2D,2C,AD,AH");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestFlushFinding)
{
    EXPECT_FIND_BEST_HAND("2H,4H,6H,6D,AH,8H,5H", poker::HandKind::FLUSH, "4H,5H,6H,8H,AH");
    EXPECT_FIND_BEST_HAND("2S,4C,6D,6S,JS,10S,3S", poker::HandKind::FLUSH, "2S,3S,6S,10S,JS");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestStraightFinding)
{
    EXPECT_FIND_BEST_HAND("2H,4H,3D,6C,JS,AS,5S", poker::HandKind::STRAIGHT, "2H,3D,4H,5S,6C");
    EXPECT_FIND_BEST_HAND("2H,4H,3D,7C,JS,AS,5S", poker::HandKind::STRAIGHT, "AS,2H,3D,4H,5S");
    EXPECT_FIND_BEST_HAND("AC,10S,JD,QC,KS,AS,AD", poker::HandKind::STRAIGHT, "10S,JD,QC,KS,AC");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestThreeOfAKindFinding)
{
    EXPECT_FIND_BEST_HAND("AH,4H,AD,6C,JS,AS,10S", poker::HandKind::THREE_OF_A_KIND, "AH,AD,AS,10S,JS");
    EXPECT_FIND_BEST_HAND("QH,KH,6D,6C,6S,2S,3S", poker::HandKind::THREE_OF_A_KIND, "6D,6C,6S,QH,KH");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestTwoPairFinding)
{
    EXPECT_FIND_BEST_HAND("AH,AD,6D,6C,10S,10D,JS", poker::HandKind::TWO_PAIR, "AH,AD,10S,10D,JS");
    EXPECT_FIND_BEST_HAND("AH,AD,6D,6C,10S,QD,JS", poker::HandKind::TWO_PAIR, "AH,AD,6D,6C,QD");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestOnePairFinding)
{
    EXPECT_FIND_BEST_HAND("AH,KD,6D,6C,10S,2C,JS", poker::HandKind::ONE_PAIR, "6D,6C,JS,KD,AH");
}

///------------------------------------------------------------------------------------------------

TEST(BestHandFinderTest, TestHighCardFinding)
{
    EXPECT_FIND_BEST_HAND("AH,KD,5D,6C,10S,2C,JS", poker::HandKind::HIGH_CARD, "6C,10S,JS,KD,AH");
}

///------------------------------------------------------------------------------------------------
