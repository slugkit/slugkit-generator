#pragma once

#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

namespace slugkit::utils {

/// @brief A lightweight substring marker that stores position as offset and size.
///
/// `string_markup` solves the dangling reference problem when storing `string_view`
/// members that refer to substrings of an owned `std::string`. By storing offsets
/// instead of pointers, the markup remains valid after move operations.
///
/// @tparam Char The character type (char, wchar_t, char8_t, etc.)
/// @tparam Traits The character traits (defaults to std::char_traits<Char>)
///
/// @par Example: Move-Safe String Components
/// @code
/// class LanguageCode {
///     std::string code_;              // "en-US"
///     string_markup language_;        // Offset to "en"
///     string_markup region_;          // Offset to "US"
///
/// public:
///     LanguageCode(std::string code) : code_(std::move(code)) {
///         language_ = string_markup(code_, parse_language());
///         region_ = string_markup(code_, parse_region());
///     }
///
///     // Compiler-generated copy/move: perfect! ✅
///     LanguageCode(const LanguageCode&) = default;
///     LanguageCode(LanguageCode&&) noexcept = default;
///
///     auto language() const -> std::string_view {
///         return language_.value(code_);  // Reconstruct on demand
///     }
/// };
/// @endcode
///
/// @par Performance
/// - Space: Same as string_view (16 bytes on 64-bit)
/// - Construction: O(1)
/// - Copy/Move: Trivial (just integers)
/// - Access: O(1) with pointer arithmetic overhead
///
/// @par Thread Safety
/// `string_markup` is immutable after construction and safe to access from
/// multiple threads without synchronization.
template <typename Char, std::unsigned_integral SizeType = std::size_t, typename Traits = std::char_traits<Char>>
class basic_string_markup {
public:
    using string_view_type = std::basic_string_view<Char, Traits>;
    using size_type = SizeType;
    using string_iterator_type = typename string_view_type::const_iterator;

    constexpr basic_string_markup() = default;

    /// @brief Constructs a markup with explicit start position and size.
    ///
    /// @param start Starting offset from the beginning of the source string
    /// @param size Length of the substring
    ///
    /// @par Example
    /// @code
    /// string_markup m(0, 5);  // First 5 characters
    /// auto view = m.value("hello world");  // "hello"
    /// @endcode
    ///
    /// @note No validation is performed during construction. Bounds are checked
    ///       when calling value().
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    constexpr basic_string_markup(size_type start, size_type size)
        : start_(start)
        , size_(size) {
    }

    /// @brief Constructs a markup representing the entire string_view.
    ///
    /// @param string The string_view to mark up (stores start=0, size=length)
    ///
    /// @par Example
    /// @code
    /// string_markup m("hello");  // start=0, size=5
    /// @endcode
    /* implicit */ constexpr basic_string_markup(string_view_type string) noexcept
        : basic_string_markup(0, string.size()) {
    }

    /// @brief Constructs a markup by calculating the substring's position within a parent string.
    ///
    /// Automatically calculates the offset by comparing the substring's pointer
    /// position within the parent string.
    ///
    /// @param str The parent string_view
    /// @param substr A substring whose position will be marked up
    ///
    /// @par Example
    /// @code
    /// std::string_view full = "hello-world";
    /// std::string_view part = full.substr(0, 5);
    /// string_markup m(full, part);  // Stores: start=0, size=5
    /// @endcode
    ///
    /// @warning substr must actually be a substring of str (pointers must be within bounds).
    ///          Behavior is undefined if this precondition is violated.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    constexpr basic_string_markup(string_view_type str, string_view_type substr) noexcept
        : basic_string_markup(substr.begin() - str.begin(), substr.size()) {
    }

    /// @brief Constructs a markup by calculating the substring's position within a parent string.
    ///
    /// Automatically calculates the offset by comparing the substring's iterator position within the parent string.
    ///
    /// @param begin The beginning of the parent string
    /// @param sub_start The beginning of the substring
    /// @param sub_end The end of the substring
    ///
    /// @par Example
    /// @code
    /// std::string_view full = "hello-world";
    /// std::string_view part = full.substr(0, 5);
    /// string_markup m(full.begin(), part.begin(), part.end());  // Stores: start=0, size=5
    /// @endcode
    constexpr basic_string_markup(
        string_iterator_type begin,
        string_iterator_type sub_start,
        string_iterator_type sub_end
    ) noexcept
        : basic_string_markup(static_cast<size_type>(sub_start - begin), static_cast<size_type>(sub_end - sub_start)) {
    }

    /// @brief Returns the starting offset of the marked substring.
    ///
    /// @return Starting position (offset from beginning of source string)
    [[nodiscard]] constexpr auto start() const noexcept -> size_type {
        return start_;
    }

    /// @brief Returns the size of the marked substring.
    ///
    /// @return Length of the substring in characters
    [[nodiscard]] constexpr auto size() const noexcept -> size_type {
        return size_;
    }

    /// @brief Checks if the markup is empty (size is zero).
    ///
    /// @return true if size() == 0, false otherwise
    [[nodiscard]] constexpr auto empty() const noexcept -> bool {
        return size_ == 0;
    }

    /// @brief Reconstructs a string_view from the markup using the provided source string.
    ///
    /// This is the core operation that makes `string_markup` useful: it reconstructs
    /// the substring view using the current location of the source string, making it
    /// safe to use after moves.
    ///
    /// @param str The source string to extract the substring from
    /// @return A string_view pointing to the marked substring within str
    ///
    /// @throws std::out_of_range if the markup's start or end position exceeds str's size
    ///
    /// @par Example
    /// @code
    /// std::string code = "en-US";
    /// string_markup lang(0, 2);
    ///
    /// auto view1 = lang.value(code);  // "en"
    ///
    /// std::string moved = std::move(code);
    /// auto view2 = lang.value(moved);  // Still "en" ✅
    /// @endcode
    ///
    /// @par Exception Safety
    /// Strong exception guarantee. If an exception is thrown, the markup remains unchanged.
    [[nodiscard]] constexpr auto value(string_view_type str) const -> string_view_type {
        if (size_ == 0) {
            return {};
        }
        if (start_ >= str.size()) {
            throw std::out_of_range(
                fmt::format("string_markup: start {} is out of range for string of size {}", start_, str.size())
            );
        }
        if (start_ + size_ > str.size()) {
            throw std::out_of_range(
                fmt::format("string_markup: start {} + size {} > str.size() {}", start_, size_, str.size())
            );
        }
        return {str.data() + start_, size_};
    }

    /// @brief Reconstructs a string_view from the markup using the provided base pointer.
    ///
    /// @param base The base pointer to extract the substring from
    /// @return A string_view pointing to the marked substring within base
    /// @note This function checks nothing, it's the responsibility of the caller to ensure the base pointer is valid.
    [[nodiscard]] constexpr auto value(const std::byte* base) const -> std::string_view {
        return {reinterpret_cast<const char*>(base + start_), size_};
    }
    [[nodiscard]] constexpr auto value(const char* base) const -> std::string_view {
        return {base + start_, size_};
    }

private:
    size_type start_ = 0;
    size_type size_ = 0;
};

using string_markup =
    basic_string_markup<std::string_view::value_type, std::string_view::size_type, std::string_view::traits_type>;

}  // namespace slugkit::utils
