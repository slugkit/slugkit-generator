#pragma once

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/types.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace slugkit::generator {

enum class CompareOperator { kNone, kEq, kNe, kGt, kGe, kLt, kLe };

struct SizeLimit {
    CompareOperator op;
    std::uint8_t value;

    operator bool() const {
        return op != CompareOperator::kNone;
    }

    auto operator==(const SizeLimit& other) const -> bool {
        return op == other.op && value == other.value;
    }

    [[nodiscard]] auto Matches(std::size_t value) const -> bool;

    [[nodiscard]] auto GetHash() const -> std::int64_t;
};

/// @brief A selector is a set of tags that can be used to filter the
///  words in a dictionary.
/// @note The selector is immutable.
/// @note The selector is only valid as long as the pattern is alive.
struct Selector {
    using tag_list_t = std::unordered_set<std::string_view>;
    using option_map_t = std::unordered_map<std::string_view, std::string_view>;

    std::string_view kind;
    tag_list_t include_tags;
    tag_list_t exclude_tags;
    std::optional<std::string_view> language;
    std::optional<SizeLimit> size_limit;
    option_map_t options;

    auto operator==(const Selector& other) const -> bool {
        return kind == other.kind && include_tags == other.include_tags && exclude_tags == other.exclude_tags &&
               language == other.language && size_limit == other.size_limit && options == other.options;
    }

    /// @brief Get the case of the selector.
    [[nodiscard]] auto GetCase() const -> CaseType;

    [[nodiscard]] auto HasSizeLimit() const -> bool {
        return size_limit.has_value();
    }

    [[nodiscard]] auto LimitsMaxLength() const -> bool;

    [[nodiscard]] auto GetMaxLength() const -> std::optional<std::size_t>;

    /// @brief Get the hash of the selector.
    [[nodiscard]] auto GetHash() const -> std::int64_t;

    [[nodiscard]] auto ToString() const -> std::string;

    [[nodiscard]] auto Complexity() const -> std::int32_t;

    [[nodiscard]] auto IsNSFW() const -> bool;

private:
    mutable CaseType case_type_{CaseType::kNone};
};

/// @brief Check if a word matches a selector.
/// @param selector The selector to check.
/// @param word The word to check.
/// @param skip_dictionary_check If true, the dictionary check is skipped (kind and language).
/// @return True if the word matches the selector, false otherwise.
auto Matches(const Selector& selector, const Word& word, bool skip_dictionary_check = false) -> bool;

/// @brief The base of the number generator.
/// @note The base is used to calculate the capacity of the number generator.
/// @note The base is used to format the number generator.
enum class NumberBase { kDec, kHex, kHexUpper, kRoman, kRomanLower };

/// @brief Class that defines settings for a number generator.
/// placeholder := '{', 'number', [':', number, base], '}';
struct NumberGen {
    std::uint8_t max_length;
    NumberBase base;

    [[nodiscard]] auto GetHash() const -> std::int64_t;

    [[nodiscard]] auto ToString() const -> std::string;

    [[nodiscard]] auto Complexity() const -> std::int32_t {
        return constants::kNumberBaseCost;
    }
};

/// @brief Class that defines settings for a special character generator.
/// placeholder := '{', 'special', [':', number, ['-', number]], '}';
/// min_length := 0..255
/// max_length := 1..255
/// both min_length and max_length are inclusive
struct SpecialCharGen {
    std::uint8_t min_length;
    std::uint8_t max_length;

    [[nodiscard]] auto GetHash() const -> std::int64_t;

    [[nodiscard]] auto ToString() const -> std::string;

    [[nodiscard]] auto Complexity() const -> std::int32_t;
};

}  // namespace slugkit::generator