#pragma once

#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/pattern.hpp>

#include <slugkit/utils/set.hpp>
#include <slugkit/utils/text.hpp>

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
        std::string_view language;
        void operator()(Selector& selector) const {
            if (!selector.language.has_value()) {
                selector.language = std::string_view(language);
            }
        }
        void operator()(auto&&) const {
        }
    };

    struct AddSelectorIncludeTag {
        std::string_view tag;
        void operator()(Selector& selector) const {
            if (!selector.exclude_tags.contains(tag)) {
                selector.include_tags.insert(tag);
            }
        }
        void operator()(auto&&) const {
        }
    };

    struct AddSelectorExcludeTag {
        std::string_view tag;
        void operator()(Selector& selector) const {
            if (!selector.include_tags.contains(tag)) {
                selector.exclude_tags.insert(tag);
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
        void operator()(auto&&) const {
        }
    };

    using position_t = std::string_view::const_iterator;

    constexpr static char kEscapeChar = '\\';
    constexpr static std::string_view kEscapedChars = "\\{}[]";
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
        return c != '{' && c != '}' && c != '[' && c != ']' && c != kEscapeChar;
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

    std::string_view ParseTag() {
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
        return std::string_view(start, pos_);
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
            selector.language = std::string_view(language.value);
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

    Pattern::PatternElement ParseElement() {
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
                std::visit(SetSelectorLanguage{language.value}, placeholder);
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
        auto arbitrary_start = pos_;
        auto arbitrary_text_end = pattern_.end();
        while (!IsEof()) {
            SkipArbitraryText();
            if (IsEof()) {
                text_chunks.push_back(std::string_view(arbitrary_start, pos_));
                break;
            }
            if (Match('{')) {
                // We push arbitrary text before the placeholder to the text chunks.
                // Empty text chunks are also pushed.
                // Postcondition: text_chunks.size() == result.size() + 1.
                text_chunks.push_back(std::string_view(arbitrary_start, pos_));
                Next();
                auto element = ParseElement();
                result.push_back(std::move(element));
                Expect('}');
                arbitrary_start = pos_;
            } else if (Match('[')) {
                arbitrary_text_end = pos_;
                Next();
                ParseGlobalSettings(result);
                Expect(']');
                SkipWhitespace();
                // Expect EOF
                if (!IsEof()) {
                    throw PatternSyntaxError(
                        fmt::format("Pattern parse error: unexpected character at column {}", GetCurrentColumn())
                    );
                }
            } else if (Match(kEscapeChar)) {
                Next();
                if (IsEof()) {
                    throw PatternSyntaxError(
                        fmt::format("Pattern parse error: unexpected end of pattern at column {}", GetCurrentColumn())
                    );
                }
                ExpectOneOf(kEscapedChars);
                // TODO handle escaped characters in substitutions.
            } else {
                throw PatternSyntaxError(
                    fmt::format("Pattern parse error: unexpected character at column {}", GetCurrentColumn())
                );
            }
        }
        if (text_chunks.size() == result.size()) {
            text_chunks.push_back(std::string_view(arbitrary_start, arbitrary_text_end));
        }
        return result;
    }
};
}  // namespace slugkit::generator::detail
