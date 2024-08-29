#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) 
{
    ASSERT_EQ(UrlEncode(""sv), ""s);
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_EQ(UrlEncode("Hello World!"sv), "Hello+World%21"s);
    EXPECT_EQ(UrlEncode("abc*"sv), "abc%2a"s);
    EXPECT_EQ(UrlEncode(" "sv), "+"s);
    EXPECT_EQ(UrlEncode("     "sv), "+++++"s);
    EXPECT_EQ(UrlEncode("hello\r"sv), "hello%d"s);

    std::string feed = { char(131) };

    EXPECT_EQ(UrlEncode(feed), "%83"s);

    feed = char(138);

    EXPECT_EQ(UrlEncode(feed), "%8a"s);

    feed += "Hello World!\r";

    EXPECT_EQ(UrlEncode(feed), "%8aHello+World%21%d"s);
}

/* Напишите остальные тесты самостоятельно */
