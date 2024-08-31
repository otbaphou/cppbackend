#include "htmldecode.h"
#include <iostream>
std::pair<char, int> DecodeAmp(std::string_view in_str, std::string_view::iterator pos)
{
    char c1 = *pos;

    if (!(c1 >= 'a' && c1 <= 'z' || c1 >= 'A' && c1 <= 'Z'))
    {
        return { ' ', 0 };
    }

    bool is_lower = std::islower(c1) == 0 ? false : true;

    std::map<std::string, char> values{ {"lt", '<' }, {"gt", '>'}, {"amp", '&'}, {"apos", '\''}, {"quot", '"'}};


    for (std::string_view::iterator iter = pos; iter != in_str.end(); ++iter)
    {
        std::vector<std::string> keys_to_delete;

        char c = *iter;

        if (!(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z'))
        {
            return {' ', 0};
        }

        bool is_lower_tmp = std::islower(c) == 0 ? false : true;

        if (is_lower != is_lower_tmp)
        {
            return { ' ', 0 };
        }

        c = std::tolower(c);

        if (values.size() == 0)
        {
            return { ' ', 0 };
        }


        for (const std::pair<std::string, char>& entry : values)
        {
            size_t idx = std::distance(pos, iter);

            if (entry.first.size() <= idx)
            {
                continue;
            }

            if (entry.first[idx] == c)
            {
                if (idx == entry.first.size() - 1)
                {
                    return { entry.second, idx };
                }

                continue;
            }

            keys_to_delete.push_back(entry.first);
        }

        for (std::string& key : keys_to_delete)
        {
            values.erase(key);
        }
    }
    return { ' ', 0 };
}

std::string HtmlDecode(std::string_view str) 
{
    std::string result;

    for (std::string_view::iterator iter = str.begin(); iter != str.end(); ++iter)
    {
        bool decoded = false;

        if (*iter == '&')
        {
            std::pair<char, int> dcd = DecodeAmp(str, iter + 1);

            if (dcd.second != 0)
            {
                result.push_back(dcd.first);
                std::advance(iter, dcd.second + 1);

                if(*std::next(iter) == ';')
                {
                    ++iter;
                }

                continue;
            }
        }

        result.push_back(*iter);
    }

    return { result };
}
