///------------------------------------------------------------------------------------------------
///  HandComparisonTest.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 6/11/2024
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <net_common/Hand.h>

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

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestHighCardScenarios)
{
    auto highCardFiveHand = CreateHand("2H,3H,5C,2C,2D", poker::HandKind::HIGH_CARD);
    auto highCardAceHand = CreateHand("7H,4D,AD,8H,4C", poker::HandKind::HIGH_CARD);
    EXPECT_LT(highCardFiveHand, highCardAceHand);
    
    auto highCardSixDiamondHand = CreateHand("2H,3H,5C,2C,6D", poker::HandKind::HIGH_CARD);
    auto highCardSixHeartHand = CreateHand("2H,3H,5C,2C,6H", poker::HandKind::HIGH_CARD);
    EXPECT_EQ(highCardSixDiamondHand, highCardSixHeartHand);
    
    auto highCardAceHandWithHigherKicker = CreateHand("6H,5D,AD,9H,4C", poker::HandKind::HIGH_CARD);
    EXPECT_LT(highCardAceHand, highCardAceHandWithHigherKicker);
    
    auto highCardAceHandWithHigherThirdKicker = CreateHand("7H,6D,AD,9H,4C", poker::HandKind::HIGH_CARD);
    EXPECT_LT(highCardAceHandWithHigherKicker, highCardAceHandWithHigherThirdKicker);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestOnePairScenarios)
{
    auto pairOfFivesHand = CreateHand("7H,5D,5C,8H,4C", poker::HandKind::ONE_PAIR);
    auto pairOfSixesHand = CreateHand("7H,5D,6C,6H,4C", poker::HandKind::ONE_PAIR);
    EXPECT_LT(pairOfFivesHand, pairOfSixesHand);
    
    auto otherPairOfFivesHigherKickerHand = CreateHand("AH,5D,5C,8H,4C", poker::HandKind::ONE_PAIR);
    EXPECT_LT(pairOfFivesHand, otherPairOfFivesHigherKickerHand);
    
    auto otherPairOfFivesHigherSecondKickerHand = CreateHand("AH,5D,5C,9H,4C", poker::HandKind::ONE_PAIR);
    EXPECT_LT(otherPairOfFivesHigherKickerHand, otherPairOfFivesHigherSecondKickerHand);
    
    auto otherPairOfFivesHigherThirdKickerHand = CreateHand("AH,5D,5C,9H,5C", poker::HandKind::ONE_PAIR);
    EXPECT_LT(otherPairOfFivesHigherSecondKickerHand, otherPairOfFivesHigherThirdKickerHand);
    
    auto otherPairOfFivesDifferentSuitsHand = CreateHand("7C,5D,5C,8D,4H", poker::HandKind::ONE_PAIR);
    EXPECT_EQ(pairOfFivesHand, otherPairOfFivesDifferentSuitsHand);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestTwoPairScenarios)
{
    auto twoPairThreesAndSixes = CreateHand("7H,3D,3C,6H,6C", poker::HandKind::TWO_PAIR);
    auto twoPairThreesAndSevens = CreateHand("AH,3D,3C,7H,7C", poker::HandKind::TWO_PAIR);
    EXPECT_LT(twoPairThreesAndSixes, twoPairThreesAndSevens);
    
    auto twoPairThreesAndSixesHigherKicker = CreateHand("AH,3D,3C,6H,6C", poker::HandKind::TWO_PAIR);
    EXPECT_LT(twoPairThreesAndSixes, twoPairThreesAndSixesHigherKicker);
    
    auto twoPairFoursAndSixes = CreateHand("7H,4D,4C,6H,6C", poker::HandKind::TWO_PAIR);
    EXPECT_LT(twoPairThreesAndSixes, twoPairFoursAndSixes);
    
    auto twoPairFoursAndFives = CreateHand("7H,4D,4C,5H,5C", poker::HandKind::TWO_PAIR);
    EXPECT_LT(twoPairFoursAndFives, twoPairThreesAndSixes);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestThreeOfAKindScenarios)
{
    auto trippleFivesHand = CreateHand("5H,5D,5C,8H,4C", poker::HandKind::THREE_OF_A_KIND);
    auto trippleAcesHand = CreateHand("AH,AD,AC,6H,4C", poker::HandKind::THREE_OF_A_KIND);
    EXPECT_LT(trippleFivesHand, trippleAcesHand);
    
    auto otherTrippleFivesHigherKickerHand = CreateHand("5H,5D,5C,AH,4C", poker::HandKind::THREE_OF_A_KIND);
    EXPECT_LT(trippleFivesHand, otherTrippleFivesHigherKickerHand);
    
    auto otherTrippleFivesHigherSecondKickerHand = CreateHand("5H,5D,5C,AH,5C", poker::HandKind::THREE_OF_A_KIND);
    EXPECT_LT(otherTrippleFivesHigherKickerHand, otherTrippleFivesHigherSecondKickerHand);
    
    auto otherTrippleFivesDifferentSuitsHand = CreateHand("5C,5D,5C,8D,4H", poker::HandKind::THREE_OF_A_KIND);
    EXPECT_EQ(trippleFivesHand, otherTrippleFivesDifferentSuitsHand);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestStraightScenarios)
{
    auto straightFiveToNine = CreateHand("5H,6D,7C,8H,9C", poker::HandKind::STRAIGHT);
    auto straightSixToTen = CreateHand("6H,7D,8C,9H,10C", poker::HandKind::STRAIGHT);
    EXPECT_LT(straightFiveToNine, straightSixToTen);
    
    auto straightFiveToNineDifferentSuit = CreateHand("5C,6S,7H,8D,9C", poker::HandKind::STRAIGHT);
    EXPECT_EQ(straightFiveToNine, straightFiveToNineDifferentSuit);
    
    auto straightTenToAce = CreateHand("10C,JS,QH,KD,AC", poker::HandKind::STRAIGHT);
    EXPECT_LT(straightFiveToNine, straightTenToAce);
    
    auto straightAceToFive = CreateHand("AC,2S,3H,4D,5C", poker::HandKind::STRAIGHT);
    EXPECT_LT(straightAceToFive, straightFiveToNine);
    EXPECT_LT(straightAceToFive, straightTenToAce);
    
    auto straightNineToKing = CreateHand("9H, 10C,JS,QH,KD", poker::HandKind::STRAIGHT);
    EXPECT_LT(straightNineToKing, straightTenToAce);
    EXPECT_LT(straightAceToFive, straightNineToKing);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestFlushScenarios)
{
    auto flushHighCardEightHand = CreateHand("2H,3H,4H,6H,8H", poker::HandKind::FLUSH);
    auto flushHighCardNineHand = CreateHand("2H,3H,4H,6H,9H", poker::HandKind::FLUSH);
    EXPECT_LT(flushHighCardEightHand, flushHighCardNineHand);
    
    auto flushHighCardNineWithHigherKickerHand = CreateHand("2H,3H,4H,7H,9H", poker::HandKind::FLUSH);
    EXPECT_LT(flushHighCardNineHand, flushHighCardNineWithHigherKickerHand);
    
    auto flushHighCardNineWithHigherThirdKickerHand = CreateHand("2H,3H,5H,7H,9H", poker::HandKind::FLUSH);
    EXPECT_LT(flushHighCardNineWithHigherKickerHand, flushHighCardNineWithHigherThirdKickerHand);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestFullHouseScenarios)
{
    auto fullHouseFivesWithNines = CreateHand("5H,5C,5S,9H,9C", poker::HandKind::FULL_HOUSE);
    auto fullHouseNinesWithFives = CreateHand("9H,9C,9S,5H,5C", poker::HandKind::FULL_HOUSE);
    EXPECT_LT(fullHouseFivesWithNines, fullHouseNinesWithFives);
    
    auto fullHouseFivesWithEights = CreateHand("5H,5C,5S,8H,8C", poker::HandKind::FULL_HOUSE);
    EXPECT_LT(fullHouseFivesWithEights, fullHouseFivesWithNines);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestFourOfAKindScenarios)
{
    auto fourOfAKindFives = CreateHand("5H,5C,5S,5D,9H", poker::HandKind::FOUR_OF_A_KIND);
    auto fourOfAKindFivesWithHigherKicker = CreateHand("5H,5C,5S,5D,10H", poker::HandKind::FOUR_OF_A_KIND);
    
    EXPECT_LT(fourOfAKindFives, fourOfAKindFivesWithHigherKicker);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestStraightFlushScenarios)
{
    auto straightFlushFiveToNine = CreateHand("5H,6H,7H,8H,9H", poker::HandKind::STRAIGHT_FLUSH);
    auto straightFlushSixToTen = CreateHand("6H,7H,8H,9H,10H", poker::HandKind::STRAIGHT_FLUSH);
    EXPECT_LT(straightFlushFiveToNine, straightFlushSixToTen);
    
    auto straightFlushAceToFive = CreateHand("AS,2S,3S,4S,5S", poker::HandKind::STRAIGHT_FLUSH);
    EXPECT_LT(straightFlushAceToFive, straightFlushFiveToNine);
    
    auto straightFlushNineToKing = CreateHand("9C, 10C,JC,QC,KC", poker::HandKind::STRAIGHT_FLUSH);
    EXPECT_LT(straightFlushAceToFive, straightFlushNineToKing);
}

///------------------------------------------------------------------------------------------------

TEST(HandComparisonTest, TestDifferentHandKindScenarios)
{
    auto highCardFiveHand = CreateHand("2H,3H,5C,2C,2D", poker::HandKind::HIGH_CARD);
    auto pairOfFivesHand = CreateHand("7H,5D,5C,8H,4C", poker::HandKind::ONE_PAIR);
    EXPECT_LT(highCardFiveHand, pairOfFivesHand);
    
    auto twoPairThreesAndSixes = CreateHand("7H,3D,3C,6H,6C", poker::HandKind::TWO_PAIR);
    EXPECT_LT(pairOfFivesHand, twoPairThreesAndSixes);
    
    auto trippleAces = CreateHand("AH,AD,AC,8H,4C", poker::HandKind::THREE_OF_A_KIND);
    EXPECT_LT(twoPairThreesAndSixes, trippleAces);
    
    auto fiveToNineStraight = CreateHand("8H,7D,9C,6H,5C", poker::HandKind::STRAIGHT);
    EXPECT_LT(trippleAces, fiveToNineStraight);
    
    auto flush = CreateHand("8H,7H,9H,6H,3H", poker::HandKind::FLUSH);
    EXPECT_LT(fiveToNineStraight, flush);
    
    auto fullHouseFivesAndThrees = CreateHand("5H,5D,3C,3H,5C", poker::HandKind::FULL_HOUSE);
    EXPECT_LT(flush, fullHouseFivesAndThrees);
    
    auto quadKings = CreateHand("KH,KD,KC,KS,4C", poker::HandKind::FOUR_OF_A_KIND);
    EXPECT_LT(fullHouseFivesAndThrees, quadKings);
    
    auto straightFlush = CreateHand("8H,7H,9H,6H,5H", poker::HandKind::STRAIGHT_FLUSH);
    EXPECT_LT(quadKings, straightFlush);
    
    auto royalFlush = CreateHand("AH,KH,QH,JH,10H", poker::HandKind::STRAIGHT_FLUSH);
    EXPECT_LT(straightFlush, royalFlush);
}

///------------------------------------------------------------------------------------------------
