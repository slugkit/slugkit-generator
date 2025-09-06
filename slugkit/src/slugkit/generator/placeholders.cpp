#include <slugkit/generator/placeholders.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/detail/pattern_parser.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/hash.hpp>
#include <slugkit/utils/set.hpp>
#include <slugkit/utils/text.hpp>

#include <boost/functional/hash.hpp>

namespace slugkit::generator {

//--------------------------------
// SizeLimit
//--------------------------------
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

//--------------------------------
// Selector
//--------------------------------
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
    if (!include_tags.empty() || !exclude_tags.empty() || size_limit.has_value() || !options.empty()) {
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
    if (!options.empty()) {
        if (result.back() != ':') {
            result += " ";
        }
        auto count = 0UL;
        for (const auto& option : options) {
            if (count > 0 && count < options.size()) {
                result += " ";
            }
            result += std::string(option.first) + "=" + std::string(option.second);
            count++;
        }
    }
    return result;
}

bool Selector::IsNSFW() const {
    return include_tags.contains("nsfw") || !exclude_tags.contains("nsfw");
}

void Selector::ApplyOptions(
    [[maybe_unused]] std::string_view original_pattern,
    [[maybe_unused]] OptionsType&& options
) {
    if (!options.empty()) {
        throw PatternSyntaxError("There are no options for dictionary selectors implemented");
    }
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

bool Matches(const Selector& selector, const Word& word) {
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
// EmojiGen
//--------------------------------
std::int64_t EmojiGen::GetHash() const {
    auto seed = StrHash(detail::PatternParser::kEmojiKeyword.data(), detail::PatternParser::kEmojiKeyword.size());
    for (const auto& tag : include_tags) {
        boost::hash_combine(seed, StrHash(tag.data(), tag.size()));
    }
    for (const auto& tag : exclude_tags) {
        boost::hash_combine(seed, StrHash(tag.data(), tag.size()));
    }
    boost::hash_combine(seed, min_count);
    boost::hash_combine(seed, max_count);
    boost::hash_combine(seed, unique);
    boost::hash_combine(seed, StrHash(tone.data(), tone.size()));
    boost::hash_combine(seed, StrHash(gender.data(), gender.size()));

    return seed;
}

std::string EmojiGen::ToString() const {
    std::string result(detail::PatternParser::kEmojiKeyword);
    if (!include_tags.empty() || !exclude_tags.empty() || has_options_) {
        result += ":";
    }
    for (const auto& tag : include_tags) {
        result += "+" + std::string(tag);
    }
    for (const auto& tag : exclude_tags) {
        result += "-" + std::string(tag);
    }
    if (min_count != 1 || max_count != 1) {
        if (result.back() != ':') {
            result += " ";
        }
        result += "count=" + std::to_string(min_count);
        if (min_count != max_count) {
            result += "-" + std::to_string(max_count);
        }
    }
    if (unique) {
        if (result.back() != ':') {
            result += " ";
        }
        result += "unique=true";
    }
    if (!tone.empty()) {
        if (result.back() != ':') {
            result += " ";
        }
        result += "tone=" + std::string(tone);
    }
    if (!gender.empty()) {
        if (result.back() != ':') {
            result += " ";
        }
        result += "gender=" + std::string(gender);
    }
    return result;
}

std::int32_t EmojiGen::Complexity() const {
    return constants::kEmojiBaseCost;
}

void EmojiGen::ApplyOptions(
    [[maybe_unused]] std::string_view original_pattern,
    [[maybe_unused]] OptionsType&& options
) {
    // remember option positions to report them in the error message
    // we need the count and unique options because there are combinations
    // that don't make sense together
    std::string_view count_option;
    std::string_view unique_option;
    for (const auto& option : options) {
        if (option.first == kCountOption) {
            auto pos = option.second.begin();
            auto range = detail::ParseRange(original_pattern, pos, option.second.end());
            if (pos != option.second.end()) {
                throw PatternSyntaxError(fmt::format(
                    "Unexpected character(s) after count option: {} at column {}",
                    std::string_view(pos, option.second.end() - pos),
                    pos - original_pattern.begin()
                ));
            }
            min_count = range.min;
            max_count = range.max;
            if (max_count == 0) {
                throw PatternSyntaxError(fmt::format(
                    "Max count for emoji generator cannot be 0: {} at column {}",
                    max_count,
                    option.second.begin() - original_pattern.begin()
                ));
            }
            if (max_count > constants::kMaxEmojiCount) {
                throw PatternSyntaxError(fmt::format(
                    "Max count for emoji generator cannot be greater than {}: {} at column {}",
                    constants::kMaxEmojiCount,
                    max_count,
                    option.second.begin() - original_pattern.begin()
                ));
            }
            count_option = std::string_view(option.first.begin(), option.second.end() - option.first.begin());
            has_options_ = true;
        } else if (option.first == kUniqueOption) {
            if (option.second == "true" || option.second == "yes") {
                unique = true;
            } else if (option.second != "false" && option.second != "no") {
                throw PatternSyntaxError(fmt::format(
                    "Unknown value for unique option: {} at column {}",
                    option.second,
                    option.second.begin() - original_pattern.begin()
                ));
            }
            unique_option = std::string_view(option.first.begin(), option.second.end() - option.first.begin());
            has_options_ = true;
        } else if (option.first == kToneOption) {
            tone = option.second;
            has_options_ = true;
        } else if (option.first == kGenderOption) {
            gender = option.second;
            has_options_ = true;
        } else {
            throw PatternSyntaxError(fmt::format(
                "Unknown option for emoji generator: {} at column {}",
                option.first,
                option.second.begin() - original_pattern.begin()
            ));
        }
    }
    if (unique && min_count == 1 && max_count == 1) {
        throw PatternSyntaxError(fmt::format(
            "Unique option cannot be used with count equal to 1: {} at column {}",
            unique_option,
            unique_option.begin() - original_pattern.begin()
        ));
    }
}

}  // namespace slugkit::generator
