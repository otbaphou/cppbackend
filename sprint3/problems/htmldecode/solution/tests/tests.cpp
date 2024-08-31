#include <catch2/catch_test_macros.hpp>

#include "../src/htmldecode.h"
#include <iostream>
using namespace std::literals;

TEST_CASE("Text without mnemonics", "[HtmlDecode]") 
{
    CHECK(HtmlDecode(""sv) == ""s);
    CHECK(HtmlDecode("hello"sv) == "hello"s);

    CHECK(HtmlDecode("&aPos;"sv) == "&aPos;"s);

    CHECK(HtmlDecode("&&&"sv) == "&&&"s);
    CHECK(HtmlDecode("hello&&&"sv) == "hello&&&"s);

    CHECK(HtmlDecode("hello&lt"sv) == "hello<"s);
    CHECK(HtmlDecode("hello&gt"sv) == "hello>"s);
    CHECK(HtmlDecode("hello&amp"sv) == "hello&"s);
    CHECK(HtmlDecode("hello&apos"sv) == "hello'"s);
    CHECK(HtmlDecode("hello&quot"sv) == "hello\""s);

    CHECK(HtmlDecode("hello&l"sv) == "hello&l"s);

    CHECK(HtmlDecode("Johnson&amp;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&ampJohnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&AMP;Johnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&AMPJohnson"sv) == "Johnson&Johnson"s);
    CHECK(HtmlDecode("Johnson&Johnson"sv) == "Johnson&Johnson"s);

    CHECK(HtmlDecode("M&amp;M&APOSs"sv) == "M&M's"s);

    CHECK(HtmlDecode("&amp;lt;"sv) == "&lt;"s);

    std::string s;
    std::cin >> s;
}

// Напишите недостающие тесты самостоятельно
