#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) 
{
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("HelloWorld"sv) == "HelloWorld"s);
    BOOST_TEST(UrlDecode("Hello+World"sv) == "Hello World"s);
    BOOST_TEST(UrlDecode("Hello%20World"sv) == "Hello World"s);

    bool passed1 = false;

    try
    {
        UrlDecode("Hello% 20World"sv);
    }
    catch (...)
    {
        passed1 = true;
    }

    BOOST_TEST(passed1);

    bool passed2 = false;

    try
    {
        UrlDecode("Hello%World"sv);
    }
    catch (...)
    {
        passed2 = true;
    }

    BOOST_TEST(passed2);

    bool passed3 = false;

    try
    {
        UrlDecode("Hello%2"sv);
    }
    catch (...)
    {
        passed3 = true;
    }

    BOOST_TEST(passed3);

    std::string s;
    std::cin >> s;
}