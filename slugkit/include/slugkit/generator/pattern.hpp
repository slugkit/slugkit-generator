#pragma once

#include <slugkit/generator/constants.hpp>

#include <slugkit/generator/placeholders.hpp>
#include <slugkit/generator/types.hpp>
// #include <slugkit/common/utils/string_view_serialize.hpp>

// #include <userver/formats/parse/to.hpp>
// #include <userver/formats/serialize/common_containers.hpp>
// #include <userver/formats/serialize/to.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace slugkit::generator {

/// @brief A pattern is a string with placeholders that can be substituted with text.
/// @note The pattern is immutable.
/// The pattern captures the pattern source and child selectors are string views
/// into the source.
struct Pattern {
    /// @brief A single (non-alternating) placeholder.
    using SimplePlaceholder = std::variant<Selector, NumberGen, SpecialCharGen, EmojiGen>;

    /// @brief An alternation chooses one of several child placeholders deterministically, the same
    /// way case mutations pick a cased form: its capacity is the sum of the children's capacities,
    /// and a sequence value selects a child block and the offset within it.
    /// @note Placeholder-level alternation only for now: the children are simple placeholders, not
    /// nested alternations.
    struct Alternation {
        std::vector<SimplePlaceholder> alternatives;

        /// @brief Canonical, re-parseable representation, e.g. "{noun}|{verb}".
        [[nodiscard]] auto ToString() const -> std::string;
        [[nodiscard]] auto GetHash() const -> std::int64_t;
        [[nodiscard]] auto Complexity() const -> std::int32_t;
        [[nodiscard]] auto IsNSFW() const -> bool;
    };

    using PatternElement = std::variant<Selector, NumberGen, SpecialCharGen, EmojiGen, Alternation>;
    using Placeholders = std::vector<PatternElement>;
    using TextChunks = std::vector<std::string_view>;
    using Substitutions = std::vector<std::string>;

    const std::string pattern;
    const TextChunks text_chunks;
    const Placeholders placeholders;

    explicit Pattern(std::string pattern);
    Pattern(const Pattern&) = delete;
    Pattern& operator=(const Pattern&) = delete;

    /// @brief Check if the pattern is equal to another pattern.
    /// @param other The other pattern to compare with.
    /// @return True if the pattern is equal to the other pattern, false otherwise.
    [[nodiscard]] auto operator==(const Pattern& other) const -> bool {
        return pattern == other.pattern;
    }

    /// @brief Check if the pattern is empty.
    /// @return True if the pattern is empty, false otherwise.
    [[nodiscard]] auto IsEmpty() const -> bool {
        return placeholders.empty();
    }

    /// @brief Get the arbitrary text length of the pattern.
    /// @return The arbitrary text length of the pattern.
    [[nodiscard]] auto ArbitraryTextLength() const -> std::size_t;

    /// @brief Get the canonical string representation of the pattern.
    /// @return The string representation of the pattern.
    [[nodiscard]] auto ToString() const -> std::string;

    /// @brief Format the pattern with the substitutions.
    /// @param substitutions The substitutions to use.
    /// @return The formatted string.
    [[nodiscard]] auto Format(Substitutions substitutions) const -> std::string;

    /// @brief Get the hash of the pattern.
    /// @return The hash of the pattern.
    [[nodiscard]] auto GetHash() const -> std::int64_t;

    /// @brief Get the complexity of the pattern.
    /// @return The complexity of the pattern.
    [[nodiscard]] auto Complexity() const -> std::int32_t;

    /// @brief Check if the pattern is NSFW.
    /// @return True if the pattern is NSFW, false otherwise.
    [[nodiscard]] auto IsNSFW() const -> bool;
};

auto ParsePlaceholders(std::string_view pattern) -> Pattern::Placeholders;

auto ParsePattern(std::string_view pattern) -> Pattern;

using PatternPtr = std::shared_ptr<Pattern>;

/// @brief A utility class for formatting a pattern with substitutions.
/// Uses a single allocation for the result.
class SlugFormatter {
public:
    using Substitutions = Pattern::Substitutions;

public:
    /// @param unescape_text When true (generation), backslash escapes in the arbitrary text (e.g.
    /// `\|`, `\{`) are resolved to their literal character. When false (canonical ToString), the
    /// text is emitted verbatim so the result round-trips through the parser.
    explicit SlugFormatter(const Pattern& pattern, bool unescape_text = true);

    /// @brief Format the pattern with the substitutions.
    /// @param substitutions The substitutions to use.
    /// @return The formatted string.
    auto operator()(Substitutions substitutions) const -> std::string;

private:
    const Pattern& pattern_;
    bool unescape_text_;
};

namespace literals {

// The literal operators are mostly for tests

auto operator""_selector(const char* str, std::size_t size) -> Selector;
auto operator""_number_gen(const char* str, std::size_t size) -> NumberGen;
auto operator""_special_gen(const char* str, std::size_t size) -> SpecialCharGen;
auto operator""_emoji_gen(const char* str, std::size_t size) -> EmojiGen;

auto operator""_pattern(const char* str, std::size_t size) -> Pattern;
auto operator""_pattern_ptr(const char* str, std::size_t size) -> PatternPtr;

}  // namespace literals

}  // namespace slugkit::generator
