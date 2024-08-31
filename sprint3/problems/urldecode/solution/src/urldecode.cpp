#include "urldecode.h"

#include <charconv>
#include <stdexcept>
#include <iostream>
int HexCharToInt(char c)
{
    int entry = 0;

    if (!isdigit(c))
    {
        int char_code = c - 'a';

        if (char_code < 0 || char_code > 5)
        {
            throw std::invalid_argument("Invalid HEX-string!");
        }

        entry = 10 + char_code;
    }
    else
    {
        entry = c - '0';
    }

    return entry;
}

int HexToCode(std::string hex_str)
{
    int result = 0;

    //Transforming string to lower characters

    std::transform(hex_str.begin(), hex_str.end(), hex_str.begin(), [](unsigned char c) 
    { 
        return std::tolower(c); 
    });

    size_t size = hex_str.size();

    for (size_t i = 0; i < size; ++i)
    {
        char c = hex_str[i];

        result += std::pow(16, size - i - 1) * HexCharToInt(c);
    }

    return result;
}

std::string UrlDecode(std::string_view str) 
{
    std::string true_url = "";

    for (size_t i = 0; i < str.size(); ++i)
    {
        char c = str[i];

        if(c == '+')
        {
            true_url.push_back(' ');
        }
        else
        {
            if (c == '%')
            {
                if (i + 2 < str.size())
                {
                    std::string code(str.begin() + i + 1, str.begin() + i + 3);

                    true_url.push_back(' ' + HexToCode(code) - 32);
                    i += 2;
                }
                else
                {
                    throw std::invalid_argument("Invalid % encoded character (out of bounds)");
                }
            }
            else
            {
                true_url.push_back(c);
            }
        }
    }

    return true_url;
}
