#pragma once

#include <slugkit/compat/strong_typedef.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace slugkit::utils::text {

using CaseMask = slugkit::compat::StrongTypedef<struct CaseMaskTag, std::uint64_t>;

extern const std::string kEnUsLocale;

auto ToLower(std::string_view str, const std::string& locale) -> std::string;

auto ToUpper(std::string_view str, const std::string& locale) -> std::string;

auto Capitalize(std::string_view str, const std::string& locale) -> std::string;

auto FirstUpper(std::string_view str, const std::string& locale) -> std::string;

/// @brief Change case of the string using the permutation mask
/// @param str The string to change
/// @param locale The locale to use
/// @param permutation The permutation to use, each bit of the permutation is used to change the case of the
/// corresponding character (0 - lower, 1 - upper), lsb is used for the first character
/// @return The string with changed case
auto MixedCase(std::string_view str, const std::string& locale, CaseMask permutation) -> std::string;

/// @brief Count the byte positions in @p str whose case can change (ASCII letters). This matches
/// MixedCase's byte-wise model: only these positions are affected by a case mask, so a word has
/// exactly 2^CountCaseToggleable(str) distinct cased forms (digits, hyphens, spaces and bytes of
/// multi-byte sequences are not counted).
auto CountCaseToggleable(std::string_view str) noexcept -> std::size_t;

/// @brief Expand a compact case index into a CaseMask for MixedCase. The low bits of @p compact
/// are assigned in order to the toggleable (letter) positions of @p str; non-letter positions
/// stay lower-case. This is a collision-free bijection between [0, 2^CountCaseToggleable(str)) and
/// the distinct cased forms of @p str. If @p compact carries more bits than there are toggleable
/// positions the surplus high bits are ignored.
auto ExpandCaseMask(std::string_view str, std::uint64_t compact) noexcept -> CaseMask;

/// @brief Split the string into a vector of strings using the delimiter
/// @tparam OutputIterator The type of the output iterator
/// @param str The string to split
/// @param delimiter The delimiter to use
/// @return A vector of strings
template <typename OutputIterator>
auto Split(std::string_view str, std::string_view delimiter, OutputIterator out) -> OutputIterator {
    auto pos = str.find(delimiter);
    while (pos != std::string_view::npos) {
        if constexpr (std::is_same_v<std::decay_t<typename OutputIterator::container_type::value_type>, std::string>) {
            *out++ = std::string(str.substr(0, pos));
        } else {
            *out++ = str.substr(0, pos);
        }
        str = str.substr(pos + delimiter.size());
        pos = str.find(delimiter);
    }
    if constexpr (std::is_same_v<std::decay_t<typename OutputIterator::container_type::value_type>, std::string>) {
        *out++ = std::string(str);
    } else {
        *out++ = str;
    }
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
    using iterator_value_type = typename std::decay_t<typename std::iterator_traits<InputIterator>::value_type>;
    std::string result;
    while (first != last) {
        if constexpr (std::is_same_v<iterator_value_type, std::string>) {
            result += *first;
        } else if constexpr (slugkit::compat::IsStrongTypedef<iterator_value_type>::value) {
            result += std::string(first->GetUnderlying());
        } else {
            result += std::string(*first);
        }
        ++first;
        if (first != last) {
            result += delimiter;
        }
    }
    return result;
}

}  // namespace slugkit::utils::text
