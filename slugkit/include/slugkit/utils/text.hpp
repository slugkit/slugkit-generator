#pragma once

#include <userver/utils/strong_typedef.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace slugkit::utils::text {

using CaseMask = userver::utils::StrongTypedef<struct CaseMaskTag, std::uint64_t>;

extern const std::string kEnUsLocale;

auto GetLocale(const std::string& name) -> const std::locale&;

auto ToLower(std::string_view str, const std::string& locale) -> std::string;

auto ToUpper(std::string_view str, const std::string& locale) -> std::string;

auto Capitalize(std::string_view str, const std::string& locale) -> std::string;

/// @brief Change case of the string using the permutation mask
/// @param str The string to change
/// @param locale The locale to use
/// @param permutation The permutation to use, each bit of the permutation is used to change the case of the
/// corresponding character (0 - lower, 1 - upper), lsb is used for the first character
/// @return The string with changed case
auto MixedCase(std::string_view str, const std::string& locale, CaseMask permutation) -> std::string;

/// @brief Split the string into a vector of strings using the delimiter
/// @tparam OutputIterator The type of the output iterator
/// @param str The string to split
/// @param delimiter The delimiter to use
/// @return A vector of strings
template <typename OutputIterator>
auto Split(std::string_view str, std::string_view delimiter, OutputIterator out) -> OutputIterator {
    auto pos = str.find(delimiter);
    while (pos != std::string_view::npos) {
        *out++ = str.substr(0, pos);
        str = str.substr(pos + delimiter.size());
        pos = str.find(delimiter);
    }
    *out++ = str;
    return out;
}

/// @brief Join a vector of strings into a single string using the delimiter
/// @tparam InputIterator The type of the input iterator
/// @param first The first iterator
/// @param last The last iterator
/// @param delimiter The delimiter to use
/// @return A single string
template <typename InputIterator>
auto Join(InputIterator first, InputIterator last, std::string_view delimiter) -> std::string {
    std::string result;
    while (first != last) {
        result += *first;
        ++first;
        if (first != last) {
            result += delimiter;
        }
    }
    return result;
}

}  // namespace slugkit::utils::text
