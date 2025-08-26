#include <slugkit/generator/pattern.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/utils/set.hpp>
#include <slugkit/utils/text.hpp>

#include <userver/storages/postgres/detail/string_hash.hpp>

#include <boost/functional/hash.hpp>

#include <numeric>

namespace slugkit::generator {

namespace {

std::size_t StrHash(const char* str, std::size_t len) {
    auto seed = len;
    boost::hash_range(seed, str, str + len);
    return seed;
}

}  // namespace

std::int64_t SizeLimit::GetHash() const {
    auto seed = static_cast<std::size_t>(op);
    boost::hash_combine(seed, value);
    return seed;
}

bool SizeLimit::Matches(std::size_t lhs) const {
    switch (op) {
        case CompareOperator::kEq:
            return lhs == value;
        case CompareOperator::kNe:
            return lhs != value;
        case CompareOperator::kLt:
            return lhs < value;
        case CompareOperator::kLe:
            return lhs <= value;
        case CompareOperator::kGt:
            return lhs > value;
        case CompareOperator::kGe:
            return lhs >= value;
        default:
            return false;
    }
}

std::int64_t Selector::GetHash() const {
    auto seed = StrHash(kind.data(), kind.size());
    if (language.has_value()) {
        boost::hash_combine(seed, StrHash(language->data(), language->size()));
    }
    for (const auto& tag : include_tags) {
        boost::hash_combine(seed, StrHash(tag.data(), tag.size()));
    }
    for (const auto& exclude_tag : exclude_tags) {
        boost::hash_combine(seed, StrHash(exclude_tag.data(), exclude_tag.size()));
    }
    if (size_limit.has_value()) {
        boost::hash_combine(seed, size_limit->GetHash());
    }
    for (const auto& option : options) {
        boost::hash_combine(seed, StrHash(option.first.data(), option.first.size()));
        boost::hash_combine(seed, StrHash(option.second.data(), option.second.size()));
    }
    return seed;
}

CaseType Selector::GetCase() const {
    if (case_type_ != CaseType::kNone) {
        return case_type_;
    }
    // TODO get locale based on language
    const auto locale = utils::text::kEnUsLocale;
    if (kind == utils::text::ToLower(kind, locale)) {
        return CaseType::kLower;
    }
    if (kind == utils::text::ToUpper(kind, locale)) {
        return CaseType::kUpper;
    }
    if (kind == utils::text::Capitalize(kind, locale)) {
        return CaseType::kTitle;
    }
    case_type_ = CaseType::kMixed;
    return case_type_;
}

std::optional<std::size_t> Selector::GetMaxLength() const {
    if (!size_limit.has_value()) {
        return std::nullopt;
    }
    switch (size_limit->op) {
        case CompareOperator::kLt:
            return static_cast<std::size_t>(size_limit->value);
        case CompareOperator::kLe:
            return static_cast<std::size_t>(size_limit->value) - 1;
        case CompareOperator::kEq:
            return static_cast<std::size_t>(size_limit->value);
        default:
            return std::nullopt;
    }
}

bool Selector::LimitsMaxLength() const {
    if (!size_limit.has_value()) {
        return false;
    }
    switch (size_limit->op) {
        case CompareOperator::kLt:
            return true;
        case CompareOperator::kLe:
            return true;
        case CompareOperator::kEq:
            return true;
        default:
            return false;
    }
}

std::string Selector::ToString() const {
    auto result = std::string(kind);
    if (language.has_value()) {
        result += "@" + std::string(*language);
    }
    if (!include_tags.empty() || !exclude_tags.empty() || size_limit.has_value()) {
        result += ":";
    }
    if (!include_tags.empty()) {
        std::vector<std::string> sorted_tags(include_tags.begin(), include_tags.end());
        std::sort(sorted_tags.begin(), sorted_tags.end());
        for (const auto& tag : sorted_tags) {
            result += "+" + std::string(tag);
        }
    }
    if (!exclude_tags.empty()) {
        std::vector<std::string> sorted_tags(exclude_tags.begin(), exclude_tags.end());
        std::sort(sorted_tags.begin(), sorted_tags.end());
        for (const auto& tag : sorted_tags) {
            result += "-" + std::string(tag);
        }
    }
    if (size_limit.has_value()) {
        switch (size_limit->op) {
            case CompareOperator::kEq:
                result += "==" + std::to_string(size_limit->value);
                break;
            case CompareOperator::kNe:
                result += "!=" + std::to_string(size_limit->value);
                break;
            case CompareOperator::kLt:
                result += "<" + std::to_string(size_limit->value);
                break;
            case CompareOperator::kLe:
                result += "<=" + std::to_string(size_limit->value);
                break;
            case CompareOperator::kGt:
                result += ">" + std::to_string(size_limit->value);
                break;
            case CompareOperator::kGe:
                result += ">=" + std::to_string(size_limit->value);
                break;
            default:
                break;
        }
    }
    return result;
}

bool Selector::IsNSFW() const {
    return include_tags.contains("nsfw") || !exclude_tags.contains("nsfw");
}

bool Matches(const Selector& selector, const Word& word, bool skip_dictionary_check) {
    if (!skip_dictionary_check) {
        if (selector.kind != word.kind) {
            return false;
        }
        if (selector.language.has_value() && selector.language != word.language) {
            return false;
        }
    }
    if (!selector.include_tags.empty()) {
        if (!utils::IsSubset(selector.include_tags, word.tags)) {
            return false;
        }
    }
    if (!selector.exclude_tags.empty()) {
        if (utils::Intersects(selector.exclude_tags, word.tags)) {
            return false;
        }
    }
    if (selector.size_limit.has_value()) {
        return selector.size_limit->Matches(word.word.size());
    }
    return true;
}

std::int32_t Selector::Complexity() const {
    std::int32_t cost = constants::kDictionaryBaseCost;
    cost += constants::kDictionaryTagCost * (include_tags.size() + exclude_tags.size());
    if (size_limit.has_value()) {
        cost += constants::kDictionaryLengthCost;
        if (!include_tags.empty() || !exclude_tags.empty()) {
            cost += constants::kDictionaryTagAndLengthCost;
        }
    }
    switch (GetCase()) {
        case CaseType::kUpper:
            cost += constants::kDictionaryUpperCaseCost;
            break;
        case CaseType::kTitle:
            cost += constants::kDictionaryTitleCaseCost;
            break;
        case CaseType::kMixed:
            cost += constants::kDictionaryMixedCaseCost;
            break;
        default:
            break;
    }
    return cost;
}

//--------------------------------
// NumberGen
//--------------------------------
std::int64_t NumberGen::GetHash() const {
    auto seed = static_cast<std::size_t>(base);
    boost::hash_combine(seed, max_length);
    return seed;
}

std::string NumberGen::ToString() const {
    char base_char = 'd';
    switch (base) {
        case NumberBase::kDec:
            base_char = 'd';
            break;
        case NumberBase::kHex:
            base_char = 'x';
            break;
        case NumberBase::kHexUpper:
            base_char = 'X';
            break;
        case NumberBase::kRoman:
            base_char = 'R';
            break;
        case NumberBase::kRomanLower:
            base_char = 'r';
    }
    return fmt::format("number:{}{}", max_length, base_char);
}

//--------------------------------
// SpecialCharGen
//--------------------------------
std::int64_t SpecialCharGen::GetHash() const {
    auto seed = static_cast<std::size_t>(min_length);
    boost::hash_combine(seed, max_length);
    return seed;
}

std::string SpecialCharGen::ToString() const {
    if (min_length == max_length) {
        return fmt::format("special:{}", min_length);
    }
    return fmt::format("special:{}-{}", min_length, max_length);
}

std::int32_t SpecialCharGen::Complexity() const {
    std::int32_t cost =
        constants::kSpecialCharBaseCost + std::max(0, min_length - 2) * constants::kSpecialCharLengthCost;
    if (min_length != max_length) {
        cost += (max_length - min_length) * constants::kSpecialCharVariableLengthCost;
    }
    return cost;
}

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
    auto seed = StrHash(pattern.data(), pattern.size());
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

namespace detail {

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

    std::string_view ParseTag() {
        auto start = pos_;
        if (IsEof()) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: unexpected end of pattern at column {}", GetCurrentColumn())
            );
        }
        while (!IsEof() && std::isalnum(Peek())) {
            Next();
        }
        if (start == pos_) {
            throw PatternSyntaxError(fmt::format("Pattern parse error: expected tag at column {}", GetCurrentColumn()));
        }
        return std::string_view(start, pos_);
    }

    std::uint64_t ParseInteger() {
        auto start = pos_;
        while (!IsEof() && std::isdigit(Peek())) {
            Next();
        }
        if (start == pos_) {
            throw PatternSyntaxError(
                fmt::format("Pattern parse error: expected number at column {}", GetCurrentColumn())
            );
        }
        return std::stoull(std::string(start, pos_));
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
            min_length = ParseInteger();
            if (min_length > constants::kMaxSpecialLength) {
                throw PatternSyntaxError(fmt::format(
                    "Pattern parse error: special char min length {} exceeds limit {} at column {}",
                    min_length,
                    constants::kMaxSpecialLength,
                    GetCurrentColumn()
                ));
            }
            if (Match('-')) {
                Next();
                max_length = ParseInteger();
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
            } else {
                max_length = min_length;
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
            while (true) {
                SkipWhitespace();
                if (Match('+')) {
                    Next();
                    selector.include_tags.insert(ParseTag());
                } else if (Match('-')) {
                    Next();
                    selector.exclude_tags.insert(ParseTag());
                } else {
                    break;
                }
            }
            SkipWhitespace();
            if (auto size_limit = TryParseSizeLimit(); size_limit) {
                selector.size_limit = size_limit;
            }
            // TODO parse options.
        }
    }

    Selector ParseSelector(Token&& kind) {
        Selector result;
        result.kind = kind.value;
        ParseSelectorModifiers(result);
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

}  // namespace detail

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

Pattern operator""_pattern(const char* str, std::size_t size) {
    return Pattern(std::string(str, size));
}

PatternPtr operator""_pattern_ptr(const char* str, std::size_t size) {
    return std::make_shared<Pattern>(std::string(str, size));
}

}  // namespace literals

}  // namespace slugkit::generator
