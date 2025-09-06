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
            [&substitutions](auto&& arg) { substitutions.push_back(fmt::format("{{{}}}", arg.ToString())); }, element
        );
    }
    return SlugFormatter(*this)(substitutions);
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

SlugFormatter::SlugFormatter(const Pattern& pattern)
    : pattern_(pattern) {
}

std::string SlugFormatter::operator()(Substitutions substitutions) const {
    if (substitutions.size() != pattern_.placeholders.size()) {
        throw SlugFormatError(
            fmt::format("Expected {} substitutions, got {}", pattern_.placeholders.size(), substitutions.size())
        );
    }

    auto subsitutions_length = std::accumulate(
        substitutions.begin(),
        substitutions.end(),
        std::size_t(0),
        [](std::size_t sum, const std::string& substitution) { return sum + substitution.size(); }
    );
    auto arbitrary_text_length = pattern_.ArbitraryTextLength();

    std::string result;
    result.reserve(arbitrary_text_length + subsitutions_length);

    auto text_chunk_iter = pattern_.text_chunks.begin();
    auto substitution_iter = substitutions.begin();

    while (substitution_iter != substitutions.end()) {
        result.append(*text_chunk_iter);
        text_chunk_iter++;
        result.append(*substitution_iter);
        substitution_iter++;
    }
    result.append(*text_chunk_iter);

    return result;
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
