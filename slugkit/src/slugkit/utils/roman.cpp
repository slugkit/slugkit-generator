#include <slugkit/utils/roman.hpp>

#include <fmt/format.h>
#include <array>
#include <utility>

#include <slugkit/utils/small_map.hpp>
#include <slugkit/utils/text.hpp>

namespace slugkit::utils::roman {

namespace {

/// Roman numerals to integer values.
constexpr SmallMap<char, int, 14> kRomanNumerals = {{
    {'I', 1},
    {'V', 5},
    {'X', 10},
    {'L', 50},
    {'C', 100},
    {'D', 500},
    {'M', 1000},
    {'i', 1},
    {'v', 5},
    {'x', 10},
    {'l', 50},
    {'c', 100},
    {'d', 500},
    {'m', 1000},
}};

constexpr SmallMap<char, int, 8> kRomanValidConsecutiveChars = {{
    {'I', 3},
    {'X', 3},
    {'C', 3},
    {'M', 3},
    {'i', 3},
    {'x', 3},
    {'c', 3},
    {'m', 3},
}};

constexpr SmallMap<char, std::array<char, 2>, 6> kRomanValidSubtractions = {{
    {'I', {'V', 'X'}},
    {'X', {'L', 'C'}},
    {'C', {'D', 'M'}},
    {'i', {'v', 'x'}},
    {'x', {'l', 'c'}},
    {'c', {'d', 'm'}},
}};

constexpr auto IsValidSubtraction(char c, char prev_char) -> bool {
    auto f = kRomanValidSubtractions.find(c);
    if (f == kRomanValidSubtractions.end()) {
        return false;
    }
    for (auto c : f->second) {
        if (c == prev_char) {
            return true;
        }
    }
    return false;
}

constexpr std::array<std::pair<int, std::string_view>, 13> kRomanValues = {{
    {1000, "M"},
    {900, "CM"},
    {500, "D"},
    {400, "CD"},
    {100, "C"},
    {90, "XC"},
    {50, "L"},
    {40, "XL"},
    {10, "X"},
    {9, "IX"},
    {5, "V"},
    {4, "IV"},
    {1, "I"},
}};

}  // namespace

auto ToRoman(int num, bool lower) -> std::string {
    if (num < 1 || num > 3999) {
        throw std::invalid_argument("num must be between 1 and 3999");
    }

    std::string result;
    // there is no need to reserve memory for the result,
    // because roman numerals are not that long
    for (const auto& [value, symbol] : kRomanValues) {
        while (num >= value) {
            result.append(symbol);
            num -= value;
        }
    }

    return lower ? text::ToLower(result, text::kEnUsLocale) : result;
}

auto ParseRoman(std::string_view roman) -> int {
    // TODO track the case of the roman numeral istead of converting to lower/upper
    if (!((text::ToLower(roman, text::kEnUsLocale) == roman) || (text::ToUpper(roman, text::kEnUsLocale) == roman))) {
        throw std::invalid_argument("Roman number must be either all lowercase or all uppercase");
    }

    int total = 0;
    int prev_value = 0;
    char prev_char = '\0';
    int consecutive_chars = 1;
    bool last_subtraction = false;

    for (auto it = roman.rbegin(); it != roman.rend(); ++it) {
        auto current_value = kRomanNumerals.find(*it);
        if (current_value == kRomanNumerals.end()) {
            throw std::invalid_argument(fmt::format("Invalid Roman numeral character: {}", *it));
        }

        if (*it == prev_char) {
            if (last_subtraction) {
                throw std::invalid_argument(fmt::format("Invalid subtractive combination: {}{}", *it, prev_char));
            }
            consecutive_chars += 1;
            if (auto f = kRomanValidConsecutiveChars.find(*it);
                f == kRomanValidConsecutiveChars.end() || consecutive_chars > f->second) {
                throw std::invalid_argument(fmt::format("Invalid consecutive characters: {}{}", *it, prev_char));
            }
        } else {
            consecutive_chars = 1;
        }

        if (current_value->second >= prev_value) {
            total += current_value->second;
            last_subtraction = false;
        } else {
            if (last_subtraction) {
                throw std::invalid_argument(fmt::format("Invalid subtractive combination: {}{}", *it, prev_char));
            }
            if (!IsValidSubtraction(*it, prev_char)) {
                throw std::invalid_argument(fmt::format("Invalid subtractive combination: {}{}", *it, prev_char));
            }
            if (total % prev_value >= current_value->second) {
                throw std::invalid_argument(fmt::format("Invalid subtractive combination: {}{}", *it, prev_char));
            }
            total -= current_value->second;
            last_subtraction = true;
        }
        prev_value = current_value->second;
        prev_char = *it;
    }

    return total;
}

}  // namespace slugkit::utils::roman
