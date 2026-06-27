#include <slugkit/generator/pattern_generator.hpp>

#include <slugkit/generator/constants.hpp>
#include <slugkit/generator/exceptions.hpp>
#include <slugkit/generator/permutations.hpp>
#include <slugkit/generator/structured_loader.hpp>
#include <slugkit/utils/primes.hpp>
#include <slugkit/utils/roman.hpp>
#include <slugkit/utils/text.hpp>

#include <cstdint>
#include <stdexcept>

namespace slugkit::generator {

namespace {

using RomanNumerals = std::array<std::string, 3999>;
using FilteredRomanNumerals = std::vector<RomanNumerals::const_iterator>;

constexpr std::uint32_t kSeedStep = 2083;  // just a random prime number

}  // namespace

struct FilteredRomanDictionary {
    FilteredRomanDictionary(const RomanNumerals& roman_dictionary, const std::size_t max_length)
        : max_length{max_length} {
        for (auto it = roman_dictionary.begin(); it != roman_dictionary.end(); ++it) {
            if (it->size() <= max_length) {
                filtered_numerals.push_back(it);
            }
        }
    }

    std::size_t GetCapacity() const {
        return filtered_numerals.size();
    }

    std::size_t max_length;
    FilteredRomanNumerals filtered_numerals;
};

namespace {
// 32 special symbols, so we can use hex numbers
// to get stable permutations
constexpr auto kSpecialSymbols = "!@#$%^&*()_+-=[]{}|;:,.<>?'\"~/\\?";
constexpr auto kSpecialSymbolsCount = 32;

struct RomanDictionary {
    RomanDictionary() {
        for (int i = 1; i <= 3999; ++i) {
            numerals[i - 1] = utils::roman::ToRoman(i);
        }
    }

    RomanNumerals numerals;

    FilteredRomanDictionaryPtr Filter(const NumberGen& selector) const {
        return std::make_shared<FilteredRomanDictionary>(numerals, selector.max_length);
    }
};

const RomanDictionary kRomanDictionary;

}  // namespace

//-------------------------------------------------------------
// SelectorSubstitutionGenerator
//-------------------------------------------------------------
SelectorSubstitutionGenerator::SelectorSubstitutionGenerator(
    FilteredDictionaryConstPtr dictionary,
    const SelectorSettings& settings
)
    : dictionary_{dictionary}
    , selected_size_{settings.selected_size} {
}

std::string SelectorSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto index = Permute(selected_size_, seed, sequence_number);
    if (dictionary_->GetCase() == CaseType::kMixed) {
        // For mixed case selected_size_ is the mixed-case capacity (sum of every word's case
        // space). Decompose the permuted value into a word and its case mask so each sequence
        // value maps to a distinct cased slug -- collision-free, unlike a uniform per-word mask.
        auto [word_index, case_index] = dictionary_->DecomposeMixed(static_cast<std::uint64_t>(index));
        const auto& word = dictionary_->GetWord(word_index).word;
        auto mask = utils::text::ExpandCaseMask(word, case_index);
        return utils::text::MixedCase(word, utils::text::kEnUsLocale, mask);
    }
    return (*dictionary_)[index];
}

std::size_t SelectorSubstitutionGenerator::GetMaxLength() const {
    return dictionary_->GetMaxLength();
}

//-------------------------------------------------------------
// BinarySelectorSubstitutionGenerator
//-------------------------------------------------------------
BinarySelectorSubstitutionGenerator::BinarySelectorSubstitutionGenerator(
    binary::FilteredDictionaryConstPtr dictionary,
    const SelectorSettings& settings
)
    : dictionary_{std::move(dictionary)}
    , selected_size_{settings.selected_size} {
}

std::string BinarySelectorSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto index = Permute(selected_size_, seed, sequence_number);
    // The dictionary stores precomputed case variants, so pick the matching one instead of
    // converting at generation time. kMixed decomposes the permuted value into a word and its
    // case mask (selected_size_ is the mixed-case capacity), then applies the lowercase variant.
    if (dictionary_->GetCase() == CaseType::kMixed) {
        auto [word_index, case_index] = dictionary_->DecomposeMixed(static_cast<std::uint64_t>(index));
        const auto& entry = (*dictionary_)[IndexType(static_cast<IndexType::UnderlyingType>(word_index))];
        auto word = entry.Lowercase();
        auto mask = utils::text::ExpandCaseMask(word, case_index);
        return utils::text::MixedCase(word, utils::text::kEnUsLocale, mask);
    }
    const auto& entry = (*dictionary_)[IndexType(static_cast<IndexType::UnderlyingType>(index))];
    switch (dictionary_->GetCase()) {
        case CaseType::kUpper:
            return std::string{entry.Uppercase()};
        case CaseType::kTitle:
            return std::string{entry.Titlecase()};
        case CaseType::kNone:
        case CaseType::kLower:
        case CaseType::kMixed:  // handled above
        default:
            return std::string{entry.Lowercase()};
    }
}

std::size_t BinarySelectorSubstitutionGenerator::GetMaxLength() const {
    return dictionary_->GetMaxLength();
}

//-------------------------------------------------------------
// NumberSubstitutionGenerator
//-------------------------------------------------------------
NumberSubstitutionGenerator::NumberSubstitutionGenerator(const NumberGen& number_gen)
    : base_{number_gen.base}
    , max_length_{static_cast<std::uint32_t>(number_gen.max_length)} {
    if (base_ == NumberBase::kRoman) {
        throw std::runtime_error("Roman numbers are supposed to be substituted by a separate generator");
    }
    if (base_ == NumberBase::kDec && max_length_ > constants::kMaxDecimalLength) {
        throw std::runtime_error("Decimal number length is too long");
    } else if ((base_ == NumberBase::kHex || base_ == NumberBase::kHexUpper) &&
               max_length_ > constants::kMaxHexLength) {
        throw std::runtime_error("Hex number length is too long");
    }
}

std::string NumberSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    std::uint64_t max_value = 0;
    switch (base_) {
        case NumberBase::kDec:
            max_value = static_cast<std::uint64_t>(std::pow(10, max_length_));
            break;
        case NumberBase::kHex:
        case NumberBase::kHexUpper:
            if (max_length_ < constants::kMaxHexLength) {
                max_value = 1ULL << (max_length_ * 4);
            }
            break;
        case NumberBase::kRoman:
        case NumberBase::kRomanLower:
            throw std::runtime_error("Roman numbers are supposed to be substituted by a separate generator");
            break;
    }
    // Permute dispatches correctly to PermutePowerOf2 or Permute
    auto value = Permute(max_value, seed, sequence_number);
    if (base_ == NumberBase::kDec) {
        return fmt::format("{:0{}d}", value, max_length_);
    }
    if (base_ == NumberBase::kHex) {
        return fmt::format("{:0{}x}", value, max_length_);
    }
    return fmt::format("{:0{}X}", value, max_length_);
}

numeric::BigInt NumberSubstitutionGenerator::GetCapacity() const {
    switch (base_) {
        case NumberBase::kDec:
            return numeric::BigInt(static_cast<std::size_t>(std::pow(10, max_length_)));
        case NumberBase::kHex:
        case NumberBase::kHexUpper:
            return numeric::BigInt{1ULL} << (max_length_ * 4);
        default:
            throw std::runtime_error("Roman numbers are supposed to be substituted by a separate generator");
    }
}

//-------------------------------------------------------------
// RomanSubstitutionGenerator
//-------------------------------------------------------------
RomanSubstitutionGenerator::RomanSubstitutionGenerator(const NumberGen& number_gen)
    : roman_dictionary_{kRomanDictionary.Filter(number_gen)}
    , base_{number_gen.base}
    , max_length_{static_cast<std::uint32_t>(number_gen.max_length)} {
}

std::string RomanSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto index = Permute(roman_dictionary_->GetCapacity(), seed, sequence_number);
    auto value = roman_dictionary_->filtered_numerals[index];
    if (base_ == NumberBase::kRomanLower) {
        return utils::text::ToLower(*value, utils::text::kEnUsLocale);
    }
    return *value;
}

numeric::BigInt RomanSubstitutionGenerator::GetCapacity() const {
    return numeric::BigInt(roman_dictionary_->GetCapacity());
}

//-------------------------------------------------------------
// SpecialSubstitutionGenerator
//-------------------------------------------------------------
SpecialSubstitutionGenerator::SpecialSubstitutionGenerator(const SpecialCharGen& special_gen)
    : min_length_{static_cast<std::uint32_t>(special_gen.min_length)}
    , max_length_{static_cast<std::uint32_t>(special_gen.max_length)} {
    if (min_length_ > constants::kMaxSpecialLength) {
        throw std::runtime_error(fmt::format("Min special symbols length is {}", constants::kMaxSpecialLength));
    }
    if (max_length_ > constants::kMaxSpecialLength) {
        throw std::runtime_error(fmt::format("Max special symbols length is {}", constants::kMaxSpecialLength));
    }
    if (min_length_ > max_length_) {
        throw std::runtime_error("Min special symbols length is greater than max special symbols length");
    }
    cumulative_caps_.resize(max_length_ - min_length_ + 1);
    for (std::size_t i = 0; i < cumulative_caps_.size(); ++i) {
        cumulative_caps_[i] = 1ULL << ((i + min_length_) * 5);
    }
}

std::size_t SpecialSubstitutionGenerator::SelectLength(std::uint32_t seed, std::size_t sequence_number) const {
    auto p = Permute(cumulative_caps_.back(), seed, sequence_number);
    auto it = std::upper_bound(cumulative_caps_.begin(), cumulative_caps_.end(), p);
    auto idx = std::distance(cumulative_caps_.begin(), it);
    return min_length_ + idx;
}

std::string SpecialSubstitutionGenerator::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    std::size_t length = min_length_;
    if (min_length_ != max_length_) {
        // Select a length between min_length_ and max_length_
        // the probability of getting a lenght must be proportional to the length
        length = SelectLength(seed, sequence_number);
    }

    if (length == 0) {
        return "";
    }

    auto index = Permute(1ULL << (length * 5), seed, sequence_number);
    // Now we use each 5 bits to get a special symbol
    std::string result;
    result.resize(length);
    for (std::size_t i = 0; i < length; ++i) {
        result[i] = kSpecialSymbols[index % kSpecialSymbolsCount];
        index /= kSpecialSymbolsCount;
    }
    return result;
}

numeric::BigInt SpecialSubstitutionGenerator::GetCapacity() const {
    return std::accumulate(cumulative_caps_.begin(), cumulative_caps_.end(), numeric::BigInt{0});
}

//-------------------------------------------------------------
// EmojiSubstitutionGeneratorBase
//-------------------------------------------------------------

EmojiSubstitutionGeneratorBase::EmojiSubstitutionGeneratorBase(std::size_t dict_size, const EmojiGen& emoji_gen)
    : dict_size_{dict_size}
    , min_count_{static_cast<std::size_t>(emoji_gen.min_count)}
    , max_count_{static_cast<std::size_t>(emoji_gen.max_count)}
    , unique_{emoji_gen.unique}
    , tone_{emoji_gen.tone}
    , gender_{emoji_gen.gender}
    , cumulative_caps_{} {
    cumulative_caps_.resize(max_count_ - min_count_ + 1);
    if (max_count_ > constants::kMaxEmojiCount) {
        throw DictionaryError("Max count for emoji generator cannot be greater than 16");
    }
    if (unique_) {
        if (dict_size_ < min_count_) {
            throw DictionaryError("Not enough emoji to generate a unique string");
        }
        if (dict_size_ < max_count_) {
            // Adjust max_count_ to the size of the dictionary
            max_count_ = dict_size_;
        }
        for (std::size_t i = 0; i < cumulative_caps_.size(); ++i) {
            cumulative_caps_[i] = UniquePermutationCount(dict_size_, i + min_count_);
        }
    } else {
        for (std::size_t i = 0; i < cumulative_caps_.size(); ++i) {
            cumulative_caps_[i] = PermutationCount(dict_size_, i + min_count_);
        }
    }
}

std::size_t EmojiSubstitutionGeneratorBase::SelectCount(std::uint32_t seed, std::size_t sequence_number) const {
    if (min_count_ == max_count_) {
        return min_count_;
    }
    auto p = Permute(cumulative_caps_.back(), seed, sequence_number);
    auto it = std::upper_bound(cumulative_caps_.begin(), cumulative_caps_.end(), p);
    auto idx = std::distance(cumulative_caps_.begin(), it);
    return min_count_ + idx;
}

std::string EmojiSubstitutionGeneratorBase::Generate(std::uint32_t seed, std::size_t sequence_number) const {
    auto count = SelectCount(seed, sequence_number);
    auto permutation = unique_ ? UniquePermutation(seed, dict_size_, count, sequence_number)
                               : NonUniquePermutation(seed, dict_size_, count, sequence_number);
    std::string result;
    result.reserve(constants::kEmojiMaxCharLength * count);
    for (const auto& item : permutation) {
        result += EmojiAt(item);
    }
    return result;
}

numeric::BigInt EmojiSubstitutionGeneratorBase::GetCapacity() const {
    return std::accumulate(cumulative_caps_.begin(), cumulative_caps_.end(), numeric::BigInt{0});
}

//-------------------------------------------------------------
// PatternGenerator::Impl
//-------------------------------------------------------------
namespace {

// Build the selector substitution generator appropriate to the filtered-dictionary type, so
// the otherwise-identical settings/init logic can be shared between the in-memory and binary
// dictionary sets.
auto MakeSelectorGenerator(FilteredDictionaryConstPtr dictionary, const SelectorSettings& settings)
    -> SubstitutionGeneratorPtr {
    return std::make_unique<SelectorSubstitutionGenerator>(std::move(dictionary), settings);
}

auto MakeSelectorGenerator(binary::FilteredDictionaryConstPtr dictionary, const SelectorSettings& settings)
    -> SubstitutionGeneratorPtr {
    return std::make_unique<BinarySelectorSubstitutionGenerator>(std::move(dictionary), settings);
}

// Build the emoji substitution generator appropriate to the filtered-dictionary type. Emoji is a
// regular dictionary kind now, sourced from the dictionary set like any selector.
auto MakeEmojiGenerator(FilteredDictionaryConstPtr dictionary, const EmojiGen& emoji_gen) -> SubstitutionGeneratorPtr {
    return std::make_unique<EmojiSubstitutionGenerator>(std::move(dictionary), emoji_gen);
}

auto MakeEmojiGenerator(binary::FilteredDictionaryConstPtr dictionary, const EmojiGen& emoji_gen)
    -> SubstitutionGeneratorPtr {
    return std::make_unique<BinaryEmojiSubstitutionGenerator>(std::move(dictionary), emoji_gen);
}

// The emoji placeholder filters the "emoji" dictionary kind by its include/exclude tags, exactly
// like a selector; build the equivalent Selector so the shared DictionarySet::Filter path applies.
auto EmojiSelector(const EmojiGen& emoji_gen) -> Selector {
    Selector selector;
    selector.kind = constants::kEmojiKind;
    selector.include_tags = emoji_gen.include_tags;
    selector.exclude_tags = emoji_gen.exclude_tags;
    return selector;
}

}  // namespace

struct PatternGenerator::Impl {
    PatternPtr pattern;
    std::string seed;
    std::vector<SubstitutionGeneratorPtr> generators;
    PatternSettings settings;

    // Constructor for the case when settings are not calculated yet for the pattern
    Impl(const DictionarySet& dictionaries, PatternPtr pattern)
        : pattern{pattern}
        , generators{}
        , settings{CalculateSettings(dictionaries)} {
        //
    }

    // Constructor for the case when settings are provided
    Impl(const DictionarySet& dictionaries, PatternPtr pattern, PatternSettings settings)
        : pattern{pattern}
        , generators{}
        , settings{settings} {
        InitGenerators(dictionaries);
    }

    // Binary dictionary set overloads: the binary dictionaries reproduce the in-memory
    // dictionaries' lexicographic order, so the same settings/init logic applies and the
    // generated slugs are byte-identical.
    Impl(const binary::DictionarySet& dictionaries, PatternPtr pattern)
        : pattern{pattern}
        , generators{}
        , settings{CalculateSettings(dictionaries)} {
        //
    }

    Impl(const binary::DictionarySet& dictionaries, PatternPtr pattern, PatternSettings settings)
        : pattern{pattern}
        , generators{}
        , settings{settings} {
        InitGenerators(dictionaries);
    }

    // This function has a side effect of initializing the generators
    template <typename DictSet>
    PatternSettings CalculateSettings(const DictSet& dictionaries) {
        numeric::BigInt capacity(1);
        std::size_t max_pattern_length = pattern->ArbitraryTextLength();

        std::vector<SelectorSettings> selectors;
        using FilteredPtr = decltype(dictionaries.Filter(std::declval<const Selector&>()));
        std::map<std::int64_t, FilteredPtr> filtered_dictionaries;

        for (const auto& element : pattern->placeholders) {
            if (std::holds_alternative<Selector>(element)) {
                const auto& selector = std::get<Selector>(element);
                auto hash = selector.GetHash();
                auto f = filtered_dictionaries.find(hash);
                if (f == filtered_dictionaries.end()) {
                    f = filtered_dictionaries.emplace(hash, dictionaries.Filter(selector)).first;
                }
                auto filtered_dict = f->second;
                if (!filtered_dict || filtered_dict->empty()) {
                    throw PatternSyntaxError("No matching words found for: " + selector.ToString());
                }
                auto original_size = filtered_dict->size();
                SelectorSettings settings{
                    static_cast<std::int64_t>(original_size), static_cast<std::int64_t>(original_size)
                };
                if (filtered_dict->GetCase() == CaseType::kMixed) {
                    // Mixed case multiplies a word's capacity by its number of cased forms; use the
                    // collision-free block layout's total instead of the plain word count.
                    settings.selected_size = static_cast<std::int64_t>(filtered_dict->MixedCapacity());
                } else {
                    // use primes to maximize capacity
                    auto original_capacity = lcm(capacity, numeric::BigInt(original_size));
                    if (original_size > 2) {
                        auto prime = utils::PrevPrime(original_size);
                        auto prime_capacity = lcm(capacity, numeric::BigInt(prime));
                        if (prime_capacity > original_capacity) {
                            settings.selected_size = prime;
                        }
                    }
                }
                selectors.push_back(settings);
                generators.push_back(MakeSelectorGenerator(filtered_dict, settings));
            } else if (std::holds_alternative<NumberGen>(element)) {
                const auto& number_gen = std::get<NumberGen>(element);
                if (number_gen.base == NumberBase::kRoman || number_gen.base == NumberBase::kRomanLower) {
                    generators.push_back(std::make_unique<RomanSubstitutionGenerator>(number_gen));
                } else {
                    generators.push_back(std::make_unique<NumberSubstitutionGenerator>(number_gen));
                }
            } else if (std::holds_alternative<SpecialCharGen>(element)) {
                const auto& special_gen = std::get<SpecialCharGen>(element);
                generators.push_back(std::make_unique<SpecialSubstitutionGenerator>(special_gen));
            } else if (std::holds_alternative<EmojiGen>(element)) {
                const auto& emoji_gen = std::get<EmojiGen>(element);
                auto emoji_dict = dictionaries.Filter(EmojiSelector(emoji_gen));
                if (!emoji_dict || emoji_dict->empty()) {
                    throw PatternSyntaxError("No matching emoji found for: " + emoji_gen.ToString());
                }
                generators.push_back(MakeEmojiGenerator(emoji_dict, emoji_gen));
            }
            capacity = lcm(capacity, generators.back()->GetCapacity());
            max_pattern_length += generators.back()->GetMaxLength();
        }
        return PatternSettings{selectors, capacity, static_cast<std::uint8_t>(max_pattern_length)};
    }

    template <typename DictSet>
    void InitGenerators(const DictSet& dictionaries) {
        auto selector_settings = settings.selectors.begin();
        numeric::BigInt capacity{1};
        std::size_t max_pattern_length = pattern->ArbitraryTextLength();
        for (const auto& element : pattern->placeholders) {
            if (std::holds_alternative<Selector>(element)) {
                if (selector_settings == settings.selectors.end()) {
                    throw std::runtime_error("Incorrect pattern settings");
                }
                const auto& selector = std::get<Selector>(element);
                auto filtered_dict = dictionaries.Filter(selector);
                if (!filtered_dict || filtered_dict->empty()) {
                    throw PatternSyntaxError("No matching words found for: " + selector.ToString());
                }
                generators.push_back(MakeSelectorGenerator(filtered_dict, *selector_settings));
                ++selector_settings;
            } else if (std::holds_alternative<NumberGen>(element)) {
                const auto& number_gen = std::get<NumberGen>(element);
                if (number_gen.base == NumberBase::kRoman || number_gen.base == NumberBase::kRomanLower) {
                    generators.push_back(std::make_unique<RomanSubstitutionGenerator>(number_gen));
                } else {
                    generators.push_back(std::make_unique<NumberSubstitutionGenerator>(number_gen));
                }
            } else if (std::holds_alternative<SpecialCharGen>(element)) {
                const auto& special_gen = std::get<SpecialCharGen>(element);
                generators.push_back(std::make_unique<SpecialSubstitutionGenerator>(special_gen));
            } else if (std::holds_alternative<EmojiGen>(element)) {
                const auto& emoji_gen = std::get<EmojiGen>(element);
                auto emoji_dict = dictionaries.Filter(EmojiSelector(emoji_gen));
                if (!emoji_dict || emoji_dict->empty()) {
                    throw PatternSyntaxError("No matching emoji found for: " + emoji_gen.ToString());
                }
                generators.push_back(MakeEmojiGenerator(emoji_dict, emoji_gen));
            }
            capacity = lcm(capacity, generators.back()->GetCapacity());
            max_pattern_length += generators.back()->GetMaxLength();
        }
        settings.capacity = capacity;
        settings.max_pattern_length = max_pattern_length;
    }

    std::string Generate(std::string_view seed, std::size_t sequence_number) const {
        return Generate(FNV1aHash(seed), sequence_number);
    }

    std::string Generate(std::uint32_t seed, std::size_t sequence_number) const {
        std::vector<std::string> substitutions;
        for (const auto& generator : generators) {
            seed += kSeedStep;
            substitutions.push_back(generator->Generate(seed, sequence_number));
        }
        return pattern->Format(substitutions);
    }
};

PatternGenerator::PatternGenerator(const DictionarySet& dictionaries, PatternPtr pattern)
    : impl_{dictionaries, pattern} {
}

PatternGenerator::PatternGenerator(const DictionarySet& dictionaries, PatternPtr pattern, PatternSettings settings)
    : impl_{dictionaries, pattern, settings} {
}

PatternGenerator::PatternGenerator(const binary::DictionarySet& dictionaries, PatternPtr pattern)
    : impl_{dictionaries, pattern} {
}

PatternGenerator::PatternGenerator(
    const binary::DictionarySet& dictionaries,
    PatternPtr pattern,
    PatternSettings settings
)
    : impl_{dictionaries, pattern, settings} {
}

PatternGenerator::~PatternGenerator() = default;

std::string PatternGenerator::operator()(std::uint32_t seed, std::size_t sequence_number) const {
    return impl_->Generate(seed, sequence_number);
}

std::string PatternGenerator::operator()(std::string_view seed, std::size_t sequence_number) const {
    return impl_->Generate(seed, sequence_number);
}

numeric::BigInt PatternGenerator::GetCapacity() const {
    return impl_->settings.capacity;
}

std::size_t PatternGenerator::GetMaxPatternLength() const {
    return impl_->settings.max_pattern_length;
}

const PatternSettings& PatternGenerator::GetSettings() const {
    return impl_->settings;
}

std::uint32_t PatternGenerator::SeedHash(std::string_view seed) {
    return FNV1aHash(seed);
}

}  // namespace slugkit::generator
