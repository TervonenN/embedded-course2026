#include <gtest/gtest.h>
#include "../TimeParser.h"

// Test suite: TimeParserTest
TEST(TimeParserTest, TestCaseCorrectTime) {


    // Test with correct time string
    char time_test[] = "141205";
    ASSERT_EQ(time_parse(time_test), 51125);

}

TEST(TimeParserTest, ParsesMinutesAndSeconds)
{
    char time_test[] = "000120";

    ASSERT_EQ(time_parse(time_test), 80);
}

TEST(TimeParserTest, AcceptsMaximumValidTime) 
{

    char time_test[] = "235959";
    ASSERT_EQ(time_parse(time_test), 86399);
}

TEST(TimeParserTest, RejectsHourAboveMaximum)
{
    char time_test[] = "240000";
    ASSERT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, RejectsMinuteAboveMaximum)
{
    char time_test[] = "006000";
    ASSERT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, RejectsSecondAboveMaximum)
{
    char time_test[] = "000060";
    ASSERT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, AcceptsZeroTime)
{
    char time_test[] = "000000";
    ASSERT_EQ(time_parse(time_test), 0);
}

TEST(TimeParserTest, RejectsNullPointer)
{
    ASSERT_EQ(time_parse(nullptr), TIME_ARRAY_ERROR);
}

TEST(TimeParserTest, RejectsTooShortTime)
{
    char time_test[] = "12345";
    ASSERT_EQ(time_parse(time_test), TIME_LEN_ERROR);
}

TEST(TimeParserTest, RejectsTooLongTime)
{
    char time_test[] = "1234567";
    ASSERT_EQ(time_parse(time_test), TIME_LEN_ERROR);
}

TEST(TimeParserTest, RejectsNonNumericCharacters)
{
    char time_test[] = "12A405";
    ASSERT_EQ(time_parse(time_test), TIME_ARRAY_ERROR);
}
// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
