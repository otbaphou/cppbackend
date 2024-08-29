#pragma once

#include <string>
#include <set>
#include <algorithm>
#include <sstream>

/*
 * URL-кодирует строку str.
 * Пробел заменяется на +,
 * Символы, отличные от букв английского алфавита, цифр и -._~ а также зарезервированные символы
 * заменяются на их %-кодированные последовательности.
 * Зарезервированные символы: !#$&'()*+,/:;=?@[]
 */
std::string UrlEncode(std::string_view str);
