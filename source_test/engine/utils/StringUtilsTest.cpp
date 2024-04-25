///------------------------------------------------------------------------------------------------
///  StringUtilsTest.cpp
///  Predators
///
///  Created by Alex Koukoulas on 19/09/2023
///------------------------------------------------------------------------------------------------

#include <gtest/gtest.h>
#include <engine/utils/MathUtils.h>
#include <engine/utils/StringUtils.h>

TEST(StringIsIntTests, TestCharactersAreNotInts)
{
    EXPECT_FALSE(strutils::StringIsInt("c"));
    EXPECT_FALSE(strutils::StringIsInt("!"));
}

TEST(StringIsIntTests, TestNumbersAreInts)
{
    EXPECT_TRUE(strutils::StringIsInt("1"));
    EXPECT_TRUE(strutils::StringIsInt("999999999"));
}

TEST(StringStartsWithTests, TestPrefixedStringReturnsTrue)
{
    EXPECT_TRUE(strutils::StringStartsWith("abcd", "abc"));
}

TEST(StringStartsWithTests, TestNotPrefixedStringReturnsFalse)
{
    EXPECT_FALSE(strutils::StringStartsWith("abcd", "abd"));
}

TEST(StringStartsWithTests, TestPatternIsLargerThanStringReturnsFalse)
{
    EXPECT_FALSE(strutils::StringStartsWith("abc", "abcd"));
}

TEST(StringEndsWithTests, TestPostfixedStringReturnsTrue)
{
    EXPECT_TRUE(strutils::StringEndsWith("abcd", "bcd"));
}

TEST(StringEndsWithTests, TestNotPostfixedStringReturnsFalse)
{
    EXPECT_FALSE(strutils::StringEndsWith("abcd", "abc"));
}

TEST(StringEndsWithTests, TestPatternIsLargerThanStringReturnsFalse)
{
    EXPECT_FALSE(strutils::StringEndsWith("abc", "abcd"));
}

TEST(StringContainsTests, TestContainedStringReturnsTrue)
{
    EXPECT_TRUE(strutils::StringContains("abcd", "bcd"));
}

TEST(StringContainsTests, TestNotContainedStringReturnsFalse)
{
    EXPECT_FALSE(strutils::StringContains("abcd", "abd"));
}

TEST(StringContainsTests, TestPatternIsLargerThanStringReturnsFalse)
{
    EXPECT_FALSE(strutils::StringContains("abc", "abcd"));
}

TEST(StringToUpperTests, TestStringReturnedIsUppercase)
{
    EXPECT_EQ(strutils::StringToUpper("abc"), "ABC");
}

TEST(StringToLowerTests, TestStringReturnedIsLowercase)
{
    EXPECT_EQ(strutils::StringToLower("ABC"), "abc");
}

TEST(StringSplitTests, TestStringIsCorrectlySplitBySpace)
{
    auto expectedVector = std::vector<std::string>{"A", "B", "C"};
    EXPECT_EQ(strutils::StringSplit("A B C", ' ')[0], expectedVector[0]);
    EXPECT_EQ(strutils::StringSplit("A B C", ' ')[1], expectedVector[1]);
    EXPECT_EQ(strutils::StringSplit("A B C", ' ')[2], expectedVector[2]);
}

TEST(StringSplitTests, TestStringIsCorrectlySplitBySpaceEvenIfThereAreMultipleConsecutiveSpaces)
{
    auto expectedVector = std::vector<std::string>{"A", "B", "C"};
    EXPECT_EQ(strutils::StringSplit("A  B   C", ' ')[0], expectedVector[0]);
    EXPECT_EQ(strutils::StringSplit("A  B   C", ' ')[1], expectedVector[1]);
    EXPECT_EQ(strutils::StringSplit("A  B   C", ' ')[2], expectedVector[2]);
}

TEST(VecToStringTests, TestCorrectConstructionOfStringBasedOnInputVector)
{
    auto expectedString = "[1, 2, 3, 4]";
    EXPECT_EQ(strutils::VecToString(std::vector<std::string>{"1", "2", "3", "4"}), expectedString);
}

TEST(VecToStringTests, TestCorrectConstructionOfStringBasedOnEmptyInputVector)
{
    auto expectedString = "[]";
    EXPECT_EQ(strutils::VecToString(std::vector<std::string>{}), expectedString);
}

TEST(StringToVecOfStringsTests, TestCorrectConstructionOfVecOfStringsBasedOnParsingInputString)
{
    std::vector<std::string> expectedVector = {"1", "2", "3", "4"};
    EXPECT_EQ(strutils::StringToVecOfStrings("[1,2,3,4]"), expectedVector);
}

TEST(StringToVecOfStringsTests, TestCorrectConstructionOfVecOfStringsBasedOnParsingInputEmptyStringRegardlessOfWhitespace)
{
    std::vector<std::string> expectedVector = {"1", "2", "3", "4"};
    EXPECT_EQ(strutils::StringToVecOfStrings("[1,2, 3,  4]"), expectedVector);
}

TEST(StringToVecOfStringsTests, TestCorrectConstructionOfVecOfStringsBasedOnParsingStringWithMalformedBrackets)
{
    std::vector<std::string> expectedVector = {"1", "2", "3", "4"};
    EXPECT_EQ(strutils::StringToVecOfStrings("[1,2,3,4["), expectedVector);
}

TEST(StringToVecOfStringsTests, TestCorrectConstructionOfVecOfStringsBasedOnParsingInputEmptyString)
{
    std::vector<std::string> expectedVector = {};
    EXPECT_EQ(strutils::StringToVecOfStrings(""), expectedVector);
}

TEST(GetHoursMinutesStringFromSecondsTests, TestProperFormattingOfTime)
{
    EXPECT_EQ(strutils::GetHoursMinutesStringFromSeconds(43920), "12:12");
}

TEST(StringReplaceAllOccurencesTests, TestProperOccurrenceReplacement)
{
    std::string string = "ABABABA";
    strutils::StringReplaceAllOccurences("BA", "CA", string);
    EXPECT_EQ(string, "ACACACA");
}

TEST(FloatToStringTests, TestCastingsWithDifferentPrecisions)
{
    EXPECT_EQ(strutils::FloatToString(1.33333f, 0), "1");
    EXPECT_EQ(strutils::FloatToString(1.33333f, 1), "1.3");
    EXPECT_EQ(strutils::FloatToString(1.33333f, 2), "1.33");
    EXPECT_EQ(strutils::FloatToString(1.33333f, 3), "1.333");
}
