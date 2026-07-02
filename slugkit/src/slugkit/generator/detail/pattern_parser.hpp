#pragma once

#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/pattern.hpp>

#include <slugkit/utils/set.hpp>
#include <slugkit/utils/text.hpp>

#include <fmt/format.h>

namespace slugkit::generator::detail {

struct IntOrRange {
    std::uint64_t min;
    std::uint64_t max;
};

auto ParseInteger(
    std::string_view original_pattern,
    std::string_view::const_iterator& pos,
    std::string_view::const_iterator e
) -> std::uint64_t;

auto ParseRange(
    std::string_view original_pattern,
    std::string_view::const_iterator& pos,
    std::string_view::const_iterator e
) -> IntOrRange;

struct PatternParser {
    enum class TokenType {
        kLbrace,
        kRbrace,
        kComma,
        kColon,         /* : */
        kEscape,        /* \ */
        kPlus,          /* + */
        kMinus,         /* - */
        kAt,            /* @ */
        kAssign,        /* = */
        kEq,            /* == */
        kLt,            /* < */
        kGt,            /* > */
        kLe,            /* <= */
        kGe,            /* >= */
        kIdentifier,    /* [a-zA-Z_][a-zA-Z0-9_]* */
        kCharSequence,  /* \S+ */
        kTag,           /* [a-zA-Z0-9_]+ */
        kNumber,        /* [0-9]+ */
        kKeywordNumber, /* number */
        kKeywordHex,    /* hex */
        kKeywordDec,    /* dec */
        kKeywordOct,    /* oct */
        kEof,           /* end of file */
    };

    struct Token {
        TokenType type;
        std::string_view value;
        std::size_t position;
    };

    struct SetSelectorLanguage {
        LanguageCodeView language;
        void operator()(Selector& selector) const {
            if (!selector.language.has_value()) {
                selector.language = language;
            }
        }
        // Global settings propagate into alternation branches' placeholders.
        void operator()(Pattern::Alternation& alternation) const {
            for (auto& group : alternation.alternatives) {
                for (auto& placeholder : group.placeholders) {
                    std::visit(*this, placeholder);
                }
            }
        }
        void operator()(auto&&) const {
        }
    };

    struct AddSelectorIncludeTag {
        TagView tag;
        void operator()(Selector& selector) const {
            if (!selector.exclude_tags.contains(tag)) {
                selector.include_tags.insert(tag);
            }
        }
        // Global settings propagate into alternation branches' placeholders.
        void operator()(Pattern::Alternation& alternation) const {
            for (auto& group : alternation.alternatives) {
                for (auto& placeholder : group.placeholders) {
                    std::visit(*this, placeholder);
                }
            }
        }
        void operator()(auto&&) const {
        }
    };

    struct AddSelectorExcludeTag {
        TagView tag;
        void operator()(Selector& selector) const {
            if (!selector.include_tags.contains(tag)) {
                selector.exclude_tags.insert(tag);
            }
        }
        // Global settings propagate into alternation branches' placeholders.
        void operator()(Pattern::Alternation& alternation) const {
            for (auto& group : alternation.alternatives) {
                for (auto& placeholder : group.placeholders) {
                    std::visit(*this, placeholder);
                }
            }
        }
        void operator()(auto&&) const {
        }
    };

    struct SetSelectorSizeLimit {
        SizeLimit size_limit;
        void operator()(Selector& selector) const {
            if (!selector.size_limit.has_value()) {
                selector.size_limit = size_limit;
            }
        }
        // Global settings propagate into alternation branches' placeholders.
        void operator()(Pattern::Alternation& alternation) const {
            for (auto& group : alternation.alternatives) {
                for (auto& placeholder : group.placeholders) {
                    std::visit(*this, placeholder);
                }
            }
        }
        void operator()(auto&&) const {
        }
    };

    using position_t = std::string_view::const_iterator;

    constexpr static char kEscapeChar = '\\';
    constexpr static char kAlternationChar = '|';
    constexpr static char kGroupOpen = '(';
    constexpr static char kGroupClose = ')';
    constexpr static std::string_view kEscapedChars = "\\{}[]|()";
    constexpr static std::string_view kNumberKeyword = "number";
    constexpr static std::string_view kNumKeword = "num";
    constexpr static std::string_view kSpecialCharKeyword = "special";
    constexpr static std::string_view kSpecKeyword = "spec";
    constexpr static std::string_view kEmojiKeyword = "emoji";

    std::string_view pattern_;
    position_t pos_;

    PatternParser(std::string_view pattern)
        : pattern_(pattern)
        , pos_(pattern_.begin()) {
    }

    bool IsEof() const {
        return pos_ == pattern_.end();
    }

    char Peek() const {
        return *pos_;
    }

    bool Match(char c) const {
        return !IsEof() && Peek() == c;
    }

    void Expect(char c) {
        if (IsEof()) {
            throw PatternSyntaxError(fmt::format(
                "Pattern parse error: unexpected end of pattern at column {}, expected `{}`", GetCurrentColumn(), c
            ));
        }
        if (Peek() != c) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: expected `{}` at column {}, got `{}`", c, GetCurrentColumn(), Peek())
            );
        }
        Next();
    }

    void Expect(std::string_view s) {
        for (char c : s) {
            Expect(c);
        }
    }

    void ExpectOneOf(std::string_view s) {
        if (IsEof()) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: unexpected end of pattern at column {}", GetCurrentColumn())
            );
        }
        for (char c : s) {
            if (Peek() == c) {
                Next();
                return;
            }
        }
        throw PatternSyntaxError(fmt::format(
            "Pattern parse error: expected one of `{}` at column {}, got `{}`", s, GetCurrentColumn(), Peek()
        ));
    }

    char Next() {
        return *pos_++;
    }

    std::size_t GetCurrentColumn() const {
        return pos_ - pattern_.begin();
    }

    void SkipWhitespace() {
        while (!IsEof() && std::isspace(Peek())) {
            Next();
        }
    }

    bool IsArbitraryText(char c) const {
        return c != '{' && c != '}' && c != '[' && c != ']' && c != kEscapeChar && c != kAlternationChar &&
               c != kGroupOpen && c != kGroupClose;
    }

    void SkipArbitraryText() {
        while (!IsEof() && IsArbitraryText(Peek())) {
            Next();
        }
    }

    Token ParseIdentifier() {
        auto start = pos_;
        if (IsEof()) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: unexpected end of pattern at column {}", GetCurrentColumn())
            );
        }
        if (!std::isalpha(Peek()) && Peek() != '_') {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: expected identifier at column {}", GetCurrentColumn())
            );
        }
        Next();
        while (!IsEof() && std::isalnum(Peek())) {
            Next();
        }
        return {
            TokenType::kIdentifier, std::string_view(start, pos_), static_cast<std::size_t>(start - pattern_.begin())
        };
    }

    Token ParseCharSequence() {
        auto start = pos_;
        while (!IsEof() && !std::isspace(Peek()) && Peek() != '}') {
            Next();
        }
        return {
            TokenType::kCharSequence, std::string_view(start, pos_), static_cast<std::size_t>(start - pattern_.begin())
        };
    }

    TagView ParseTag() {
        auto start = pos_;
        if (IsEof()) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: unexpected end of pattern at column {}", GetCurrentColumn())
            );
        }
        while (!IsEof() && (std::isalnum(Peek()) || Peek() == '_')) {
            Next();
        }
        if (start == pos_) {
            throw PatternSyntaxError(fmt::format("Pattern parse error: expected tag at column {}", GetCurrentColumn()));
        }
        return TagView(start, pos_);
    }

    std::uint64_t ParseInteger() {
        return detail::ParseInteger(pattern_, pos_, pattern_.end());
    }

    /// @brief Parses a long number base keyword from the current position.
    /// Keyword can be 'dec', 'hex', 'HEX'.
    /// @return The number base if found, otherwise throws a PatternSyntaxError.
    NumberBase ParseNumberBase() {
        auto start_column = GetCurrentColumn();
        if (Match('d')) {
            Next();
            Expect("ec");
            return NumberBase::kDec;
        }
        if (Match('h')) {
            Next();
            Expect("ex");
            return NumberBase::kHex;
        }
        if (Match('H')) {
            Next();
            Expect("EX");
            return NumberBase::kHexUpper;
        }
        if (Match('r')) {
            Next();
            Expect("oman");
            return NumberBase::kRomanLower;
        }
        if (Match('R')) {
            Next();
            Expect("OMAN");
            return NumberBase::kRoman;
        }
        throw PatternSyntaxError(fmt::format("Pattern parse error: expected number base at column {}", start_column));
    }

    NumberGen ParseNumberGen() {
        Expect(':');
        auto size = ParseInteger();
        if (size == 0) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: number size cannot be 0 at column {}", GetCurrentColumn())
            );
        }
        auto base = NumberBase::kDec;
        if (IsEof()) {
            return {static_cast<std::uint8_t>(size & 0xff), base};
        }
        if (Match('x')) {
            base = NumberBase::kHex;
            Next();
        } else if (Match('X')) {
            base = NumberBase::kHexUpper;
            Next();
        } else if (Match('d')) {
            base = NumberBase::kDec;
            Next();
        } else if (Match('R')) {
            base = NumberBase::kRoman;
            Next();
        } else if (Match('r')) {
            base = NumberBase::kRomanLower;
            Next();
        } else {
            SkipWhitespace();
            if (Match(',')) {
                Next();
                SkipWhitespace();
                base = ParseNumberBase();
            }
        }
        if (base == NumberBase::kDec && size > constants::kMaxDecimalLength) {
            throw PatternSyntaxError(fmt::format(
                "Pattern parse error: decimal number size {} exceeds limit {} at column {}",
                size,
                constants::kMaxDecimalLength,
                GetCurrentColumn()
            ));
        }
        if ((base == NumberBase::kHex || base == NumberBase::kHexUpper) && size > constants::kMaxHexLength) {
            throw PatternSyntaxError(fmt::format(
                "Pattern parse error: hex number size {} exceeds limit {} at column {}",
                size,
                constants::kMaxHexLength,
                GetCurrentColumn()
            ));
        }
        return {static_cast<std::uint8_t>(size & 0xff), base};
    }

    SpecialCharGen ParseSpecialCharGen() {
        std::uint64_t min_length = 1;
        std::uint64_t max_length = 1;
        if (Match(':')) {
            Next();
            auto range = ParseRange(pattern_, pos_, pattern_.end());
            min_length = range.min;
            max_length = range.max;
            if (min_length > constants::kMaxSpecialLength) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: special char min length {} exceeds limit {} at column {}",
                    min_length,
                    constants::kMaxSpecialLength,
                    GetCurrentColumn()
                ));
            }
            if (max_length > constants::kMaxSpecialLength) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: special char max length {} exceeds limit {} at column {}",
                    max_length,
                    constants::kMaxSpecialLength,
                    GetCurrentColumn()
                ));
            }
            if (min_length > max_length) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: special char min length {} cannot be greater than max length {} "
                    "at "
                    "column {}",
                    min_length,
                    max_length,
                    GetCurrentColumn()
                ));
            }
            if (max_length == 0) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: special char generator is useless with max length 0 at column {}",
                    GetCurrentColumn()
                ));
            }
        }
        return {static_cast<std::uint8_t>(min_length & 0xff), static_cast<std::uint8_t>(max_length & 0xff)};
    }

    SizeLimit TryParseSizeLimit() {
        auto op = CompareOperator::kNone;
        if (Match('=')) {
            Next();
            Expect('=');
            op = CompareOperator::kEq;
        } else if (Match('!')) {
            Next();
            Expect('=');
            op = CompareOperator::kNe;
        } else if (Match('>')) {
            Next();
            if (Match('=')) {
                Next();
                op = CompareOperator::kGe;
            } else {
                op = CompareOperator::kGt;
            }
        } else if (Match('<')) {
            Next();
            if (Match('=')) {
                Next();
                op = CompareOperator::kLe;
            } else {
                op = CompareOperator::kLt;
            }
        }
        if (op == CompareOperator::kNone) {
            return {op, 0};
        }
        SkipWhitespace();
        return {op, static_cast<std::uint8_t>(ParseInteger() & 0xff)};
    }

    template <typename T>
    void ParseTags(T& placeholder) {
        SkipWhitespace();
        // Parse include/exclude tags.
        while (true) {
            SkipWhitespace();
            if (Match('+')) {
                Next();
                placeholder.include_tags.insert(ParseTag());
            } else if (Match('-')) {
                Next();
                placeholder.exclude_tags.insert(ParseTag());
            } else {
                break;
            }
        }
    }

    /// @brief Parses options from the current position.
    /// @return The options if found, otherwise empty map.
    /// options are parsed until the next '}' or EOF is reached.
    /// options are key=value pairs separated by whitespace.
    Selector::OptionsType ParseOptions() {
        Selector::OptionsType options;
        while (true) {
            SkipWhitespace();
            if (Match('}')) {
                break;
            }
            if (IsEof()) {
                break;
            }
            auto key = ParseIdentifier();
            Expect('=');
            auto value = ParseCharSequence();
            options[key.value] = value.value;
        }
        return options;
    }

    void ParseSelectorModifiers(Selector& selector) {
        if (Match('@')) {
            Next();
            auto language = ParseIdentifier();
            selector.language = LanguageCodeView(language.value);
        }
        SkipWhitespace();
        if (Match(':')) {
            Next();
            // Parse include/exclude tags.
            ParseTags(selector);
            SkipWhitespace();
            if (auto size_limit = TryParseSizeLimit(); size_limit) {
                selector.size_limit = size_limit;
            }
            // This one currently will throw an exception.
            // but we need to suppport option parsing for dictionary selectors
            // so we can modify option handling later
            selector.ApplyOptions(pattern_, ParseOptions());
        }
    }

    EmojiGen ParseEmojiGen() {
        EmojiGen result;
        if (Match(':')) {
            Next();
            ParseTags(result);
            SkipWhitespace();
            result.ApplyOptions(pattern_, ParseOptions());
        }
        return result;
    }

    Selector ParseSelector(Token&& kind) {
        Selector result;
        result.kind = kind.value;
        ParseSelectorModifiers(result);
        if (auto mutex_tags = result.MutuallyExclusiveTags(); !mutex_tags.empty()) {
            auto mutex_tags_str = utils::text::Join(mutex_tags.begin(), mutex_tags.end(), ", ");
            throw PatternSyntaxError(fmt::format(
                "Pattern parse error: mutually exclusive tags at column {}: {}", kind.position, mutex_tags_str
            ));
        }
        return result;
    }

    // Widen a simple placeholder into a top-level pattern element (which also admits Alternation).
    static Pattern::PatternElement ToPatternElement(Pattern::SimplePlaceholder&& simple) {
        return std::visit(
            [](auto&& arg) -> Pattern::PatternElement { return std::forward<decltype(arg)>(arg); }, std::move(simple)
        );
    }

    // Consume arbitrary text (and escapes) into `pending`, keeping escapes raw so the canonical form
    // round-trips and generation un-escapes. Stops at any reserved character.
    void AccumulateText(std::string& pending) {
        while (!IsEof()) {
            char c = Peek();
            if (IsArbitraryText(c)) {
                pending.push_back(c);
                Next();
                continue;
            }
            if (c != kEscapeChar) {
                break;  // a reserved char handled by the caller
            }
            Next();  // consume '\'
            if (IsEof()) {
                throw PatternSyntaxError(
                    fmt::format("Pattern parse error: unexpected end of pattern at column {}", GetCurrentColumn())
                );
            }
            char escaped = Peek();
            if (kEscapedChars.find(escaped) == std::string_view::npos) {
                throw PatternSyntaxError(
                    fmt::format("Pattern parse error: invalid escape `\\{}` at column {}", escaped, GetCurrentColumn())
                );
            }
            pending.push_back(kEscapeChar);  // keep raw; FormatChunks un-escapes at generation time
            pending.push_back(escaped);
            Next();
        }
    }

    // Parse a group body (between '(' and ')'): text interleaved with placeholders. Flat -- no nested
    // groups/alternations; `(`, `|`, `[`, `]` inside must be escaped. Does not consume the ')'.
    Pattern::Group ParseGroupBody() {
        Pattern::Group group;
        std::string pending;
        while (true) {
            AccumulateText(pending);
            if (IsEof() || Match(kGroupClose)) {
                break;
            }
            if (Match('{')) {
                Next();
                group.text_chunks.push_back(std::move(pending));
                pending.clear();
                group.placeholders.push_back(ParseElement());
                Expect('}');
            } else {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: unexpected `{}` inside group at column {}; escape it for a literal",
                    Peek(),
                    GetCurrentColumn()
                ));
            }
        }
        group.text_chunks.push_back(std::move(pending));  // trailing text; text.size() == ph.size() + 1
        return group;
    }

    // Parse one alternation branch: a parenthesised group `( ... )`, or a bare placeholder `{ ... }`
    // wrapped as a single-placeholder group with no surrounding text.
    Pattern::Group ParseAlternationElement() {
        if (Match(kGroupOpen)) {
            Next();  // consume '('
            auto group = ParseGroupBody();
            Expect(kGroupClose);
            if (group.placeholders.empty() && group.text_chunks.size() == 1 && group.text_chunks[0].empty()) {
                throw PatternSyntaxError(
                    fmt::format("Pattern parse error: empty group `()` at column {}", GetCurrentColumn())
                );
            }
            return group;
        }
        Expect('{');
        Pattern::Group group;
        group.text_chunks.emplace_back();  // leading ""
        group.placeholders.push_back(ParseElement());
        Expect('}');
        group.text_chunks.emplace_back();  // trailing ""
        return group;
    }

    Pattern::SimplePlaceholder ParseElement() {
        SkipWhitespace();
        if (IsEof()) {
            return {};
        }
        // TODO: it might be faster to parse char by char
        // to avoid parsing the whole identifier and comparing it with keywords
        auto ident = ParseIdentifier();
        if (ident.value == kNumKeword) {
            return ParseNumberGen();
        } else if (ident.value == kSpecKeyword) {
            return ParseSpecialCharGen();
        } else if (ident.value == kNumberKeyword) {
            return ParseNumberGen();
        } else if (ident.value == kSpecialCharKeyword) {
            return ParseSpecialCharGen();
        } else if (ident.value == kEmojiKeyword) {
            return ParseEmojiGen();
        }
        return ParseSelector(std::move(ident));
    }

    void ParseGlobalSettings(Pattern::Placeholders& placeholders) {
        SkipWhitespace();
        if (Match('@')) {
            Next();
            auto language = ParseIdentifier();
            for (auto& placeholder : placeholders) {
                std::visit(SetSelectorLanguage{LanguageCodeView(language.value)}, placeholder);
            }
        }
        SkipWhitespace();
        while (true) {
            SkipWhitespace();
            if (Match('+')) {
                Next();
                auto tag = ParseTag();
                for (auto& placeholder : placeholders) {
                    std::visit(AddSelectorIncludeTag{tag}, placeholder);
                }
            } else if (Match('-')) {
                Next();
                auto tag = ParseTag();
                for (auto& placeholder : placeholders) {
                    std::visit(AddSelectorExcludeTag{tag}, placeholder);
                }
            } else {
                break;
            }
        }
        SkipWhitespace();
        if (auto size_limit = TryParseSizeLimit(); size_limit) {
            for (auto& placeholder : placeholders) {
                std::visit(SetSelectorSizeLimit{size_limit}, placeholder);
            }
        }
    }

    Pattern::Placeholders operator()() {
        Pattern::TextChunks text_chunks;
        return operator()(text_chunks);
    }

    Pattern::Placeholders operator()(Pattern::TextChunks& text_chunks) {
        Pattern::Placeholders result;
        std::string pending;  // arbitrary text (raw, escapes kept) awaiting the next element

        // Push an element (placeholder or alternation) preceded by the accumulated pending text.
        // Keeps text_chunks.size() == result.size() until the trailing chunk is pushed at EOF.
        auto push_element = [&](Pattern::PatternElement&& element) {
            text_chunks.push_back(pending);
            pending.clear();
            result.push_back(std::move(element));
        };
        // Pull a group up into the enclosing pattern: parentheses are transparent, so the group's
        // text and placeholders are inlined, boundary text merging with `pending`. Exact equivalence
        // to writing the group's contents unparenthesised.
        auto pull_up = [&](Pattern::Group&& group) {
            pending += group.text_chunks.front();
            for (std::size_t i = 0; i < group.placeholders.size(); ++i) {
                text_chunks.push_back(pending);
                pending = group.text_chunks[i + 1];
                result.push_back(ToPatternElement(std::move(group.placeholders[i])));
            }
        };

        while (!IsEof()) {
            AccumulateText(pending);
            if (IsEof()) {
                break;
            }
            if (Match('{') || Match(kGroupOpen)) {
                // Alternation run: `elem (| elem)*`, each branch a group `(...)` or bare placeholder.
                std::vector<Pattern::Group> branches;
                branches.push_back(ParseAlternationElement());
                auto run_end = pos_;
                SkipWhitespace();
                while (Match(kAlternationChar)) {
                    Next();  // consume '|'
                    SkipWhitespace();
                    branches.push_back(ParseAlternationElement());
                    run_end = pos_;
                    SkipWhitespace();
                }
                pos_ = run_end;  // trailing whitespace after the run is arbitrary text

                // Collapse equivalent branches (same text + same placeholder predicates, via
                // Group::GetHash): they add no variance. Order preserved (first occurrence wins).
                std::vector<Pattern::Group> distinct;
                for (auto& branch : branches) {
                    auto hash = branch.GetHash();
                    bool duplicate = false;
                    for (const auto& kept : distinct) {
                        if (kept.GetHash() == hash) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        distinct.push_back(std::move(branch));
                    }
                }

                if (distinct.size() == 1) {
                    // Lone group / all-equivalent alternation: pull up (parentheses are transparent).
                    pull_up(std::move(distinct.front()));
                } else {
                    push_element(Pattern::Alternation{std::move(distinct)});
                }
            } else if (Match('[')) {
                Next();
                ParseGlobalSettings(result);
                Expect(']');
                SkipWhitespace();
                if (!IsEof()) {
                    throw PatternSyntaxError(
                        fmt::format("Pattern parse error: unexpected character at column {}", GetCurrentColumn())
                    );
                }
            } else if (Match(kAlternationChar)) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: unexpected `|` at column {}; alternation must be between "
                    "placeholders or groups; escape as `\\|` for a literal pipe",
                    GetCurrentColumn()
                ));
            } else if (Match(kGroupClose)) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: unmatched `)` at column {}; escape as `\\)` for a literal", GetCurrentColumn()
                ));
            } else {
                throw PatternSyntaxError(
                    fmt::format("Pattern parse error: unexpected character at column {}", GetCurrentColumn())
                );
            }
        }
        text_chunks.push_back(std::move(pending));  // final chunk; text_chunks.size()==result.size()+1
        return result;
    }
};
}  // namespace slugkit::generator::detail
