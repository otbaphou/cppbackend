#include "urlencode.h"

const static std::set<char> encoded_chars{'!', '#', '$', '&', '(', ')', '*', '+', '/', ':', ';', '=', '?', '@', '[', ']'};


std::string CodeToHex(int code)
{
    std::stringstream ss;

    ss << std::hex << code; // int decimal_value
    std::string result(ss.str());

    if (code < 0)
    {
        size_t size = result.size();
        std::string true_result{result.begin() + size - 2, result.end()};
        return true_result;
    }

    return result;
}

std::string UrlEncode(std::string_view str) 
{
    std::string result;

    for(auto iter = str.begin(); iter != str.end(); ++iter)
    {
        char c = *iter;

        if (c == ' ')
        {
            result.push_back('+');
            continue;
        }

        int value = (int)c;

        if ((value < 32 || value >= 128) || encoded_chars.contains(c))
        {
            result.push_back('%');
            result += CodeToHex(value);
            continue;
        }

        result.push_back(c);
    }

    return { result };
}
