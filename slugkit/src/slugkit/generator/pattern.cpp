#include <slugkit/generator/pattern.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/detail/pattern_parser.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/permutations.hpp>
#include <slugkit/utils/set.hpp>
#include <slugkit/utils/text.hpp>

#include <boost/functional/hash.hpp>

#include <numeric>

namespace slugkit::generator {

//--------------------------------
// Pattern
//--------------------------------
std::string Pattern::ToString() const {
    std::vector<std::string> substitutions;
    for (const auto& element : placeholders) {
        std::visit(
            [&substitutions](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, Alternation>) {
                    // Alternation::ToString already yields "{a}|{b}" with its own braces.
                    substitutions.push_back(arg.ToString());
                } else {
                    substitutions.push_back(fmt::format("{{{}}}", arg.ToString()));
                }
            },
            element
        );
    }
    // Canonical output must round-trip through the parser, so keep the arbitrary text verbatim
    // (escapes preserved).
    return SlugFormatter(*this, /*unescape_text=*/false)(substitutions);
}

std::string Pattern::Group::ToString() const {
    // Canonical inner content: text and placeholders interleaved. Text is kept verbatim (escapes
    // preserved so it re-parses); each placeholder is wrapped in braces.
    std::string result;
    for (std::size_t i = 0; i < placeholders.size(); ++i) {
        result += text_chunks[i];
        std::visit([&result](auto&& arg) { result += fmt::format("{{{}}}", arg.ToString()); }, placeholders[i]);
    }
    result += text_chunks.back();
    return result;
}

std::int64_t Pattern::Group::GetHash() const {
    std::size_t seed = 0;
    for (const auto& chunk : text_chunks) {
        boost::hash_combine(seed, FNV1aHash(chunk));
    }
    for (const auto& placeholder : placeholders) {
        std::visit([&seed](auto&& arg) { boost::hash_combine(seed, arg.GetHash()); }, placeholder);
    }
    return seed;
}

std::int32_t Pattern::Group::Complexity() const {
    std::int32_t cost = 0;
    for (const auto& placeholder : placeholders) {
        std::visit([&cost](auto&& arg) { cost += arg.Complexity(); }, placeholder);
    }
    return cost;
}

bool Pattern::Group::IsNSFW() const {
    for (const auto& placeholder : placeholders) {
        if (std::holds_alternative<Selector>(placeholder) && std::get<Selector>(placeholder).IsNSFW()) {
            return true;
        }
    }
    return false;
}

std::string Pattern::Alternation::ToString() const {
    std::string result;
    bool first = true;
    for (const auto& branch : alternatives) {
        if (!first) {
            result += '|';
        }
        first = false;
        // A single bare placeholder renders without parentheses ({noun}); anything else (multiple
        // placeholders, or literal text around them) is parenthesised.
        const bool bare = branch.placeholders.size() == 1 && branch.text_chunks[0].empty() &&
                          branch.text_chunks[1].empty();
        if (bare) {
            result += branch.ToString();
        } else {
            result += '(';
            result += branch.ToString();
            result += ')';
        }
    }
    return result;
}

std::int64_t Pattern::Alternation::GetHash() const {
    std::size_t seed = 0;
    for (const auto& branch : alternatives) {
        boost::hash_combine(seed, branch.GetHash());
    }
    return seed;
}

std::int32_t Pattern::Alternation::Complexity() const {
    std::int32_t cost = 0;
    for (const auto& branch : alternatives) {
        cost += branch.Complexity();
    }
    return cost;
}

bool Pattern::Alternation::IsNSFW() const {
    for (const auto& branch : alternatives) {
        if (branch.IsNSFW()) {
            return true;
        }
    }
    return false;
}

std::string Pattern::Format(Substitutions substitutions) const {
    SlugFormatter formatter(*this);
    return formatter(std::move(substitutions));
}

// @note This relies on presence of NSFW selector in the pattern
//       but a dictionary itself may contain or not contain dictionaries
//       with NSFW words and they might not be marked
bool Pattern::IsNSFW() const {
    for (const auto& element : placeholders) {
        if (std::holds_alternative<Selector>(element)) {
            if (std::get<Selector>(element).IsNSFW()) {
                return true;
            }
        } else if (std::holds_alternative<Alternation>(element)) {
            if (std::get<Alternation>(element).IsNSFW()) {
                return true;
            }
        }
    }
    return false;
}

std::int64_t Pattern::GetHash() const {
    std::size_t seed = FNV1aHash(pattern);
    for (const auto& element : placeholders) {
        std::visit([&seed](auto&& arg) { boost::hash_combine(seed, arg.GetHash()); }, element);
    }
    return seed;
}

std::int32_t Pattern::Complexity() const {
    std::int32_t cost = 0;
    for (const auto& element : placeholders) {
        std::visit([&cost](auto&& arg) { cost += arg.Complexity(); }, element);
    }
    return cost;
}

Pattern::Pattern(std::string pattern)
    : pattern(std::move(pattern)) {
    const_cast<Placeholders&>(placeholders) =
        detail::PatternParser(this->pattern)(const_cast<TextChunks&>(text_chunks));
}

std::size_t Pattern::ArbitraryTextLength() const {
    return std::accumulate(
        text_chunks.begin(),
        text_chunks.end(),
        std::size_t(0),
        [](std::size_t sum, const std::string_view& chunk) { return sum + chunk.size(); }
    );
}

Pattern::Placeholders ParsePlaceholders(std::string_view pattern) {
    return detail::PatternParser(pattern)();
}

Pattern ParsePattern(std::string_view pattern) {
    return Pattern(std::string(pattern));
}

namespace {

// Append arbitrary text, optionally resolving backslash escapes (`\X` -> `X`). Called for each text
// chunk during formatting; the parser guarantees every backslash is followed by an escapable char.
void AppendText(std::string& result, std::string_view chunk, bool unescape) {
    if (!unescape) {
        result.append(chunk);
        return;
    }
    for (std::size_t i = 0; i < chunk.size(); ++i) {
        if (chunk[i] == '\\' && i + 1 < chunk.size()) {
            result.push_back(chunk[++i]);
        } else {
            result.push_back(chunk[i]);
        }
    }
}

}  // namespace

std::string FormatChunks(const Pattern::TextChunks& text_chunks, const Pattern::Substitutions& substitutions,
                         bool unescape_text) {
    // Invariant: text_chunks.size() == substitutions.size() + 1. Shared by SlugFormatter (whole
    // pattern) and the group substitution generator (a group's sub-pattern).
    std::size_t reserve = 0;
    for (const auto& chunk : text_chunks) {
        reserve += chunk.size();
    }
    for (const auto& substitution : substitutions) {
        reserve += substitution.size();
    }
    std::string result;
    result.reserve(reserve);
    for (std::size_t i = 0; i < substitutions.size(); ++i) {
        AppendText(result, text_chunks[i], unescape_text);
        result += substitutions[i];
    }
    AppendText(result, text_chunks.back(), unescape_text);
    return result;
}

SlugFormatter::SlugFormatter(const Pattern& pattern, bool unescape_text)
    : pattern_(pattern)
    , unescape_text_(unescape_text) {
}

std::string SlugFormatter::operator()(Substitutions substitutions) const {
    if (substitutions.size() != pattern_.placeholders.size()) {
        throw SlugFormatError(
            fmt::format("Expected {} substitutions, got {}", pattern_.placeholders.size(), substitutions.size())
        );
    }
    return FormatChunks(pattern_.text_chunks, substitutions, unescape_text_);
}

namespace literals {

Selector operator""_selector(const char* str, std::size_t size) {
    auto parser = detail::PatternParser(std::string_view(str, size));
    auto ident = parser.ParseIdentifier();
    if (ident.value == detail::PatternParser::kNumberKeyword) {
        throw PatternSyntaxError("Expected dictionary kind, got number");
    }
    return parser.ParseSelector(std::move(ident));
}

NumberGen operator""_number_gen(const char* str, std::size_t size) {
    auto parser = detail::PatternParser(std::string_view(str, size));
    parser.Expect(detail::PatternParser::kNumberKeyword);
    return parser.ParseNumberGen();
}

SpecialCharGen operator""_special_gen(const char* str, std::size_t size) {
    auto parser = detail::PatternParser(std::string_view(str, size));
    parser.Expect(detail::PatternParser::kSpecialCharKeyword);
    return parser.ParseSpecialCharGen();
}

EmojiGen operator""_emoji_gen(const char* str, std::size_t size) {
    auto parser = detail::PatternParser(std::string_view(str, size));
    parser.Expect(detail::PatternParser::kEmojiKeyword);
    return parser.ParseEmojiGen();
}

Pattern operator""_pattern(const char* str, std::size_t size) {
    return Pattern(std::string(str, size));
}

PatternPtr operator""_pattern_ptr(const char* str, std::size_t size) {
    return std::make_shared<Pattern>(std::string(str, size));
}

}  // namespace literals

}  // namespace slugkit::generator
