#pragma once

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/types.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string_view>
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
    using TagsType = std::unordered_set<std::string_view>;
    using OptionsType = std::map<std::string_view, std::string_view>;

    std::string_view kind;
    TagsType include_tags;
    TagsType exclude_tags;
    std::optional<std::string_view> language;
    std::optional<SizeLimit> size_limit;
    OptionsType options;

    auto operator==(const Selector& other) const -> bool {
        return kind == other.kind && include_tags == other.include_tags && exclude_tags == other.exclude_tags &&
               language == other.language && size_limit == other.size_limit && options == other.options;
    }

    /// @brief Get the case of the selector.
    [[nodiscard]] auto GetCase() const -> CaseType;

    [[nodiscard]] auto HasSizeLimit() const -> bool {
        return size_limit.has_value();
    }

    [[nodiscard]] auto HasTags() const -> bool {
        return !include_tags.empty() || !exclude_tags.empty();
    }

    [[nodiscard]] auto NoFilter() const -> bool {
        return include_tags.empty() && exclude_tags.empty() && !HasSizeLimit();
    }

    /// @brief Check if the selector has mutually exclusive tags.
    /// If there is a tag in the exclude list that is also in the include list,
    /// the tags are mutually exclusive and the selector is invalid.
    [[nodiscard]] auto MutuallyExclusiveTags() const -> std::vector<std::string_view> {
        std::vector<std::string_view> result;
        for (const auto& tag : exclude_tags) {
            if (include_tags.contains(tag)) {
                result.push_back(tag);
            }
        }
        return result;
    }

    [[nodiscard]] auto LimitsMaxLength() const -> bool;

    [[nodiscard]] auto GetMaxLength() const -> std::optional<std::size_t>;

    /// @brief Get the hash of the selector.
    [[nodiscard]] auto GetHash() const -> std::int64_t;

    [[nodiscard]] auto ToString() const -> std::string;

    [[nodiscard]] auto Complexity() const -> std::int32_t;

    [[nodiscard]] auto IsNSFW() const -> bool;

    void ApplyOptions(std::string_view original_pattern, OptionsType&& options);

private:
    mutable CaseType case_type_{CaseType::kNone};
};

/// @brief Check if a word matches a selector.
/// @param selector The selector to check.
/// @param word The word to check.
/// @return True if the word matches the selector, false otherwise.
auto Matches(const Selector& selector, const Word& word) -> bool;

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

struct EmojiGen {
    using TagsType = Selector::TagsType;
    using OptionsType = Selector::OptionsType;

    constexpr static std::string_view kCountOption = "count";
    constexpr static std::string_view kUniqueOption = "unique";
    constexpr static std::string_view kToneOption = "tone";
    constexpr static std::string_view kGenderOption = "gender";

    TagsType include_tags{};
    TagsType exclude_tags{};
    std::uint8_t min_count = 1;
    std::uint8_t max_count = 1;
    bool unique = false;
    std::string_view tone;
    std::string_view gender;

    [[nodiscard]] auto GetHash() const -> std::int64_t;

    [[nodiscard]] auto ToString() const -> std::string;

    [[nodiscard]] auto Complexity() const -> std::int32_t;

    void ApplyOptions(std::string_view original_pattern, OptionsType&& options);

private:
    bool has_options_ = false;
};

}  // namespace slugkit::generator